#include <stdio.h>
#include <unistd.h> /* getopt */
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#include "translator.h"
#include "device.h"
#include "common.h"

#define MSIG "MAI"

#define logm_err(FMT, ...) log_err(MSIG, FMT, ## __VA_ARGS__)

#ifdef MAIN_TRACE
#define mtrace(FMT, ...) trace(FMT, ## __VA_ARGS__)
#define mtracei(FMT, ...) tracei(MSIG, FMT, ## __VA_ARGS__)
#else
#define mtrace(FMT, ...) ;
#define mtracei(FMT, ...) ;
#endif

extern void init_cpu_6502_actions(void);

/* ======= run device ======= */

typedef struct {
	int32_t load_addr;
	int32_t zero_page;
	int32_t page_size;
	bool end_on_final_instr;
} settings_t;

#define get_load_addr(settings) \
	settings->load_addr < 0 ? DEFAULT_LOAD_ADDR \
				: (uint16_t)settings->load_addr

#define get_zero_page(settings) \
	settings->zero_page < 0 ? DEFAULT_ZERO_PAGE \
				: (uint16_t)settings->zero_page

#define get_page_size(settings) \
	settings->page_size < 0 ? DEFAULT_PAGE_SIZE \
				: (uint16_t)settings->page_size

static int main_run_device(unsigned int len, const uint8_t* out, void* data) {
	struct device_t device;
	struct cpu_6502_t cpu;
	int ret;

	ret = 0;

	mtracei("Initializing CPU 6502 actions.");

	init_cpu_6502_actions();

	mtracei("Initializing device.");

	init_cpu(&cpu, get_instr_list());
	init_device(&device, &cpu, get_load_addr(((settings_t*)data)),
		    get_zero_page(((settings_t*)data)),
		    get_page_size(((settings_t*)data)));

	ret = load_to_ram(&device, get_load_addr(((settings_t*)data)),
			  out, len);
	if (ret)
		return ret;

	run_device(&device, ((settings_t*)data)->end_on_final_instr);

	return ret;
}

static int run_action(const char* infile, settings_t* settings) {

	return translate(infile, get_load_addr(settings),
			 get_zero_page(settings),
			 get_page_size(settings),
			 main_run_device, (void*)settings);
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
		mtracei("Dumping binary translated from %s", td->infile);
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
		logm_err("Could not open %s for output.", td->outfile);

		return -1;
	}

	ret = fwrite(out, 1, len, f);
	if (ret != len) {
		logm_err("Error writing data to %s.", td->outfile);

		return -1;
	}
	else
		mtracei("(i) Successfully written binary to %s.",
			td->outfile);

	fclose(f);

	return 0;
}

static int translate_file(const char* infile, const char* outfile,
			  settings_t* settings) {
	struct translation_data_t td;
	const char default_outfile[] = "a.out";

	td.infile = infile;
	td.outfile = outfile ?: default_outfile;

	return translate(td.infile, get_load_addr(settings),
			 get_zero_page(settings),
			 get_page_size(settings),
			 export_binary, &td);
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

static bool is_hex(const char* str) {

	if (strlen(str) < 2)
		return false;

	if ((str[0] == '0') && (str[1] == 'x'))
		return true;

	return false;
}

static void on_err(int errno) {

	logm_err("Could not parse argument (%s)", strerror(errno));

	return;
}

static int parse_arg(const char* str) {

	return parse_str(str, is_hex(str) ? 16 : 10, on_err);
}

int main(int argc, char* const argv[]) {
	int opt;
	char* infile;
	char* outfile;
	main_action_t action;
	settings_t settings;
	int option_index = 0;

	infile = NULL;
	outfile = NULL;
	action = MAIN_ACTION_NONE;

	settings.load_addr = -1;
	settings.zero_page = -1;
	settings.page_size = -1;
	settings.end_on_final_instr = false;

	while ((opt = getopt_long(argc, argv, "r:z:p:a:sdm:t:o:h",
				  long_options, &option_index)) != -1) {

		switch (opt) {

		case 'r':
			infile = optarg;
			action = MAIN_ACTION_RUN;
			break;

		case 'z':
			settings.zero_page = (uint16_t)parse_arg(optarg);
			break;

		case 'p':
			settings.page_size = (uint16_t)parse_arg(optarg);
			break;

		case 'a':
			settings.load_addr = (uint16_t)parse_arg(optarg);
			break;

		case 's':
			settings.end_on_final_instr = true;
			break;

		case 'd':
			logm_err("Not implemented");
			return -1;

		case 'm':
			logm_err("Not implemented");
			return -1;

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


		default:
			logm_err("Unknown stuff here (%d, %s).",
				 opt, optarg);

			return -1;
		}
	}

	if (!infile) {
		logm_err("Please specify input file with "
			 "-t or -r switch.");

		return -1;
	}

	switch (action) {

	case MAIN_ACTION_TRANSLATE:

		return translate_file(infile, outfile, &settings);

	case MAIN_ACTION_RUN:

		return run_action(infile, &settings);

	case MAIN_ACTION_NONE:
		logm_err("Nothing to do.");
		break;
	}

	return -1;
}
