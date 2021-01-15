#include "cpu.h"

#include <string.h>

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

void start_cpu(struct cpu_6502_t* cpu, uint16_t load_addr,
	       uint16_t stack_addr) {

	cpu->A = 0x0;
	cpu->X = 0x0;
	cpu->Y = 0x0;
	cpu->S = stack_addr;
	cpu->P = (uint8_t)1 << 5;
	cpu->PC = load_addr;

	return;
}

/* ======= debug ======= */

cpu_dump_t cpu_dump_data[] = {
	(cpu_dump_t){
		.cpu_dump_mode = CPU_DUMP_SIMPLE,
		.mode_string = "simple"
	},
	(cpu_dump_t){
		.cpu_dump_mode = CPU_DUMP_ONELINE,
		.mode_string = "oneline"
	},
	(cpu_dump_t){
		.cpu_dump_mode = CPU_DUMP_PRETTY,
		.mode_string = "pretty"
	},
	(cpu_dump_t){
		.cpu_dump_mode = CPU_DUMP_NONE,
		.mode_string = "none"
	}
};

static ssize_t get_cpu_dump_data_size(void) {
	static ssize_t cached = 0;

	if (!cached)
		cached = sizeof(cpu_dump_data) / sizeof(cpu_dump_t);

	return cached;
}

cpu_dump_mode_t parse_cpu_dump_mode(const char* arg) {
	unsigned int i;

	i = 0;

	while (i < get_cpu_dump_data_size()) {
		if (!strcmp(cpu_dump_data[i].mode_string, arg))
			return cpu_dump_data[i].cpu_dump_mode;
		i++;
	}

	return CPU_DUMP_NONE;
}

static char* get_cpu_dump_usage(void) {
	unsigned int i;
	ssize_t total_size;
	ssize_t curr_len;
	char* res;
	char* curr;

	i = 0;
	total_size = 1; /* account for first '[' */

	while (i < get_cpu_dump_data_size())
		total_size += strlen(cpu_dump_data[i++].mode_string) + 1;

	res = malloc(total_size + 1);
	res[total_size] = '\0';
	res[0] = '[';
	curr = &(res[1]);

	i = 0;

	while (i < get_cpu_dump_data_size()) {
		curr_len = strlen(cpu_dump_data[i].mode_string);
		strncpy(curr, cpu_dump_data[i].mode_string, curr_len);
		curr[curr_len] = i == (get_cpu_dump_data_size() - 1)
				 ? ']' : '|';
		curr += curr_len + 1;
		i++;
	}

	return res;
}

char* get_cpu_dump_help(const char* fmt) {
	char* dump_usage;
	char* res;

	dump_usage = get_cpu_dump_usage();

	res = malloc(strlen(dump_usage) + strlen(fmt) + 1);

	sprintf(res, fmt, dump_usage);

	free(dump_usage);

	return res;
}

void dump_cpu(struct cpu_6502_t* cpu, cpu_dump_mode_t mode) {
	char sep;

	sep = '\t';

	switch (mode) {
	case CPU_DUMP_NONE:
		break;

	case CPU_DUMP_SIMPLE:
		printf("%.2x %.2x %.2x %.2x %.2x %.4x\n",
		       cpu->A, cpu->X, cpu->Y,
		       cpu->S, cpu->P, cpu->PC);
		break;

	case CPU_DUMP_ONELINE:
		sep = ' ';

	case CPU_DUMP_PRETTY:
		printf("A: %.2x%cX: %.2x%cY: %.2x%c",
		       cpu->A, sep, cpu->X, sep, cpu->Y,
		       mode == CPU_DUMP_PRETTY ? '\n' : ' ');
		printf("S: %.2x%cP: %.2x%cPC: %.4x\n",
		       cpu->S, sep, cpu->P, sep, cpu->PC);
		break;

	default:
		log_err("CPU", "Invalid mode (%d) in dump_cpu!", mode);
		break;
	}

	return;
}
