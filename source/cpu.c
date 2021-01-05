#include "cpu.h"

#include "common.h"

#define CSIG "CPU"

#ifdef CPU_TRACE
#define ctrace(FMT, ...) trace(FMT, ## __VA_ARGS__)
#define ctracei(FMT, ...) tracei(CSIG, FMT, ## __VA_ARGS__)
#else
#define ctrace(FMT, ...) ;
#define ctracei(FMT, ...) ;
#endif

DEFINE_INSTR_MAP(instr_map);

void init_cpu(struct cpu_6502_t* cpu, instr_t* instr_list) {

	populate_imap(instr_map);

#ifdef CPU_TRACE
	print_imap(instr_map);
#endif

	cpu->instr_map = instr_map;

#ifdef CPU_TRACE
	dump_cpu(cpu, CPU_DUMP_PRETTY);
#endif

	return;
}

void start_cpu(struct cpu_6502_t* cpu, uint16_t zero_page,
	       uint16_t page_size, uint16_t load_addr) {

	cpu->A = 0x0;
	cpu->X = 0x0;
	cpu->Y = 0x0;
	cpu->S = zero_page + page_size * 2;
	cpu->P = 0x0;
	cpu->PC = load_addr;

	return;
}

/* ======= debug ======= */

void dump_cpu(struct cpu_6502_t* cpu, cpu_dump_mode_t mode) {

	switch (mode) {
	case CPU_DUMP_SIMPLE:
		printf("%.2x %.2x %.2x %.2x %.2x %.4x\n",
		       cpu->A, cpu->X, cpu->Y,
		       cpu->S, cpu->P, cpu->PC);
		break;

	case CPU_DUMP_ONELINE:
	case CPU_DUMP_PRETTY:
		printf("A: %.2x\tX: %.2x\tY: %.2x%c",
		       cpu->A, cpu->X, cpu->Y,
		       mode == CPU_DUMP_PRETTY ? '\n' : ' ');
		printf("S: %.2x\tP: %.2x\tPC: %.4x\n",
		       cpu->S, cpu->P, cpu->PC);
		break;

	default:
		printf("[CPU] (*) Invalid mode (%d) in dump_cpu!", mode);
		break;
	}

	return;
}
