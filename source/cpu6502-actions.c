#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "device.h"
#include "instr.h"
#include "cpu.h"
#include "common.h"

#define ASIG "ACT"

#define loga_err(FMT, ...) log_err(ASIG, FMT, ## __VA_ARGS__)

extern instr_t* get_instr_list(void);

#define DEFINE_ACTION(NAME) \
	static int NAME ## _action(subinstr_t* s, uint16_t arg, void* data)

#define _device \
	((struct device_t*)data)

#define add_action(NAME) \
	instr_named(#NAME)->action = NAME ## _action;

#define _mode \
	(s->mode & 0xFF)

/* if return value is 0, set Z flag; set N flag
 * to match 7th bit of return value */
#define affect_NZ(device, ret) \
	if (ret == 0) set_Z((device)->cpu); \
	if (ret & 0x80) set_N((device)->cpu); \
	else clr_N((device)->cpu);

#define get_sign(x) \
	(x & ((uint8_t)1 << 7) ? -1 : 1)

#define get_abs(x) \
	(x & ~((uint8_t)1 << 7))

#define get_signed(x) \
	((int)get_sign(x) * (int)get_abs(x))

#define assert_zero_page(device, addr) \
	if (get_page(device, addr) != 0) { \
		loga_err("Address %d not in zero page", \
			 addr); \
		return DEVICE_INTERNAL_BUG; \
	}

#define check_carry_adc(cpu, x, y) \
	if ((int)x + (int)y > 255) set_C(cpu); \
	else clr_C(cpu)

#define check_carry_sbc(cpu, x, y) \
	if ((int)x + (int)y > 255) clr_C(cpu); \
	else set_C(cpu)

#define check_overflow_adc(cpu, x, y) \
	if ((get_signed(x) + get_signed(y) + (int)get_C(cpu) >= 127) \
	 || (get_signed(x) + get_signed(y) + (int)get_C(cpu) <= -128)) \
		set_V(cpu); \
	else clr_V(cpu)

#define check_overflow_sbc(cpu, x, y) \
	if ((get_signed(x) - get_signed(y) + (int)get_C(cpu) - 1 >= 127) \
	 || (get_signed(x) - get_signed(y) + (int)get_C(cpu) - 1 <= -128)) \
		set_V(cpu); \
	else clr_V(cpu)

static uint16_t get_page(struct device_t* device, uint16_t addr) {

	return (addr - device->ram.zero_page) / device->ram.page_size;
}

static uint16_t zero_page_wrap_around(struct device_t* device,
				      uint16_t addr) {

	if (get_page(device, addr) == 0)
		return addr;

	return ((addr - device->ram.zero_page) % device->ram.page_size)
	       + device->ram.zero_page;
}

static int is_page_boundary_crossed(struct device_t* device, uint16_t addr) {

	if (device->instr_frag.extrcyc)
		return 0;

	return get_page(device, addr) != get_page(device, addr + 1)
	     ? DEVICE_NEED_EXTRA_CYCLE : 0;
}

static int get_indX(struct device_t* device, uint16_t addr,
		    uint16_t* new_addr) {
	uint16_t wrapped;
	uint8_t low_byte;
	uint8_t high_byte;
	int ret;

	ret = 0;

	wrapped = zero_page_wrap_around(device, addr + device->cpu->X);

	ret = device->read(device, wrapped + 1, &high_byte);
	if (ret)
		return ret;
	ret = device->read(device, wrapped, &low_byte);
	if (ret)
		return ret;

	*new_addr = ((uint16_t)high_byte << 8) | (uint16_t)low_byte;

	return ret;
}

static int get_indY(struct device_t* device, uint16_t addr,
		    uint16_t* new_addr) {
	uint8_t zploc;
	uint8_t low_byte;
	uint8_t high_byte;
	int ret;

	ret = 0;

	ret = device->read(device, zploc + 1, &high_byte);
	if (ret)
		return ret;

	ret = device->read(device, zploc, &low_byte);
	if (ret)
		return ret;

	*new_addr = ((uint16_t)high_byte << 8) | (uint16_t)low_byte;
	*new_addr += (uint16_t)device->cpu->Y;

	return is_page_boundary_crossed(device, *new_addr);
}

static int get_addr(struct device_t* device, uint16_t addr,
		    instr_mode_t mode, uint16_t* new_addr) {
	int ret;

	switch (mode) {

	case MODE_ZERO_PAGE:
		assert_zero_page(device, addr);
		*new_addr = addr;
		break;

	case MODE_ZERO_PAGE_X:
		assert_zero_page(device, addr);
		*new_addr = zero_page_wrap_around(device,
						  addr + device->cpu->X);
		break;

	case MODE_ABSOLUTE:
		*new_addr = addr;
		break;

	case MODE_ABSOLUTE_X:
		*new_addr = addr + device->cpu->X;
		ret = is_page_boundary_crossed(device, *new_addr);
		break;

	case MODE_ABSOLUTE_Y:
		*new_addr = addr + device->cpu->Y;
		ret = is_page_boundary_crossed(device, *new_addr);
		break;

	case MODE_INDIRECT_X:
		ret = get_indX(device, addr, new_addr);
		if (ret < 0) {
			loga_err("Error reading address %d (X=%d)"
				 "(indexed indirect)", addr,
				 device->cpu->X);

			return ret;
		}
		break;

	case MODE_INDIRECT_Y:
		ret = get_indY(device, addr, new_addr);
		if (ret < 0) {
			loga_err("Error reading address %d "
				 "(indirect indexed)", addr);

			return ret;
		}

		break;
	}

	return ret;
}

static int get_byte(struct device_t* device, uint16_t addr,
		     instr_mode_t mode, uint8_t* byte) {
	uint16_t new_addr;
	int ret;
	bool boundary_crossed;

	ret = get_addr(device, addr, mode, &new_addr);
	if (ret < 0)
		return ret;
	boundary_crossed = ret == DEVICE_NEED_EXTRA_CYCLE;

	ret = device->read(device, new_addr, byte);
	if (ret < 0) {
		loga_err("Error reading address %d", new_addr);

		return ret;
	}

	return boundary_crossed ? DEVICE_NEED_EXTRA_CYCLE : ret;
}

static int write_byte(struct device_t* device, uint16_t addr,
		      instr_mode_t mode, uint8_t byte) {
	uint16_t new_addr;
	int ret;
	bool boundary_crossed;

	ret = get_addr(device, addr, mode, &new_addr);
	if (ret < 0)
		return ret;
	boundary_crossed = ret == DEVICE_NEED_EXTRA_CYCLE;

	ret = device->write(device, new_addr, byte);
	if (ret < 0) {
		loga_err("Error writing %.2x to address %d", byte, new_addr);

		return ret;
	}

	return boundary_crossed ? DEVICE_NEED_EXTRA_CYCLE : ret;
}

DEFINE_ACTION(ADC) {
	uint8_t byte;
	bool overflow;
	int ret;

	switch (_mode) {
	case MODE_IMMEDIATE:
		check_carry_adc(_device->cpu, _device->cpu->A, arg);
		check_overflow_adc(_device->cpu, _device->cpu->A, arg);
		_device->cpu->A += (arg + get_C(_device->cpu));

		break;
	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;

		check_carry_adc(_device->cpu, _device->cpu->A, byte);
		check_overflow_adc(_device->cpu, _device->cpu->A, byte);
		_device->cpu->A += (byte + get_C(_device->cpu));

		break;
	}

	affect_NZ(_device, _device->cpu->A)

	return ret;
}

typedef enum {
	BITWISE_AND,
	BITWISE_EOR,
	BITWISE_ORA
} bitwise_t;

static int bitwise_comm(subinstr_t* s, uint16_t arg, void* data, bitwise_t comm) {
	uint8_t byte;
	int ret;

	ret = 0;

	switch(_mode) {

	case MODE_IMMEDIATE:
		switch (comm) {
		case BITWISE_AND:
			_device->cpu->A &= arg;
			break;

		case BITWISE_EOR:
			_device->cpu->A ^= arg;
			break;

		case BITWISE_ORA:
			_device->cpu->A |= arg;
			break;
		default:
			return ret;
		}

		break;

	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;

		switch (comm) {
		case BITWISE_AND:
			_device->cpu->A &= byte;
			break;

		case BITWISE_EOR:
			_device->cpu->A ^= byte;
			break;

		case BITWISE_ORA:
			_device->cpu->A |= byte;
			break;
		default:
			return ret;
		}

		break;
	}

	affect_NZ(_device, _device->cpu->A);

	return ret;

}

DEFINE_ACTION(AND) {

	return bitwise_comm(s, arg, data, BITWISE_AND);
}

typedef enum {
	BIT_SHIFT_ASL,
	BIT_SHIFT_LSR,
	BIT_SHIFT_ROL,
	BIT_SHIFT_ROR
} bit_shift_t;

static void shift(struct device_t* device, bit_shift_t shift_type,
		  uint8_t* byte) {
	bool carry;
	bool precarry;

	precarry = get_C(device->cpu) != 0;

	carry = shift_type == BIT_SHIFT_ASL
	     || shift_type == BIT_SHIFT_ROL
	      ? (*byte & ((uint8_t)1 << 7)) != 0
	      : (*byte & (uint8_t)1) != 0;
	*byte = shift_type == BIT_SHIFT_ASL
	     || shift_type == BIT_SHIFT_ROL
	      ? *byte << 1
	      : *byte >> 1;

	if (carry)
		set_C(device->cpu);

	if (precarry)
		*byte = shift_type == BIT_SHIFT_ASL
		     || shift_type == BIT_SHIFT_ROL
		      ? *byte | 1
		      : *byte | ((uint8_t)1 << 7);
	else
		*byte = shift_type == BIT_SHIFT_ASL
		     || shift_type == BIT_SHIFT_ROL
		      ? *byte & ~((uint8_t)1)
		      : *byte & ~((uint8_t)1 << 7);

	return;
}

static int shift_comm(subinstr_t* s, uint16_t arg, void* data,
		      bit_shift_t shift_type) {
	uint8_t byte;
	int ret;

	ret = 0;

	switch (_mode) {

	case MODE_ACCUMULATOR:
		shift(_device, shift_type, &(_device->cpu->A));
		affect_NZ(_device, _device->cpu->A);

		break;

	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;

		shift(_device, shift_type, &byte);

		ret = write_byte(_device, arg, _mode, byte);
		if (ret)
			return ret;

		affect_NZ(_device, byte);

		break;
	}

	return ret;
}

DEFINE_ACTION(ASL) {

	return shift_comm(s, arg, data, BIT_SHIFT_ASL);
}

DEFINE_ACTION(BIT) {
	int ret;
	uint8_t byte;

	ret = 0;

	ret = get_byte(_device, arg, _mode, &byte);
	if (ret)
		return ret;

	if (byte & _device->cpu->A)
		clr_Z(_device->cpu);
	else
		set_Z(_device->cpu);

	if (byte & ((uint8_t)1 << 7))
		set_N(_device->cpu);
	else
		clr_N(_device->cpu);

	if (byte & ((uint8_t)1 << 6))
		set_V(_device->cpu);
	else
		clr_N(_device->cpu);

	return ret;
}

DEFINE_ACTION(BRK) {

	_device->cpu->PC++;

	return DEVICE_GENERATE_NMI;
}

static int sbc_comm(subinstr_t* s, uint16_t arg, void* data,
		    bool cmp_only, uint8_t* reg) {
	uint8_t byte;
	bool overflow;
	int ret;

	switch (_mode) {
	case MODE_IMMEDIATE:
		check_carry_sbc(_device->cpu, *reg, arg);

		if (!cmp_only) {
			check_overflow_sbc(_device->cpu, *reg, arg);
			*reg -= (arg + get_C(_device->cpu));
		}

		break;
	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;

		check_carry_sbc(_device->cpu, *reg, byte);

		if (cmp_only) {
			check_overflow_sbc(_device->cpu, *reg, byte);
			*reg -= (byte + get_C(_device->cpu));
		}

		break;
	}

	affect_NZ(_device, _device->cpu->A)

	return ret;


}

DEFINE_ACTION(CMP) {

	return sbc_comm(s, arg, data, true, &(_device->cpu->A));
}

DEFINE_ACTION(CPX) {

	return sbc_comm(s, arg, data, true, &(_device->cpu->X));
}

DEFINE_ACTION(CPY) {

	return sbc_comm(s, arg, data, true, &(_device->cpu->Y));
}

DEFINE_ACTION(DEC) {
	uint8_t byte;
	int ret;

	ret = get_byte(_device, arg, _mode, &byte);
	if (ret)
		return ret;

	check_carry_sbc(_device->cpu, byte, 1);

	byte--;

	ret = write_byte(_device, arg, _mode, byte);
	if (ret)
		return ret;

	affect_NZ(_device, byte);

	return ret;
}

DEFINE_ACTION(EOR) {

	return bitwise_comm(s, arg, data, BITWISE_EOR);
}

DEFINE_ACTION(CLC) {

	clr_C(_device->cpu);

	return 0;
}

DEFINE_ACTION(SEC) {

	set_C(_device->cpu);

	return 0;
}

DEFINE_ACTION(CLI) {

	clr_I(_device->cpu);

	return 0;
}

DEFINE_ACTION(SEI) {

	set_I(_device->cpu);

	return 0;
}

DEFINE_ACTION(CLV) {

	clr_V(_device->cpu);

	return 0;
}

DEFINE_ACTION(CLD) {

	clr_D(_device->cpu);

	return 0;
}

DEFINE_ACTION(SED) {

	set_D(_device->cpu);

	return 0;
}

DEFINE_ACTION(LDA) {
	uint8_t byte;
	int ret;

	switch (_mode) {

	case MODE_IMMEDIATE:
		_device->cpu->A = arg;
		byte = arg;

		break;
	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;

		_device->cpu->A = byte;

		break;
	}

	affect_NZ(_device, byte);

	return ret;
}

DEFINE_ACTION(JMP) {
	uint8_t high_byte;
	uint8_t low_byte;
	int ret;

	ret = 0;

	switch (_mode) {
	case MODE_ABSOLUTE:
		_device->cpu->PC = arg;
		break;

	case MODE_INDIRECT:
		ret = get_byte(_device, arg, _mode, &low_byte);
		if (ret)
			return ret;

		ret = get_byte(_device,
			zero_page_wrap_around(_device, arg + 1),
			_mode, &high_byte);
		if (ret)
			return ret;

		_device->cpu->PC = ((uint16_t)high_byte << 8);
		_device->cpu->PC |= (uint16_t)low_byte;

		break;

	}

	return ret;
}

DEFINE_ACTION(NOP) {

	return 0;
}

DEFINE_ACTION(SBC) {

	return sbc_comm(s, arg, data, false, &(_device->cpu->A));
}

DEFINE_ACTION(STA) {

	return write_byte(_device, arg, _mode, _device->cpu->A);
}

static instr_t* instr_named(char name[3]) {
	instr_t* i;

	for_each_instr(i)
		if (!strncmp(i->name, name, 3))
			return i;

	return NULL;
}

void init_cpu_6502_actions(void) {

	add_action(ADC);
	add_action(ASL);
	add_action(BIT);
	add_action(BRK);
	add_action(CMP);
	add_action(DEC);
	add_action(JMP);
	add_action(LDA);
	add_action(NOP);
	add_action(SBC);
	add_action(STA);

	return;
}
