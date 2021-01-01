#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "device.h"
#include "instr.h"
#include "cpu.h"

extern instr_t* get_instr_list(void);

#define DEFINE_ACTION(NAME) \
	static int NAME ## _action(subinstr_t* s, uint16_t arg, void* data)

#define device \
	((struct device_t*)data)

#define add_action(NAME) \
	instr_named(#NAME)->action = NAME ## _action;

#define mode \
	(s->mode & 0xFF)

#define affect_NZ(device, ret) \
	if (ret == 0) set_Z((device)->cpu); \
	if (ret & 0x80) set_N((device)->cpu); \
	else clr_N((device)->cpu);

DEFINE_ACTION(LDA) {
	uint8_t ret;

	switch (mode) {
	case MODE_IMMEDIATE:
		device->cpu->A = arg;
		ret = arg;
		break;
	default:
		break;
	}

	affect_NZ(device, ret);

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

	add_action(LDA);

	return;
}
