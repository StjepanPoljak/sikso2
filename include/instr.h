#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h> /* log_err */

#define log_err(PREFIX, FMT, ...) printf(PREFIX " (!) " FMT "\n", ## __VA_ARGS__)

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

#define MODE_ZERO_PAGE		0x0
#define MODE_ZERO_PAGE_X	0x1
#define MODE_ABSOLUTE		0x2
#define MODE_ABSOLUTE_X		0x3
#define MODE_ABSOLUTE_Y		0x4
#define MODE_INDIRECT_X		0x5
#define MODE_INDIRECT_Y		0x6
#define MODE_IMMEDIATE		0x7
#define MODE_ACCUMULATOR	0x8
#define MODE_BRANCH		0x9
#define MODE_IMPLIED		0xA
#define MODE_STACK		0xB
#define MODE_REGISTER		0xC
#define MODE_ZERO_PAGE_Y	0xD
#define MODE_INDIRECT		0xE
#define MODE_STATUS		0xF

/* add cycle if boundary crossed 0x20000 */

#define MODE_EXTRA_CYCLE	1 << 5

/* CPU model */

#define CPU_6502_CORE		1

typedef uint8_t instr_mode_t;
typedef uint8_t cpu_model_t;
typedef uint8_t opcode_t;

typedef struct {
	uint8_t opcode;
	uint8_t cycles;
	uint8_t length;
	instr_mode_t mode;
	cpu_model_t supported;
} subinstr_t;

typedef int(*action_t)(subinstr_t*, uint16_t, void*);

typedef struct {
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
bool get_subinstr(const char[], instr_mode_t, subinstr_t**);

#endif
