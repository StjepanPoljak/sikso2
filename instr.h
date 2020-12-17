#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stdlib.h>

#include "cpu.h"

typedef uint8_t status_t;

#define STATUS_SUCCESS			0x0
#define STATUS_HASH_INCONSISTENCY	0x1

#define OPCODE_BYTES 1

#define for_each_instr(i) for (i = get_instr_list(); \
	i < get_instr_list() + get_instr_list_size(); i++)

#define for_each_subinstr(s, i) for_each_instr(i) \
	for (s = i->list; s < i->list + i->size; s++)

#define INSTR_MAP_SIZE (1 << (OPCODE_BYTES * 8))
#define DEFINE_INSTR_MAP(m) \
	instr_map_t m[INSTR_MAP_SIZE] = { \
		(instr_map_t) { \
			.instr = NULL, \
			.subinstr = NULL \
		} \
	}

#define IS_NULL_ENTRY(m) (!m->instr || !m->subinstr)

/* mode map:
 *  -------------------
 * |  7:5  | 4 |  3:0  |
 *  -------------------
 * 7:5	- reserved
 * 4	- extra cycle
 * 3:0	- instruction mode */

/* instruction mode */

#define MODE_ZERO_PAGE		0x00
#define MODE_ZERO_PAGE_X	0x01
#define MODE_ABSOLUTE		0x02
#define MODE_ABSOLUTE_X		0x03
#define MODE_ABSOLUTE_Y		0x04
#define MODE_INDIRECT_X		0x05
#define MODE_INDIRECT_Y		0x06
#define MODE_IMMEDIATE		0x07
#define MODE_ACCUMULATOR	0x08
#define MODE_BRANCH		0x09
#define MODE_IMPLIED		0x0A
#define MODE_STACK		0x0B
#define MODE_REGISTER		0x0C
#define MODE_ZERO_PAGE_Y	0x0D
#define MODE_INDIRECT		0x0E
#define MODE_STATUS		0x0F

/* add cycle if boundary crossed 0x20000 */

#define MODE_EXTRA_CYCLE	1 << 5

/* CPU model */

#define CPU_6502_CORE		1

typedef uint8_t instr_mode_t;
typedef uint8_t cpu_model_t;
typedef uint8_t opcode_t;
typedef void*(*action_t)(instr_mode_t, void*);

typedef struct subinstr_t {
	uint8_t opcode;
	uint8_t cycles;
	uint8_t length;
	instr_mode_t mode;
	cpu_model_t supported;
} subinstr_t;

typedef struct instr_t {
	char name[3];
	action_t action;
	subinstr_t* list;
	uint8_t size;
} instr_t;

typedef struct instr_map_t {
	instr_t* instr;
	subinstr_t* subinstr;
} instr_map_t;

status_t populate_imap(instr_map_t*);
void print_imap(instr_map_t*);
instr_t* get_instr_list(void);
size_t get_instr_list_size(void);

#endif
