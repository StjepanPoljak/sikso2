#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "cpu.h"

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
	uint16_t ram_size;
	uint8_t ram[65536];
	uint16_t end_instr;
} ram_t;

struct peripheral_t {
	uint16_t addr_start;
	uint16_t addr_end;
	uint8_t(*read)(struct peripheral_t*, uint16_t);
	void(*write)(struct peripheral_t*, uint16_t, uint8_t);
};

#define device_read(device, addr) \
	((addr) < device->ram.ram_size) ? device->ram.ram[(addr)] \
					: device->read(device, (addr))

#define device_write(device, addr, val) \
	if (addr < device->ram.ram_size) device->ram.ram[addr] = val; \
	else device->write(device, addr, val)

struct device_t {
	int error;
	cpu_6502_t* cpu;
	uint16_t load_addr;
	uint16_t stack_addr;
	struct peripheral_t* peripherals;
	unsigned int num_of_peripherals;
	bool(*run_device)(struct device_t* device);
	void* data;
	uint8_t(*read)(struct device_t*, uint16_t);
	int(*write)(struct device_t*, uint16_t, uint8_t);
	ram_t ram;
};

void init_device(struct device_t* device, struct cpu_6502_t* cpu,
		 uint16_t load_addr, uint16_t stack_addr,
		 uint16_t ram_size);

int load_to_ram(struct device_t* device, uint16_t load_addr,
		const uint8_t* data, unsigned int data_size,
		bool binary);
int run_device(struct device_t* device,
	       bool end_on_last_instr,
	       cpu_dump_mode_t cpu_dump_mode,
	       struct mem_region_t* mrhead);
void free_device(struct device_t* device);

#endif
