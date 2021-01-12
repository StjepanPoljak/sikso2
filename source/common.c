#include "common.h"

#include <stdlib.h> /* strtol */
#include <errno.h>
#include <string.h>

#include "device.h"

void print_hex(const uint8_t* bin, unsigned int len, unsigned int offset) {
	unsigned int i, j, cols, rows, end;

	cols = DEFAULT_DUMP_MEM_COLS;
	rows = len / cols;

	for (i = 0; i < rows + (len % cols ? 1 : 0); i++) {
		end = (i + 1) * cols < len ? (i + 1) * cols : len;

		printf("%.4x: ", i * cols + offset);

		for (j = i * cols; j < end; j++)
			printf("%.2x%s", bin[j], j == end - 1 ? "" : " ");

		printf("\n");
	}

	return;
}

void init_settings(settings_t* settings) {

	settings->load_addr = -1;
	settings->zero_page = -1;
	settings->page_size = -1;
	settings->end_on_final_instr = false;
	settings->cpu_dump_mode = CPU_DUMP_NONE;
	settings->mrhead = NULL;
	settings->mimage = NULL;
	settings->mbhead = NULL;

	return;
}

void free_settings(settings_t* settings) {

	if (settings->mrhead)
		free_mem_region_list(settings->mrhead);

	if (settings->mimage)
		free_mem_image(settings->mimage);

	if (settings->mbhead)
		free_mem_bytes(settings->mbhead);

	return;
}

bool is_hex(const char* str) {

	if (strlen(str) < 2)
		return false;

	if ((str[0] == '0') && (str[1] == 'x'))
		return true;

	return false;
}

int parse_str(const char* str, int base, void(*on_err)(int)) {
	int ret;
	char *endptr;

	if (!strlen(str))
		return -1;

	errno = 0;
	ret = (int)strtol(str, &endptr, base);

	if (errno && on_err) {
		on_err(errno);

		return -2;
	}

	if (*endptr != '\0')
		return -3;

	return ret;
}

uint8_t* load_file(const char* infile, unsigned int* len) {
	uint8_t* out;
	long fsize;
	FILE* f;

	f = fopen(infile, "r");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);

	fsize = ftell(f);

	out = malloc(fsize);
	if (!out)
		return NULL;

	*len = fsize;

	fseek(f, 0, SEEK_SET);

	if (fread(out, 1, fsize, f) != fsize) {
		free(out);
		return NULL;
	}

	fclose(f);

	return out;
}


