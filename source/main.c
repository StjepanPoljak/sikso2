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

	run_device(&device,
		   ((settings_t*)data)->end_on_final_instr,
		   ((settings_t*)data)->cpu_dump_mode,
		   ((settings_t*)data)->mrhead);

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

	if (data) {
		td = (struct translation_data_t*)data;
		mtracei("Dumping binary translated from %s", td->infile);
	}

	print_hex(out, len, 0);

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
	MAIN_ACTION_HELP,
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

static void print_option_help(struct option* opt, char* help_string) {
	char* long_txt;
	char short_txt[3];

	short_txt[0] = '-';
	short_txt[1] = opt->val;
	short_txt[2] = '\0';

	long_txt = malloc(strlen(opt->name) + 3);
	long_txt[0] = '-';
	long_txt[1] = '-';
	long_txt[strlen(opt->name) + 2] = '\0';
	strcpy(long_txt + 2, opt->name);

	printf("%1s %-6s %-18s %s\n", "", short_txt, long_txt, help_string);

	free(long_txt);

	return;
}

#define help_text(str) \
	print_option_help(&(long_options[i]), str)

#define TEX(A) #A

#define Z_HELP_STR(ZERO_PAGE) \
	"zero page (default: " TEX(ZERO_PAGE) ")"

#define P_HELP_STR(PAGE_SIZE) \
	"page size (default: " TEX(PAGE_SIZE) ")"

#define A_HELP_STR(LOAD_ADDR) \
	"load addr (default: " TEX(LOAD_ADDR) ")"

static void print_help(void) {
	unsigned int i;
	char* cpu_dump_help;

	i = 0;

	printf("\n\tsikso2, a 6502 emulator and translator\n\n");

	printf("Author:\tStjepan Poljak\n"
	       "E-Mail:\tstjepan.poljak@protonmail.com\n"
	       "Year:\t2021\n\n"
	       "Usage:\n");

	while (i < (sizeof(long_options) / sizeof(struct option) - 1)) {
		switch(long_options[i].val) {
		case 'r':
			help_text("run file");
			break;
		case 'z':
			help_text(Z_HELP_STR(DEFAULT_ZERO_PAGE));
			break;
		case 'p':
			help_text(P_HELP_STR(DEFAULT_PAGE_SIZE));
			break;
		case 'a':
			help_text(A_HELP_STR(DEFAULT_LOAD_ADDR));
			break;
		case 's':
			help_text("stop after last instruction");
			break;
		case 'd':
			cpu_dump_help = get_cpu_dump_help(
					"dump CPU registers: %s");
			help_text(cpu_dump_help);
			free(cpu_dump_help);
			break;
		case 'm':
			help_text("dump memory (e.g. 0x0600-0x060a,0x0700)");
			break;
		case 't':
			help_text("translate file to binary");
			break;
		case 'o':
			help_text("output file name (default: a.out)");
			break;
		case 'h':
			help_text("print help");
			break;
		default:
			break;
		}

		i++;
	}

	printf("\n");

	return;
}

static void on_err(int errno) {

	logm_err("Could not parse argument (%s)", strerror(errno));

	return;
}

static int parse_arg(const char* str) {

	return parse_str(str, is_hex(str) ? 16 : 10, on_err);
}

#define IMPROPER_USAGE \
	printf("Improper usage. Try -h for help.\n"); \
	return -1


int main(int argc, char* const argv[]) {
	int opt;
	char* infile;
	char* outfile;
	main_action_t action;
	settings_t settings;
	int option_index = 0;
	bool run_setting;
	bool tra_setting;

	infile = NULL;
	outfile = NULL;
	action = MAIN_ACTION_NONE;

	run_setting = false;
	tra_setting = false;

	init_settings(&settings);

	while ((opt = getopt_long(argc, argv, "r:z:p:a:sd:m:t:o:h",
				  long_options, &option_index)) != -1) {
		switch (opt) {

		case 'r':
			infile = optarg;
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_RUN;
			break;

		case 'z':
			settings.zero_page = (uint16_t)parse_arg(optarg);
			run_setting = true;
			break;

		case 'p':
			settings.page_size = (uint16_t)parse_arg(optarg);
			run_setting = true;
			break;

		case 'a':
			settings.load_addr = (uint16_t)parse_arg(optarg);
			run_setting = true;
			break;

		case 's':
			settings.end_on_final_instr = true;
			run_setting = true;
			break;

		case 'd':
			settings.cpu_dump_mode = parse_cpu_dump_mode(optarg);
			run_setting = true;
			break;

		case 'm':
			settings.mrhead = parse_mem_region(optarg);
			run_setting = true;
			break;

		case 't':
			infile = optarg;
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_TRANSLATE;
			break;

		case 'o':
			outfile = optarg;
			tra_setting = true;
			break;

		case 'h':
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_HELP;
			break;

		default:
			IMPROPER_USAGE;
		}
	}

	if (!infile && action != MAIN_ACTION_HELP) {
		IMPROPER_USAGE;
	}

	switch (action) {

	case MAIN_ACTION_HELP:

		if (run_setting || tra_setting) {
			IMPROPER_USAGE;
		}

		print_help();

		break;

	case MAIN_ACTION_TRANSLATE:

		if (run_setting) {
			IMPROPER_USAGE;
		}

		return translate_file(infile, outfile, &settings);

	case MAIN_ACTION_RUN:

		if (tra_setting) {
			IMPROPER_USAGE;
		}

		return run_action(infile, &settings);

	case MAIN_ACTION_NONE:
		IMPROPER_USAGE;
	}

	return 0;
}
