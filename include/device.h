#ifndef DEVICE_H
#define DEVICE_H

#include "cpu.h"

#include <stdint.h>
#include <stdbool.h>

#define DEVICE_TAKE_BRANCH 5
#define DEVICE_GENERATE_NMI 4
#define DEVICE_GENERATE_IRQ 3
#define DEVICE_NEED_EXTRA_CYCLE 2
#define DEVICE_NEED_FETCH 1
#define DEVICE_INVALID_ADDR -1
#define DEVICE_PERIPHERAL_WRITE_ERROR -2
#define DEVICE_PERIPHERAL_READ_ERROR -3
#define DEVICE_INSTRUCTION_ERROR -4
#define DEVICE_NO_CPU_ERROR -5
#define DEVICE_INTERNAL_BUG -6
#define DEVICE_NO_ACTION -7

typedef struct {
	uint8_t ram[65536];
	uint16_t zero_page;
	uint16_t page_size;
} ram_t;

typedef struct {
	opcode_t opc;
	uint16_t arg;
	bool pending;
	bool skiparg;
	bool extrcyc;
#ifdef DEVICE_TRACE
	uint8_t ncyc;
#endif
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
	instr_frag_t instr_frag;
	void* data;
	int(*read)(struct device_t*, uint16_t, uint8_t*);
	int(*write)(struct device_t*, uint16_t, uint8_t);
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

int run_device(struct device_t* device, bool end_on_last_instr);
void free_device(struct device_t* device);

#endif
