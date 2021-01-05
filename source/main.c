#include <stdio.h>
#include <unistd.h> /* getopt */
#include <getopt.h>
#include <stdbool.h>

#include "translator.h"
#include "device.h"

extern void init_cpu_6502_actions(void);

/* ======= run device ======= */

typedef struct {
	uint16_t load_addr;
	uint16_t zero_page;
	uint16_t page_size;
	bool end_on_final_instr;
} run_settings_t;

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

	run_device(&device, ((run_settings_t*)data)->end_on_final_instr);

	return ret;
}

static int run_action(const char* infile, run_settings_t* run_settings) {

	return translate(infile, 0x0600, 0x0, 256, main_run_device, (void*)run_settings);
}

/* ======= debug ======= */

struct translation_data_t {
	const char *infile;
	const char *outfile;
};

#ifdef TRANSLATOR_TRACE
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
#endif

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

static struct option long_options[] = {
	{ "run",	required_argument,	0, 'r' },
	{ "zero-page",	required_argument,	0, 'z' },
	{ "page-size",	required_argument,	0, 'p' },
	{ "load-addr",	required_argument,	0, 'a' },
	{ "stop",	no_argument,		0, 's' },
	{ "dump-cpu",	no_argument,		0, 'd' },
	{ "dump-mem",	required_argument,	0, 'm' },
	{ "translate",	required_argument,	0, 't' },
	{ "output",	required_argument,	0, 'o' },
	{ "help",	required_argument,	0, 'h' },
	{ 0,		0,			0,  0  }
};

int main(int argc, char* const argv[]) {
	int opt;
	char* infile;
	char* outfile;
	main_action_t action;
	run_settings_t run_settings;
	int option_index = 0;

	infile = NULL;
	outfile = NULL;
	action = MAIN_ACTION_NONE;

	run_settings.end_on_final_instr = false;

	while ((opt = getopt_long(argc, argv, "r:st:o:h", long_options, &option_index)) != -1) {

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
		case 's':
			run_settings.end_on_final_instr = true;

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

		return run_action(infile, &run_settings);

	case MAIN_ACTION_NONE:
		printf("Nothing to do.\n");
		break;
	}

	return -1;
}
