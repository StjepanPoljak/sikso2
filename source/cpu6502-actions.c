#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "device.h"
#include "instr.h"
#include "cpu.h"

#define loga_err(FMT, ...) log_err("[ACT]", FMT, ## __VA_ARGS__)

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

static uint16_t get_page(struct device_t* device, uint16_t addr) {

	return (addr - device->ram.zero_page) / device->ram.page_size;
}

static uint16_t zero_page_wrap_around(struct device_t* device, uint16_t addr) {

	if (get_page(device, addr) == 0)
		return addr;

	return ((addr - device->ram.zero_page) % device->ram.page_size)
	       + device->ram.zero_page;
}

#define assert_zero_page(device, addr) \
	if (get_page(device, addr) != 0) { \
		loga_err("Address %d not in zero page", \
			 addr); \
		return DEVICE_INTERNAL_BUG; \
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

	return ret;
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
		*new_addr = zero_page_wrap_around(device, addr + device->cpu->X);
		break;

	case MODE_ABSOLUTE:
		*new_addr = addr;
		break;

	case MODE_ABSOLUTE_X:
		*new_addr = addr + device->cpu->X;
		break;

	case MODE_ABSOLUTE_Y:
		*new_addr = addr + device->cpu->Y;
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

	ret = get_addr(device, addr, mode, &new_addr);
	if (ret < 0)
		return ret;

	ret = device->read(device, new_addr, byte);
	if (ret < 0) {
		loga_err("Error reading address %d", new_addr);

		return ret;
	}

	return ret;
}

static int write_byte(struct device_t* device, uint16_t addr,
		      instr_mode_t mode, uint8_t byte) {
	uint16_t new_addr;
	int ret;

	ret = get_addr(device, addr, mode, &new_addr);
	if (ret < 0)
		return ret;

	ret = device->write(device, new_addr, byte);
	if (ret < 0) {
		loga_err("Error writing %.2x to address %d", byte, new_addr);

		return ret;
	}

	return ret;
}

#define check_carry_adc(cpu, x, y) \
	if ((int)x + (int)y > 255) set_C(cpu); \
	else clr_C(cpu)

#define check_carry_sbc(cpu, x, y) \
	if ((int)x + (int)y > 255) clr_C(cpu); \
	else set_C(cpu)

#define get_sign(x) \
	(x & ((uint8_t)1 << 7) ? -1 : 1)

#define get_abs(x) \
	(x & ~((uint8_t)1 << 7))

#define get_signed(x) \
	((int)get_sign(x) * (int)get_abs(x))

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

DEFINE_ACTION(ADC) {
	uint8_t byte;
	bool overflow;
	int ret;

	switch (_mode) {
	case MODE_IMMEDIATE:
		check_carry_adc(_device->cpu, _device->cpu->A, arg);
		check_overflow_adc(_device->cpu, _device->cpu->A, arg);
		_device->cpu->A += arg;

		break;
	default:
		ret = get_byte(_device, arg, _mode, &byte);
		if (ret < 0)
			return ret;

		check_carry_adc(_device->cpu, _device->cpu->A, byte);
		check_overflow_adc(_device->cpu, _device->cpu->A, byte);
		_device->cpu->A += byte;

		break;
	}

	affect_NZ(_device, byte)

	return ret;
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
		if (ret < 0)
			return ret;

		break;
	}

	affect_NZ(_device, byte);

	return ret;
}

DEFINE_ACTION(STA) {

	return write_byte(_device, arg, _mode, _device->cpu->A);
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

static instr_t* instr_named(char name[3]) {
	instr_t* i;

	for_each_instr(i)
		if (!strncmp(i->name, name, 3))
			return i;

	return NULL;
}

void init_cpu_6502_actions(void) {

	add_action(ADC);
	add_action(LDA);
	add_action(STA);
	add_action(JMP);
	add_action(NOP);

	return;
}
