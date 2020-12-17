#include "instr.h"

#include <stdio.h>

status_t populate_imap(instr_map_t* instr_map) {
	instr_t* i;
	subinstr_t* s;
	instr_map_t* curr;

	for_each_subinstr(s, i) {

		curr = &(instr_map[s->opcode]);

		if (!IS_NULL_ENTRY(curr))
			return STATUS_HASH_INCONSISTENCY;

		*curr = (instr_map_t) {
			.instr = i,
			.subinstr = s
		};

	}

	return STATUS_SUCCESS;
}

#define MAX_MODE_NAME 16

typedef struct mode_map_t {
	instr_mode_t mode;
	const char name[MAX_MODE_NAME];
} mode_map_t;

#define CASE_MODE(m, n) case (MODE_ ## m): \
	printf("%s", n); \
	break

static void print_mode(instr_mode_t mode) {

	switch(mode & 0xf) {
		CASE_MODE(ZERO_PAGE, "Zero Page");
		CASE_MODE(ZERO_PAGE_X, "Zero Page, X");
		CASE_MODE(ABSOLUTE, "Absolute");
		CASE_MODE(ABSOLUTE_X, "Absolute, X");
		CASE_MODE(ABSOLUTE_Y, "Absolute, Y");
		CASE_MODE(INDIRECT_X, "Indirect, X");
		CASE_MODE(INDIRECT_Y, "Indirect, Y");
		CASE_MODE(IMMEDIATE, "Immediate");
		CASE_MODE(ACCUMULATOR, "Accumulator");
		CASE_MODE(BRANCH, "Branch");
		CASE_MODE(IMPLIED, "Implied");
		CASE_MODE(STACK, "Stack");
		CASE_MODE(REGISTER, "Register");
		CASE_MODE(ZERO_PAGE_Y, "Zero Page, Y");
		CASE_MODE(INDIRECT, "Indirect");
		CASE_MODE(STATUS, "Status");
	}

	return;
}

void print_imap(instr_map_t* instr_map) {
	int i;
	instr_map_t* curr;

	for(i = 0; i < INSTR_MAP_SIZE; i++) {
		curr = &(instr_map[i]);

		if (IS_NULL_ENTRY(curr))
			continue;

		printf("[%.2x]: %s (", i, curr->instr->name);
		print_mode(curr->subinstr->mode);
		printf(")\n");
	}

	return;
}
