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

typedef struct {
	unsigned int addr;
	uint8_t* contents;
	unsigned int length;
} mem_image_t;

mem_image_t* get_mem_image(const char* arg);
void free_mem_image(mem_image_t* mimage);

struct mem_byte_t {
	unsigned int addr;
	uint8_t byte;
	struct mem_byte_t* next;
};

struct mem_byte_t* parse_mem_bytes(const char* arg);
void free_mem_bytes(struct mem_byte_t* head);
int do_for_each_mem_byte(struct mem_byte_t* head,
			 int (*op)(struct mem_byte_t* curr, void* data),
			 void* data);

#endif
