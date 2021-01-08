#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stdbool.h>

struct mem_region_t {
	unsigned int start_addr;
	unsigned int end_addr;
	bool is_valid;
	struct mem_region_t* next;
};

struct mem_region_t* parse_mem_region(const char* str);
void free_mem_region_list(struct mem_region_t* head);

#endif
