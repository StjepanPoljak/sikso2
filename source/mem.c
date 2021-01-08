#include "mem.h"

#include <string.h>

#include "common.h"
#include "device.h"

void print_mem_region(struct device_t* device, struct mem_region_t* mr) {

	print_hex(&(device->ram.ram[mr->start_addr]),
		  mr->end_addr - mr->start_addr + 1,
		  mr->start_addr);

	return;
}

void dump_mem(struct device_t* device, struct mem_region_t* mr) {

	print_mem_region(device, mr);
	free_mem_region_list(mr);

	return;
}


static void init_mem_region(struct mem_region_t* mem) {

	mem->is_valid = false;
	mem->next = NULL;

	return;
}

static void append_mem_region(struct mem_region_t* head,
			      struct mem_region_t* new) {
	struct mem_region_t* curr;

	curr = head;

	while (curr->next)
		curr = curr->next;

	curr->next = new;

	return;
}

static int find_sep(const char* str) {
	int i = 0;

	if (!str)
		return -1;

	while (str[i] != '\0') {

		if (str[i] == '-')
			return i;
		i++;
	}

	return -1;
}

static bool separate(const char* str, int sep_loc, struct mem_region_t* mem) {
	int ret;
	char* first;
	char* second;
	ssize_t strsize;

	first = NULL;
	second = NULL;
	ret = 0;
	strsize = 0;

	first = malloc(sep_loc + 2);
	strncpy(first, str, sep_loc + 1);
	first[sep_loc + 1] = '\0';

	ret = parse_str(first, 16, NULL);
	if (ret < 0)
		goto exit_separate;

	mem->start_addr = ret;

	strsize = strlen(str + sep_loc + 1);
	if (!strsize) {
		ret = -1;
		goto exit_separate;
	}

	second = malloc(strsize + 1);
	strncpy(second, str + sep_loc + 1, strsize);
	second[strsize] = '\0';

	ret = parse_str(second, 16, NULL);
	if (ret < 0)
		goto exit_separate;

	mem->end_addr = ret;
	mem->is_valid = true;

exit_separate:

	if (first)
		free(first);

	if (second)
		free(second);

	return ret >= 0;
}

struct mem_region_t* parse_mem_region(const char* str) {
	char* temp;
	char* ptr;
	char sep = ',';
	int sep_loc;
	ssize_t strsize;
	struct mem_region_t* mem_head;
	struct mem_region_t* mem_new;

	mem_head = NULL;

	strsize = strlen(str) + 1;
	temp = malloc(strsize);
	strcpy(temp, str);
	temp[strsize - 1] = '\0';

	ptr = strtok(temp, &sep);

	while (ptr) {

		mem_new = malloc(sizeof(*mem_new));
		init_mem_region(mem_new);

		if ((sep_loc = find_sep(ptr)) < 0) {
			if (!ptr)
				break;
			mem_new->start_addr = parse_str(ptr, 16, NULL);
			mem_new->end_addr = mem_new->start_addr;
			mem_new->is_valid = true;
		}
		else {
			separate(ptr, sep_loc, mem_new);
		}

		if (!mem_head)
			mem_head = mem_new;
		else
			append_mem_region(mem_head, mem_new);

		ptr = strtok(NULL, &sep);
	}

	free(temp);

	return mem_head;
}

static void free_mem_region(struct mem_region_t* mem) {
	struct mem_region_t* temp;

	temp = mem;
	free(temp);

	return;
}

void free_mem_region_list(struct mem_region_t* head) {
	struct mem_region_t* curr;
	struct mem_region_t* prev;

	curr = head;

	while (curr) {
		prev = curr;
		curr = curr->next;
		free_mem_region(prev);
	}

	return;
}
