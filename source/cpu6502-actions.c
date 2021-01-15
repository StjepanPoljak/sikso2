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
	(((x) & ((uint8_t)1 << 7)) ? -1 : 1)

#define get_abs(x) \
	((x) & ~((uint8_t)1 << 7))

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
	if ((int)x < (int)y) clr_C(cpu); \
	else set_C(cpu)

#define check_overflow_adc(cpu, x, y) \
	if (get_sign(x) == get_sign(y) && get_sign(x + y + get_C(cpu)) != get_sign(x)) \
			set_V(cpu); \
	else clr_V(cpu)

#define check_overflow_sbc(cpu, x, y) \
	check_overflow_adc(cpu, x, 255-y)
	/*if (get_sign(x) != get_sign(y) && get_sign(x - y + get_C(cpu) - 1) != get_sign(x)) \
			set_V(cpu); \
	else clr_V(cpu)*/

#define get_page(addr) ((addr) & 0xFF00)

#define zero_page_wrap_around(addr) ((addr) & 0xFF)

#define is_page_boundary_crossed(addr) \
	(get_page(addr) != get_page((addr) + 1))

static uint16_t get_indX(struct device_t* device,
			 uint16_t addr) {
	uint16_t wrapped;

	wrapped = zero_page_wrap_around(addr + device->cpu->X);

	return ((uint16_t)device_read(device, wrapped + 1) << 8) | (uint16_t)device_read(device, wrapped);
}

static uint16_t get_indY(struct device_t* device, uint16_t addr) {

	return (((uint16_t)device_read(device, addr + 1) << 8) | (uint16_t)device_read(device, addr)) + (uint16_t)device->cpu->Y;
}

static int get_addr(struct device_t* device, uint16_t addr,
		    instr_mode_t mode, uint16_t* new_addr) {
	int ret;

	ret = 0;

	switch (mode) {

	case MODE_ZERO_PAGE:
		//assert_zero_page(device, addr);
		*new_addr = addr;
		break;

	case MODE_ZERO_PAGE_X:
		//assert_zero_page(device, addr);
		*new_addr = addr + device->cpu->X;
		break;

	case MODE_ABSOLUTE:
		*new_addr = addr;
		break;

	case MODE_ABSOLUTE_X:
		*new_addr = addr + device->cpu->X;
		ret = is_page_boundary_crossed(*new_addr) ? DEVICE_NEED_EXTRA_CYCLE : 0;
		break;

	case MODE_ABSOLUTE_Y:
		*new_addr = addr + device->cpu->Y;
		ret = is_page_boundary_crossed(*new_addr) ? DEVICE_NEED_EXTRA_CYCLE : 0;
		break;

	case MODE_INDIRECT_X:
		*new_addr = get_indX(device, addr);
		break;

	case MODE_INDIRECT_Y:
		*new_addr = get_indY(device, addr);
		ret = is_page_boundary_crossed(addr) ? DEVICE_NEED_EXTRA_CYCLE : 0;
		break;
	}

	return ret;
}

static int get_byte(struct device_t* device, uint16_t addr,
		     instr_mode_t mode, uint8_t* byte) {
	uint16_t new_addr;
	int ret;

	ret = get_addr(device, addr, mode, &new_addr);
	if (ret < 0)
		return ret;

	*byte = device_read(device, new_addr);

	return ret;
}

static int write_byte(struct device_t* device, uint16_t addr,
		      instr_mode_t mode, uint8_t byte) {
	uint16_t new_addr;
	int ret;

	ret = get_addr(device, addr, mode, &new_addr);
	if (ret < 0)
		return ret;

	device_write(device, new_addr, byte);

	return ret;
}

DEFINE_ACTION(ADC) {
	uint8_t byte;
	int ret;
	uint8_t c;

	c = get_C(_device->cpu);

	switch (_mode) {
	case MODE_IMMEDIATE:
		check_overflow_adc(_device->cpu, _device->cpu->A, arg);
		check_carry_adc(_device->cpu, _device->cpu->A, arg);
		_device->cpu->A += (arg + c);

		break;
	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;

		check_overflow_adc(_device->cpu, _device->cpu->A, byte);
		check_carry_adc(_device->cpu, _device->cpu->A, byte);
		_device->cpu->A += (byte + c);

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

	set_B(_device->cpu);

	return DEVICE_GENERATE_NMI;
}

static int sbc_comm(subinstr_t* s, uint16_t arg, void* data,
		    bool cmp_only, uint8_t* reg) {
	uint8_t byte;
	int ret;
	uint8_t c;

	c = get_C(_device->cpu);

	switch (_mode) {
	case MODE_IMMEDIATE:
		if (!cmp_only) {
			check_overflow_sbc(_device->cpu, *reg, arg);
		}

		check_carry_sbc(_device->cpu, *reg, arg);

		if (!cmp_only) {
			*reg -= (arg - c + 1);
		}

		break;
	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret)
			return ret;


		if (!cmp_only) {
			check_overflow_sbc(_device->cpu, *reg, byte);
		}
		check_carry_sbc(_device->cpu, *reg, byte);
		if (!cmp_only) {
			*reg -= (byte + c - 1);
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
			zero_page_wrap_around(arg + 1),
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
	add_action(AND);
	add_action(ASL);
	add_action(BIT);
	add_action(BRK);
	add_action(CMP);
	add_action(CPX);
	add_action(CPY);
	add_action(DEC);
	add_action(EOR);
	add_action(CLC);
	add_action(SEC);
	add_action(CLI);
	add_action(SEI);
	add_action(CLV);
	add_action(CLD);
	add_action(SED);
	add_action(JMP);
	add_action(LDA);
	add_action(NOP);
	add_action(SBC);
	add_action(STA);

	return;
}
