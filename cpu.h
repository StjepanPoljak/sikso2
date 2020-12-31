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

void init_cpu(struct cpu_6502_t* cpu, instr_t* instr_list);
void start_cpu(struct cpu_6502_t* cpu, uint16_t zero_page,
	       uint16_t page_size, uint16_t load_addr);
void dump_cpu(struct cpu_6502_t* cpu);

#endif
