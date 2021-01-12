#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "mem.h"

#define log_err(SIG, FMT, ...) \
	printf("[" SIG "] (!) " FMT "\n", ## __VA_ARGS__)

#define trace(FMT, ...) \
	printf("  -> " FMT "\n", ## __VA_ARGS__)

#define tracei(SIG, FMT, ...) \
	printf("[" SIG "] (i) " FMT "\n", ## __VA_ARGS__)

#define get_load_addr(settings) \
	settings->load_addr < 0 ? DEFAULT_LOAD_ADDR \
				: (uint16_t)settings->load_addr

#define get_zero_page(settings) \
	settings->zero_page < 0 ? DEFAULT_ZERO_PAGE \
				: (uint16_t)settings->zero_page

#define get_page_size(settings) \
	settings->page_size < 0 ? DEFAULT_PAGE_SIZE \
				: (uint16_t)settings->page_size

typedef struct {
	int32_t load_addr;
	int32_t zero_page;
	int32_t page_size;
	bool end_on_final_instr;
	cpu_dump_mode_t cpu_dump_mode;
	struct mem_region_t* mrhead;
	mem_image_t* mimage;
	struct mem_byte_t* mbhead;
} settings_t;

void init_settings(settings_t* settings);
void free_settings(settings_t* settings);
bool is_hex(const char* str);
void print_hex(const uint8_t* mem, unsigned int len, unsigned int offset);
int parse_str(const char* str, int base, void(*on_err)(int));
uint8_t* load_file(const char* infile, unsigned int* len);

#endif
