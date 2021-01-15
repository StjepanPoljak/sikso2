#include "device.h"

#include "instr.h"
#include "cpu.h"
#include "mem.h"
#include "common.h"

#include <string.h> /* memcpy */

#include <time.h>

#define DSIG "DEV"

#define logd_err(FMT, ...) log_err(DSIG, FMT, ## __VA_ARGS__)

#ifdef DEVICE_TRACE
#define dtrace(FMT, ...) trace(FMT, ## __VA_ARGS__)
#define dtracei(FMT, ...) tracei(DSIG, FMT, ## __VA_ARGS__)
#else
#define dtrace(FMT, ...) ;
#define dtracei(FMT, ...) ;
#endif

extern void print_mem_region(struct device_t* device, struct mem_region_t* mr);
extern void dump_mem(struct device_t* device, struct mem_region_t* mr);

void fill_ram(struct device_t* device, uint8_t byte) {
	unsigned int i;

	for (i = 0; i < device->ram.ram_size; i++)
		device->ram.ram[i] = byte;

	return;
}

void init_device(struct device_t* device, struct cpu_6502_t* cpu,
		 uint16_t load_addr, uint16_t stack_addr, uint16_t ram_size) {

	device->cpu = cpu;
	device->load_addr = load_addr;
	device->stack_addr = stack_addr;
	device->peripherals = NULL;
	device->num_of_peripherals = 0;
	device->run_device = NULL;
	device->data = NULL;
	device->read = NULL;
	device->write = NULL;
	device->error = 0;

	device->ram.ram_size = ram_size;

	return;
}

int load_to_ram(struct device_t* device, uint16_t load_addr,
		const uint8_t* data, unsigned int data_size,
		bool binary) {
	unsigned int i;

	for (i = 0; i < data_size; i++) {
		device_write(device, (uint16_t)i + load_addr, data[i]);
		if (device->error) {
			logd_err("Error loading data to RAM (addr: %.4x).",
				 (uint16_t)i + load_addr);
			return device->error;
		}
	}

	if (binary)
		device->ram.end_instr = (uint16_t)data_size + load_addr;

	return 0;
}

#define instr_length(device, opc) \
	(device)->cpu->instr_map[opc].subinstr->length

#define instr_cycles(device, opc) \
	(device)->cpu->instr_map[opc].subinstr->cycles

#define instr_mode(device, opc) \
	((device)->cpu->instr_map[opc].subinstr->mode & 0xFF)

#define run_action(device, opc, arg, data) \
	device->cpu->instr_map[opc].instr->action( \
		device->cpu->instr_map[opc].subinstr, arg, data)

typedef union {
	uint16_t arg;
	uint8_t mem[2];
} arg_conv_t;

int run_device(struct device_t* device,
	       bool end_on_last_instr,
	       cpu_dump_mode_t cpu_dump_mode,
	       struct mem_region_t* mrhead) {
	int ret;
	uint16_t arg;
	uint8_t byte;
#ifdef DEVICE_TRACE
	char name[4] = { 0 };
#endif
#ifdef DEVICE_SAFEGUARD
	int safeguard;
#endif

#ifdef CLOCK_TRACE
	struct timespec cycle_start;
	struct timespec cycle_end;
#endif

	ret = 0;

	if (end_on_last_instr)
		dtracei("Ending on %.4x", device->ram.end_instr);

#ifdef DEVICE_SAFEGUARD
	safeguard = DEVICE_SAFEGUARD;
#endif
	if (!device->cpu) {
		logd_err("Please plug CPU into device.");

		return DEVICE_NO_CPU_ERROR;
	}

	start_cpu(device->cpu, device->load_addr, device->stack_addr);

	while (true) {
#ifdef CLOCK_TRACE
		timespec_get(&cycle_start, TIME_UTC);
#endif

		if ((end_on_last_instr)
		 && (device->cpu->PC >= device->ram.end_instr)) {
			dtracei("Reached last instruction "
				"(PC=%.4x)", device->cpu->PC);
			break;
		}

#ifdef DEVICE_SAFEGUARD
		if (!(--safeguard)) {
			dtracei("Reached safeguard (%d cycles)!",
				DEVICE_SAFEGUARD);
			break;
		}
#endif
		byte = device->ram.ram[device->cpu->PC++];

#ifdef DEVICE_TRACE
		memcpy(name, device->cpu->instr_map[byte].instr->name, 3);
		dtracei("Fetching instruction at %.4x", device->cpu->PC - 1);
		dtrace("opcode: %.2x", byte);
		dtrace("name: %s", name);
		dtrace("length: %d", instr_length(device, byte));
		dtrace("cycles: %d", instr_cycles(device, byte));
#endif

		switch (instr_length(device, byte)) {
		case 1:
			arg = 0;
			break;
		case 2:
			arg = (uint16_t)device->ram.ram[device->cpu->PC++];
			break;
		case 3:
			arg = ((arg_conv_t*)
			     (&(device->ram.ram[device->cpu->PC])))->arg;
			device->cpu->PC += 2;
			break;
		default:
			break;
		}

		ret = run_action(device, (opcode_t)byte, arg, (void*)device);
		if (ret == DEVICE_NEED_EXTRA_CYCLE) {
			printf("Need extra");
		}

#ifdef CLOCK_TRACE
		timespec_get(&cycle_end, TIME_UTC);
#endif

#ifdef CLOCK_TRACE
		printf("Cycle lasted %ldns.\n",
		       cycle_end.tv_nsec - cycle_start.tv_nsec);
#endif
	}

	if (ret < 0)
		logd_err("Cycle execution returned %d", ret);
	else {
		dtracei("Return value: %d", ret);

		if (cpu_dump_mode != CPU_DUMP_NONE) {
			dtracei("Dumping CPU registers...");
			dump_cpu(device->cpu, cpu_dump_mode);
		}

		if (mrhead) {
			dtracei("Dumping memory...");
			dump_mem(device, mrhead);
		}
	}

	return ret;
}

