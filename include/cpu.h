#ifndef CPU_6502_H
#define CPU_6502_H

#include "instr.h"

/* PAGES (size is 256 bytes each)
 * zero page	0x0000 - 0x00FF
 * stack	0x0100 - 0x01FF */

/* reset vector:
 * 0xFFFC - low bits of PC
 * 0xFFFD - high bits of PC */

typedef struct cpu_6502_t {
	uint8_t A;	/* accumulator */
	uint8_t X;	/* X index register */
	uint8_t Y;	/* Y index register */
	uint8_t S;	/* stack pointer */
	uint8_t P;	/* status flags (7:0) */
	uint16_t PC;	/* program counter */
	instr_map_t* instr_map;
} cpu_6502_t;

typedef enum {
	CPU_DUMP_SIMPLE,
	CPU_DUMP_ONELINE,
	CPU_DUMP_PRETTY
} cpu_dump_mode_t;

#define set_bit(cpu, bit) \
	(cpu)->P |= ((uint8_t)1 << bit)
#define clr_bit(cpu, bit) \
	(cpu)->P = (cpu)->P & ~((uint8_t)((uint8_t)1 << bit))
#define get_bit(cpu, bit) \
	(((cpu)->P & ((uint8_t)1 << bit)) >> bit)

#define set_C(cpu) set_bit(cpu, 0)
#define clr_C(cpu) clr_bit(cpu, 0)
#define get_C(cpu) get_bit(cpu, 0)

#define set_Z(cpu) set_bit(cpu, 1)
#define clr_Z(cpu) clr_bit(cpu, 1)
#define get_Z(cpu) get_bit(cpu, 1)

#define set_I(cpu) set_bit(cpu, 2)
#define clr_I(cpu) clr_bit(cpu, 2)
#define get_I(cpu) get_bit(cpu, 2)

#define set_D(cpu) set_bit(cpu, 3)
#define clr_D(cpu) clr_bit(cpu, 3)
#define get_D(cpu) get_bit(cpu, 3)

#define set_V(cpu) set_bit(cpu, 6)
#define clr_V(cpu) clr_bit(cpu, 6)
#define get_V(cpu) get_bit(cpu, 6)

#define set_N(cpu) set_bit(cpu, 7)
#define clr_N(cpu) clr_bit(cpu, 7)
#define get_N(cpu) get_bit(cpu, 7)

void init_cpu(struct cpu_6502_t* cpu, instr_t* instr_list);
void start_cpu(struct cpu_6502_t* cpu, uint16_t zero_page,
	       uint16_t page_size, uint16_t load_addr);
void dump_cpu(struct cpu_6502_t* cpu, cpu_dump_mode_t mode);

#endif
