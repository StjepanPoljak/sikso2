#include "device.h"

#include "instr.h"
#include "cpu.h"

#include <string.h> /* memcpy */

#define logd_err(FMT, ...) log_err("[DEV]", FMT, ## __VA_ARGS__)

#ifdef DEVICE_TRACE
#define dtrace(FMT, ...) printf("  -> " FMT "\n", ## __VA_ARGS__)
#define dtracei(FMT, ...) printf("[DEV] (i) " FMT "\n", ## __VA_ARGS__)
#else
#define dtrace(FMT, ...) ;
#define dtracei(FMT, ...) ;
#endif

#define DEVICE_NEED_FETCH 1
#define DEVICE_INVALID_ADDR -1
#define DEVICE_PERIPHERAL_WRITE_ERROR -2
#define DEVICE_PERIPHERAL_READ_ERROR -3
#define DEVICE_INSTRUCTION_ERROR -4
#define DEVICE_NO_CPU_ERROR -5
#define DEVICE_INTERNAL_BUG -6
#define DEVICE_NO_ACTION -7

void init_device(struct device_t* device, struct cpu_6502_t* cpu,
		 uint16_t load_addr, uint16_t zero_page,
		 uint16_t page_size) {

	device->cpu = cpu;
	device->load_addr = load_addr;
	device->peripherals = NULL;
	device->num_of_peripherals = 0;
	device->cycle_head = NULL;
	device->run_device = NULL;
	device->instr_frag.pending = false;
	device->data = NULL;
	device->ram.zero_page = zero_page;
	device->ram.page_size = page_size;

	for (unsigned int i = zero_page; i < 65526 - zero_page; i++)
		device->ram.ram[i] = 0x87;

	return;
}

static bool in_peripheral_addr(struct peripheral_t* peripheral, uint16_t addr) {

	return (addr >= peripheral->addr_start)
	    && (addr <= peripheral->addr_end);
}

static int write_byte(struct device_t* device, uint16_t addr, uint8_t byte) {
	unsigned int i;
	int ret;
	struct peripheral_t* currp;

	if (addr < device->ram.zero_page)
		return DEVICE_INVALID_ADDR;

	for (i = 0; i < device->num_of_peripherals; i++) {

		currp = &(device->peripherals[i]);

		if (in_peripheral_addr(currp, addr)) {
			ret = currp->write(currp, addr, byte);

			return ret ? 0 : DEVICE_PERIPHERAL_WRITE_ERROR;
		}
	}

	device->ram.ram[addr] = byte;

	return 0;
}

static int read_byte(struct device_t* device, uint16_t addr, uint8_t* byte) {
	unsigned int i;
	int ret;
	struct peripheral_t* currp;

	if (addr < device->ram.zero_page)
		return DEVICE_INVALID_ADDR;

	for (i = 0; i < device->num_of_peripherals; i++) {

		currp = &(device->peripherals[i]);

		if (in_peripheral_addr(currp, addr)) {
			ret = currp->read(currp, addr, byte);

			return ret ? 0 : DEVICE_PERIPHERAL_READ_ERROR;
		}
	}

	*byte = device->ram.ram[addr];

	return 0;
}

static int fetch(struct device_t* device, uint8_t* byte) {

	return read_byte(device, ((device->cpu)->PC)++, byte);
}

int load_to_ram(struct device_t* device, uint16_t load_addr,
		const uint8_t* data, unsigned int data_size) {
	unsigned int i;
	int ret;

	for (i = 0; i < data_size; i++) {
		ret = write_byte(device, (uint16_t)i + load_addr, data[i]);
		if (ret) {
			logd_err("Error loading data to RAM (addr: %.4x).",
				 (uint16_t)i + load_addr);
			return ret;
		}
	}

	return 0;
}

static void push_cycle(struct device_t* device, cycle_action_t action) {
	struct cycle_node_t* curr;
	struct cycle_node_t* newc;

	newc = malloc(sizeof(*newc));
	newc->action = action;

	curr = device->cycle_last;

	if (!curr) {
		device->cycle_last = newc;
		device->cycle_last->next = NULL;
		device->cycle_head = device->cycle_last;
	}
	else {
		newc->next = NULL;
		device->cycle_last->next = newc;
		device->cycle_last = newc;
	}

	return;
}

static int get_cycle(struct device_t* device) {
	struct cycle_node_t* head;
	int ret;

	ret = 0;

	if (!device->cycle_head)
		return DEVICE_NEED_FETCH;

	ret = device->cycle_head->action(device);
	if (ret < 0)
		return ret;

	head = device->cycle_head;

	if (device->cycle_last == device->cycle_head) {
		device->cycle_head = NULL;
		device->cycle_last = NULL;
	}
	else
		device->cycle_head = device->cycle_head->next;

	free(head);

	return device->cycle_head ? 0 : DEVICE_NEED_FETCH;
}

static void free_cycle(struct cycle_node_t* cycle) {
	struct cycle_node_t* curr;

	dtrace("Freeing cycle %p", cycle);

	curr = cycle;
	free(curr);

	return;
}

int fetch_arg(struct device_t* device) {
	uint8_t byte;
	int ret;
#ifdef DEVICE_TRACE
	uint16_t address;
#endif

	address = device->cpu->PC;
	ret = fetch(device, &byte);

	if (!device->instr_frag.pending) {
		logd_err("No instruction pending while fetching arg.");

		return DEVICE_INTERNAL_BUG;
	}

	device->instr_frag.arg <<= 8;
	device->instr_frag.arg |= byte;

#ifdef DEVICE_TRACE
	dtracei("Fetching arg fragment at %.4x", address);
	dtrace("fragment = %.2x", byte);
#endif

	return ret;
}

int execute(struct device_t* device) {
	action_t action;
	instr_map_t* map;
	int ret;

	map = &(device->cpu->instr_map[device->instr_frag.opc]);
	action = map->instr->action;

	ret = fetch_arg(device);

	if (ret)
		return ret;

	dtracei("Executing %.2x (%x)", device->instr_frag.opc,
		device->instr_frag.arg);

	if (!action) {
		logd_err("No action for opcode %.2x", device->instr_frag.opc);

		return DEVICE_NO_ACTION;
	}

	ret = action(map->subinstr, device->instr_frag.arg, (void*)device);

	device->instr_frag.pending = false;

#ifdef CPU_TRACE
	dump_cpu(device->cpu);
#endif

	return ret;
}


int nop(struct device_t* device) {

	(void)device;

	device->cpu->PC++;

	dtracei("NOP");

	return 0;
}

#define instr_length(device, opc) \
	(device)->cpu->instr_map[opc].subinstr->length

#define instr_cycles(device, opc) \
	(device)->cpu->instr_map[opc].subinstr->cycles

#define instr_mode(device, opc) \
	((device)->cpu->instr_map[opc].subinstr->mode & 0xFF)

int fetch_op(struct device_t* device) {
	uint8_t byte;
	int ret;
	unsigned int i;
#ifdef DEVICE_TRACE
	uint16_t address;
	char name[4] = { 0 };
#endif

	address = device->cpu->PC;
	ret = fetch(device, &byte);

#ifdef DEVICE_TRACE
	memcpy(name, device->cpu->instr_map[byte].instr->name, 3);
	dtracei("Fetching instruction at %.4x", address);
	dtrace("opcode: %.2x", byte);
	dtrace("name: %s", name);
	dtrace("length: %d", instr_length(device, byte));
	dtrace("cycles: %d", instr_cycles(device, byte));
#endif

	if (!device->cpu->instr_map[byte].instr) {
		logd_err("Unknown instruction fetched.");

		return DEVICE_INSTRUCTION_ERROR;
	}

	if (device->instr_frag.pending) {
		logd_err("Instruction pending while fetching.");

		return DEVICE_INTERNAL_BUG;
	}

	device->instr_frag.pending = true;
	device->instr_frag.opc = (opcode_t)byte;
	device->instr_frag.arg = 0x0;

	for (i = 1; i < instr_length(device, byte) - 1; i++)
		push_cycle(device, fetch_arg);

	for (i = instr_length(device, byte);
	     i < instr_cycles(device, byte) - 1; i++)
		push_cycle(device, nop);

	push_cycle(device, execute);

	return ret;
}

int run_device(struct device_t* device) {
	int ret;
#ifdef DEVICE_SAFEGUARD
	int safeguard;
#endif

	ret = 0;

#ifdef DEVICE_SAFEGUARD
	safeguard = DEVICE_SAFEGUARD;
#endif
	if (!device->cpu) {
		logd_err("Please plug CPU into device.");

		return DEVICE_NO_CPU_ERROR;
	}

	start_cpu(device->cpu, device->ram.zero_page,
		  device->ram.page_size, device->load_addr);

	while (ret >= 0) {
		ret = get_cycle(device);

		if (ret == DEVICE_NEED_FETCH)
			push_cycle(device, fetch_op);

#ifdef DEVICE_SAFEGUARD
		if (!(--safeguard))
			break;
#endif
	}

	if (ret < 0)
		logd_err("Cycle execution returned %d", ret);
	else
		dtracei("Return value: %d", ret);

	free_device(device);

	return ret;
}

void free_device(struct device_t* device) {
	struct cycle_node_t* curr;

	dtracei("Freeing device...");

	while ((curr = device->cycle_head)) {
		device->cycle_head = curr->next;
		free_cycle(curr);
	}

	return;
}
