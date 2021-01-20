#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "mem.h"
#include "translator.h"

#define log_err(SIG, FMT, ...) \
	printf("[" SIG "] (!) " FMT "\n", ## __VA_ARGS__)

#define trace(FMT, ...) \
	printf("  -> " FMT "\n", ## __VA_ARGS__)

#define tracei(SIG, FMT, ...) \
	printf("[" SIG "] (i) " FMT "\n", ## __VA_ARGS__)

#define get_load_addr(settings) \
	settings->load_addr < 0 ? DEFAULT_LOAD_ADDR \
				: (uint16_t)settings->load_addr

#define get_ram_size(settings) \
	settings->ram_size < 0 ? DEFAULT_RAM_SIZE \
			       : (uint16_t)settings->ram_size

#define get_stack_addr(settings) \
	settings->stack_addr < 0 ? DEFAULT_STACK_ADDR \
				 : (uint16_t)settings->stack_addr

typedef struct {
	int32_t load_addr;
	int32_t stack_addr;
	int32_t ram_size;
	bool end_on_final_instr;
	cpu_dump_mode_t cpu_dump_mode;
	struct mem_region_t* mrhead;
	mem_image_t* mimage;
	struct mem_byte_t* mbhead;
	disasm_mode_t dmode;
} settings_t;

#define print_to_str(PTR, SIZE, FMT, ...) do { \
	*SIZE = snprintf(NULL, 0, FMT, ## __VA_ARGS__) + 1; \
	if (!(*PTR)) *PTR = malloc(sizeof(**PTR) * (*SIZE)); \
	else *PTR = realloc(*PTR, sizeof(**PTR) * (*SIZE)); \
	*SIZE = snprintf(*PTR, *SIZE, FMT, ## __VA_ARGS__); \
} while (0)

void init_settings(settings_t* settings);
void free_settings(settings_t* settings);
bool is_hex(const char* str);
void print_hex(const uint8_t* mem, unsigned int len, unsigned int offset);
int parse_str(const char* str, int base, void(*on_err)(int));
uint8_t* load_file(const char* infile, unsigned int* len);
int appendc(char** str, int* last, int* size, char c);


#endif
