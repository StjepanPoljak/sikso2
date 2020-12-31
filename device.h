#ifndef DEVICE_H
#define DEVICE_H

#include "cpu.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t ram[65536];
	uint16_t zero_page;
	uint16_t page_size;
} ram_t;

typedef struct {
	opcode_t opc;
	uint16_t arg;
} instr_frag_t;

struct peripheral_t {
	uint16_t addr_start;
	uint16_t addr_end;
	ram_t* ram;
	bool(*read)(struct peripheral_t*, uint16_t, uint8_t*);
	bool(*write)(struct peripheral_t*, uint16_t, uint8_t);
};

struct device_t {
	cpu_6502_t* cpu;
	uint16_t load_addr;
	struct peripheral_t* peripherals;
	unsigned int num_of_peripherals;
	bool(*run_device)(struct device_t* device);
	struct cycle_node_t* cycle_head;
	struct cycle_node_t* cycle_last;
	void* data;
	ram_t ram;
};

typedef int(*cycle_action_t)(struct device_t*);

struct cycle_node_t {
	cycle_action_t action;
	struct cycle_node_t* next;
};

void init_device(struct device_t* device, struct cpu_6502_t* cpu,
		 uint16_t load_addr, uint16_t zero_page,
		 uint16_t page_size);

int load_to_ram(struct device_t* device, uint16_t load_addr,
		const uint8_t* data, unsigned int data_size);

int run_device(struct device_t* device);
void free_device(struct device_t* device);

#endif
