#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdint.h>
#include "instr.h"

typedef enum {
	DISASM_PRETTY,
	DISASM_SIMPLE
} disasm_mode_t;

int translate(const char* infile, unsigned int load_addr,
	      int(*op)(unsigned int, const uint8_t*, void*),
	      void *data);

int disassemble(const char* infile, instr_map_t* map,
		disasm_mode_t disasm_mode);

#endif
