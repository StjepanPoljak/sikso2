#include "mem.h"

#include <string.h>

#include "common.h"
#include "device.h"

#define MSIG "MEM"

#define logm_err(FMT, ...) log_err(MSIG, FMT, ## __VA_ARGS__)

/* mem region */

void print_mem_region(struct device_t* device, struct mem_region_t* mr) {
	struct mem_region_t* curr;

	curr = mr;

	while (curr) {
		print_hex(&(device->ram.ram[curr->start_addr]),
			  curr->end_addr - curr->start_addr + 1,
			  curr->start_addr);
		curr = curr->next;
	}

	return;
}

void dump_mem(struct device_t* device, struct mem_region_t* mr) {

	print_mem_region(device, mr);

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

static int find_sep(const char* str, char sep) {
	int i = 0;

	if (!str)
		return -1;

	while (str[i] != '\0') {

		if (str[i] == sep)
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

	first = malloc(sep_loc + 1);
	strncpy(first, str, sep_loc);
	first[sep_loc] = '\0';

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

	if (mem->end_addr < mem->start_addr)
		return false;

	return ret >= 0;
}

struct mem_region_t* parse_mem_region(const char* str) {
	char* temp;
	char* ptr;
	char sep = ',';
	int sep_loc;
	int tmp_addr;
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

		if ((sep_loc = find_sep(ptr, '-')) < 0) {
 			tmp_addr = parse_str(ptr, 16, NULL);

			if (tmp_addr < 0)
				goto mem_region_error;

			mem_new->start_addr = (unsigned int)tmp_addr;
			mem_new->end_addr = mem_new->start_addr;
			mem_new->is_valid = true;
		}
		else {
			if (!separate(ptr, sep_loc, mem_new))
				goto mem_region_error;
		}

		if (!mem_head)
			mem_head = mem_new;
		else
			append_mem_region(mem_head, mem_new);

		ptr = strtok(NULL, &sep);
	}

	free(temp);

	return mem_head;

mem_region_error:

	logm_err("Invalid argument: %s.", ptr);

	free(temp);
	free(mem_new);
	free_mem_region_list(mem_head);

	return NULL;
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

static void on_err(int merrno) {

	logm_err("Error parsing argument.");
	return;
}

/* mem image */

mem_image_t* get_mem_image(const char* arg) {
	int ret;
	char* addr;
	uint8_t* out;
	const char* infile;
	mem_image_t* mimage;

	ret = find_sep(arg, ':');
	if (ret < 0 || strlen(arg) <= ret + 1) {
		logm_err("Invalid argument %s.", arg);
		return NULL;
	}

	addr = malloc(ret + 1);
	strncpy(addr, arg, ret);
	addr[ret] = '\0';

	infile = &(arg[ret + 1]);

	ret = parse_str(addr, 16, on_err);
	free(addr);

	if (ret < 0) {
		logm_err("Error parsing %s.", arg);
		return NULL;
	}

	mimage = malloc(sizeof(*mimage));
	if (!mimage) {
		logm_err("Could not allocate memory for mem_image.");
		return NULL;
	}

	mimage->addr = ret;

	out = load_file(infile, &mimage->length);
	if (!out) {
		logm_err("Could not load %s.", infile);
		free(mimage);
		return NULL;
	}

	mimage->contents = out;

	return mimage;
}

void free_mem_image(mem_image_t* mimage) {
	free(mimage->contents);
	free(mimage);
}

/* mem byte */

static void append_mem_byte(struct mem_byte_t* head,
			    struct mem_byte_t* new) {
	struct mem_byte_t* curr;

	curr = head;

	while (curr->next)
		curr = curr->next;

	curr->next = new;

	return;
}

static bool separate_mb(const char* ptr, int sep_loc, struct mem_byte_t* mem_new) {
	char* addr;
	const char* byte;
	int tmp;

	addr = malloc(sep_loc + 1);
	strncpy(addr, ptr, sep_loc);
	addr[sep_loc] = '\0';
	tmp = parse_str(addr, 16, on_err);
	free(addr);

	if (tmp < 0)
		return false;

	mem_new->addr = (unsigned int)tmp;

	byte = &(ptr[sep_loc + 1]);
	tmp = parse_str(byte, 16, on_err);

	if (tmp < 0)
		return false;

	mem_new->byte = tmp;

	return true;
}

struct mem_byte_t* parse_mem_bytes(const char* str) {
	char* temp;
	char* ptr;
	char sep = ',';
	int sep_loc;
	ssize_t strsize;
	struct mem_byte_t* mem_head;
	struct mem_byte_t* mem_new;

	mem_head = NULL;

	strsize = strlen(str) + 1;
	temp = malloc(strsize);
	strcpy(temp, str);
	temp[strsize - 1] = '\0';

	ptr = strtok(temp, &sep);

	while (ptr) {
		mem_new = malloc(sizeof(*mem_new));
		mem_new->next = NULL;

		if ((sep_loc = find_sep(ptr, ':')) >= 0) {
			if (!separate_mb(ptr, sep_loc, mem_new))
				goto mem_byte_error;
		}
		else
			goto mem_byte_error;

		if (!mem_head)
			mem_head = mem_new;
		else
			append_mem_byte(mem_head, mem_new);

		ptr = strtok(NULL, &sep);
	}

	free(temp);

	return mem_head;

mem_byte_error:

	logm_err("Invalid argument: %s.", ptr);

	free(temp);
	free(mem_new);
	free_mem_bytes(mem_head);

	return NULL;

}

static void free_mem_byte(struct mem_byte_t* mbyte) {
	struct mem_byte_t* curr;

	curr = mbyte;
	free(curr);

	return;
}

void free_mem_bytes(struct mem_byte_t* head) {
	struct mem_byte_t* curr;
	struct mem_byte_t* prev;

	curr = head;

	while (curr) {
		prev = curr;
		curr = curr->next;
		free_mem_byte(prev);
	}

	return;
}

int do_for_each_mem_byte(struct mem_byte_t* head,
			 int (*op)(struct mem_byte_t* curr, void* data),
			 void* data) {
	struct mem_byte_t* curr;
	int ret;

	ret = 0;
	curr = head;

	while (curr) {

		if ((ret = op(curr, data)))
			break;

		curr = curr->next;
	}

	return ret;
}
