#include <stdio.h>
#include <unistd.h> /* getopt */
#include <stdbool.h>

#include "translator.h"
#include "device.h"

extern void init_cpu_6502_actions(void);

/* ======= run device ======= */

static int main_run_device(unsigned int len, const uint8_t* out, void* data) {
	struct device_t device;
	struct cpu_6502_t cpu;
	int ret;

	ret = 0;

	printf("(i) Initializing CPU 6502 actions.\n");

	init_cpu_6502_actions();

	printf("(i) Initializing device.\n");

	init_cpu(&cpu, get_instr_list());
	init_device(&device, &cpu, 0x0600, 0x0, 256);

	ret = load_to_ram(&device, 0x0600, out, len);
	if (ret)
		return ret;

	run_device(&device);

	return ret;
}

static int run_action(const char* infile) {

	return translate(infile, 0x0600, 0x0, 256, main_run_device, NULL);
}

/* ======= debug ======= */

struct translation_data_t {
	const char *infile;
	const char *outfile;
};

static int print_binary(unsigned int len, const uint8_t* out, void* data) {
	struct translation_data_t* td;
	unsigned int i, j, cols, rows, end;

	if (data) {
		td = (struct translation_data_t*)data;
		printf("(i) Dumping binary translated from %s\n", td->infile);
	}

	cols = 5;
	rows = len / cols;

	for (i = 0; i < rows + (len % cols ? 1 : 0); i++) {
		end = (i + 1) * cols < len ? (i + 1) * cols : len;

		printf("%.4x: ", i * cols);

		for (j = i * cols; j < end; j++)
			printf("%.2x%s", out[j], j == end - 1 ? "" : " ");

		printf("\n");
	}

	return 0;
}

/* ======== translation ======= */

static int export_binary(unsigned int len, const uint8_t* out, void* data) {
	struct translation_data_t* td;
	int ret;
	FILE *f;

#ifdef TRANSLATOR_TRACE
	print_binary(len, out, data);
#endif

	ret = 0;
	td = (struct translation_data_t*)data;

	f = fopen(td->outfile, "w");
	if (!f) {
		printf("(!) Could not open %s for output.\n", td->outfile);

		return -1;
	}

	ret = fwrite(out, 1, len, f);
	if (ret != len) {
		printf("(!) Error writing data to %s.\n", td->outfile);

		return -1;
	}
	else
		printf("(i) Successfully written binary to %s.\n",
		       td->outfile);

	fclose(f);

	return 0;
}

static int translate_file(const char* infile, const char* outfile) {
	struct translation_data_t td;
	const char default_outfile[] = "a.out";

	td.infile = infile;
	td.outfile = outfile ?: default_outfile;

	return translate(td.infile, 0x0600, 0x0, 256, export_binary, &td);
}

typedef enum {
	MAIN_ACTION_TRANSLATE,
	MAIN_ACTION_RUN,
	MAIN_ACTION_NONE
} main_action_t;

int main(int argc, char* const argv[]) {
	int opt;
	char* infile;
	char* outfile;
	main_action_t action;

	infile = NULL;
	outfile = NULL;
	action = MAIN_ACTION_NONE;

	while ((opt = getopt(argc, argv, "r:t:o:h")) != -1) {

		switch (opt) {

		case 't':
			infile = optarg;
			action = MAIN_ACTION_TRANSLATE;

			break;
		case 'o':
			outfile = optarg;

			break;
		case 'h':
			printf("Example usage:\n\n\tsikso2 -t "
			       "test.asm -o test\n\n");

			return 0;
		case 'r':
			infile = optarg;
			action = MAIN_ACTION_RUN;

			break;
		default:
			printf("(!) Unknown stuff here (%d, %s).\n",
			       opt, optarg);

			return -1;
		}
	}

	if (!infile) {
		printf("(!) Please specify input file with "
		       "-t or -r switch.\n");

		return -1;
	}

	switch (action) {

	case MAIN_ACTION_TRANSLATE:

		return translate_file(infile, outfile);

	case MAIN_ACTION_RUN:

		return run_action(infile);
	}

	return -1;
}
