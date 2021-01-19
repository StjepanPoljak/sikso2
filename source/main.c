#include <stdio.h>
#include <unistd.h> /* getopt */
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

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

static int mem_byte_op(struct mem_byte_t* curr, void* data) {

	mtrace("Loading byte %.2x at address %.4x", curr->byte, curr->addr);

	return load_to_ram((struct device_t*)data, curr->addr,
			   &curr->byte, 1, false);
}

static int main_run_device(unsigned int len, const uint8_t* out, void* data) {
	struct device_t device;
	struct cpu_6502_t cpu;
	int ret;

	ret = 0;

	mtracei("Initializing CPU 6502 actions.");

	init_cpu_6502_actions();

	mtracei("Initializing device.");

	init_cpu(&cpu, get_instr_list());
	init_device(&device, &cpu,
		    get_load_addr(((settings_t*)data)),
		    get_stack_addr(((settings_t*)data)),
		    get_ram_size(((settings_t*)data)));

	/* load ram image, if any */
	if (((settings_t*)data)->mimage) {
		mtracei("Loading RAM image to %.4x.",
			((settings_t*)data)->mimage->addr);
		ret = load_to_ram(&device, ((settings_t*)data)->mimage->addr,
				((settings_t*)data)->mimage->contents,
				((settings_t*)data)->mimage->length, false);
		if (ret)
			return ret;
	}

	/* load binary */
	ret = load_to_ram(&device, get_load_addr(((settings_t*)data)),
			  out, len, true);
	if (ret)
		return ret;

	/* load other bytes to RAM, if any */
	if (((settings_t*)data)->mbhead) {
		mtracei("Loading bytes to RAM...");
		ret = do_for_each_mem_byte(((settings_t*)data)->mbhead,
					   mem_byte_op, (void*)(&device));
	}

	run_device(&device,
		   ((settings_t*)data)->end_on_final_instr,
		   ((settings_t*)data)->cpu_dump_mode,
		   ((settings_t*)data)->mrhead);

	return ret;
}

static int run_binary(const char* infile, settings_t* settings) {
	int ret;
	uint8_t* out;
	unsigned int fsize;

	out = load_file(infile, &fsize);
	if (!out)
		return -1;

	ret = main_run_device(fsize, out, (void*)settings);

	free(out);

	return ret;
}

static int run_action(const char* infile, settings_t* settings) {

	return translate(infile, get_load_addr(settings),
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
			 export_binary, &td);
}

static int disassemble_file(const char* infile, settings_t* settings) {
	DEFINE_INSTR_MAP(instr_map);
	populate_imap(instr_map);

#ifdef TRANSLATOR_TRACE
	print_imap(instr_map);
#endif

	return disassemble(infile, instr_map, settings->dmode);
}

typedef enum {
	MAIN_ACTION_TRANSLATE,
	MAIN_ACTION_RUN,
	MAIN_ACTION_RUN_BINARY,
	MAIN_ACTION_DISASSEMBLE,
	MAIN_ACTION_HELP,
	MAIN_ACTION_NONE
} main_action_t;

static struct option long_options[] = {
	{ "run-asm",		required_argument,	0, 'r' },
	{ "run-bin",		required_argument,	0, 'R' },
	{ "load-addr",		required_argument,	0, 'a' },
	{ "stack-addr",		required_argument,	0, 's' },
	{ "ram-size",		required_argument,	0, 'M' },
	{ "stop",		no_argument,		0, 'S' },
	{ "dump-cpu",		no_argument,		0, 'd' },
	{ "dump-mem",		required_argument,	0, 'm' },
	{ "ram-bytes",		required_argument,	0, 'b' },
	{ "ram-file",		required_argument,	0, 'f' },
	{ "translate",		required_argument,	0, 't' },
	{ "disassemble",	required_argument,	0, 'D' },
	{ "pretty",		no_argument,		0, 'p' },
	{ "output",		required_argument,	0, 'o' },
	{ "help",		required_argument,	0, 'h' },
	{ 0,			0,			0,  0  }
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

#define S_HELP_STR(STACK_ADDR) \
	"stack address (default: " TEX(STACK_ADDR) ")"

#define A_HELP_STR(LOAD_ADDR) \
	"load addr (default: " TEX(LOAD_ADDR) ")"

#define M_HELP_STR(RAM_SIZE) \
	"ram size in bytes (default: " TEX(RAM_SIZE) ")"

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
			help_text("run assembly file");
			break;
		case 'R':
			help_text("run binary file");
			break;
		case 'a':
			help_text(A_HELP_STR(DEFAULT_LOAD_ADDR));
			break;
		case 's':
			help_text(S_HELP_STR(DEFAULT_STACK_ADDR));
			break;
		case 'M':
			help_text(M_HELP_STR(DEFAULT_RAM_SIZE));
			break;
		case 'S':
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
		case 'b':
			help_text("load bytes to RAM "
				  "(e.g. 0x0700:0e,0x0702:ff)");
			break;
		case 'f':
			help_text("load file to RAM (e.g. 0x0700:file_name)");
			break;
		case 't':
			help_text("translate file to binary");
			break;
		case 'D':
			help_text("disassemble binary file");
			break;
		case 'p':
			help_text("print binary when disassembling");
			break;
		case 'o':
			help_text("output file name (default: a.out)");
			break;
		case 'h':
			help_text("print help");
			break;
		default:
			help_text("no help");
			break;
		}

		i++;
	}

	printf("\n");

	return;
}

static void on_err(int merrno) {

	logm_err("Could not parse argument (%s)", strerror(merrno));

	return;
}

static int parse_arg(const char* str) {

	return parse_str(str, is_hex(str) ? 16 : 10, on_err);
}

#define IMPROPER_USAGE \
	printf("Improper usage. Try -h for help.\n"); \
	ret = -1; \
	goto exit_main;

typedef enum {
	SETTING_RUN,
	SETTING_TRANSLATE,
	SETTING_DISASSEMBLE,
	SETTING_NONE
} setting_category_t;

#define set_setting(sc_old, sc_new) do { \
	if (sc_old == SETTING_NONE || sc_old == sc_new) { \
		sc_old = sc_new; \
	} else { \
		IMPROPER_USAGE; \
	} } while(0)

int main(int argc, char* const argv[]) {
	int opt;
	char* infile;
	char* outfile;
	main_action_t action;
	settings_t settings;
	int option_index = 0;
	setting_category_t sc;
	int ret;

	ret = 0;

	infile = NULL;
	outfile = NULL;
	action = MAIN_ACTION_NONE;
	sc = SETTING_NONE;

	init_settings(&settings);

	while ((opt = getopt_long(argc, argv, "r:R:a:SM:s:d:m:b:f:t:D:po:h",
				  long_options, &option_index)) != -1) {
		switch (opt) {

		case 'r':
			infile = optarg;
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_RUN;
			break;

		case 'R':
			infile = optarg;
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_RUN_BINARY;
			break;

		case 'a':
			settings.load_addr = (uint16_t)parse_arg(optarg);
			break;
		case 's':
			settings.stack_addr = (uint16_t)parse_arg(optarg);
			set_setting(sc, SETTING_RUN);
			break;
		case 'M':
			settings.ram_size = (uint16_t)parse_arg(optarg);
			set_setting(sc, SETTING_RUN);
			break;

		case 'S':
			settings.end_on_final_instr = true;
			set_setting(sc, SETTING_RUN);
			break;

		case 'd':
			settings.cpu_dump_mode = parse_cpu_dump_mode(optarg);
			set_setting(sc, SETTING_RUN);
			break;

		case 'm':
			settings.mrhead = parse_mem_region(optarg);
			if (!settings.mrhead) {
				ret = -1;
				goto exit_main;
			}
			set_setting(sc, SETTING_RUN);
			break;

		case 'b':
			settings.mbhead = parse_mem_bytes(optarg);
			if (!settings.mbhead) {
				ret = -1;
				goto exit_main;
			}
			set_setting(sc, SETTING_RUN);
			break;

		case 'f':
			settings.mimage = get_mem_image(optarg);
			if (!settings.mimage) {
				ret = -1;
				goto exit_main;
			}
			set_setting(sc, SETTING_RUN);
			break;

		case 't':
			infile = optarg;
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_TRANSLATE;
			break;

		case 'D':
			infile = optarg;
			if (action != MAIN_ACTION_NONE) {
				IMPROPER_USAGE;
			}
			action = MAIN_ACTION_DISASSEMBLE;
			break;

		case 'p':
			settings.dmode = DISASM_PRETTY;
			set_setting(sc, SETTING_DISASSEMBLE);
			break;

		case 'o':
			outfile = optarg;
			set_setting(sc, SETTING_TRANSLATE);
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

		if (sc != SETTING_NONE) {
			IMPROPER_USAGE;
		}

		print_help();

		break;

	case MAIN_ACTION_TRANSLATE:

		if (sc != SETTING_NONE && sc != SETTING_TRANSLATE) {
			IMPROPER_USAGE;
		}

		ret = translate_file(infile, outfile, &settings);
		break;

	case MAIN_ACTION_RUN_BINARY:

		if (sc != SETTING_NONE && sc != SETTING_RUN) {
			IMPROPER_USAGE;
		}

		ret = run_binary(infile, &settings);
		break;

	case MAIN_ACTION_RUN:

		if (sc != SETTING_NONE && sc != SETTING_RUN) {
			IMPROPER_USAGE;
		}

		ret = run_action(infile, &settings);
		break;

	case MAIN_ACTION_DISASSEMBLE:

		if (sc != SETTING_NONE && sc != SETTING_DISASSEMBLE) {
			IMPROPER_USAGE;
		}

		ret = disassemble_file(infile, &settings);
		break;

	case MAIN_ACTION_NONE:
		IMPROPER_USAGE;
	}

exit_main:

	free_settings(&settings);

	return ret;
}
