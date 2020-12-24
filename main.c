#include <stdio.h>
#include <unistd.h> /* getopt */

#include "translator.h"
#include "instr.h"

struct translation_data_t {
	const char *infile;
	const char *outfile;
};

void run_cpu(void) {
	DEFINE_INSTR_MAP(instr_map);
	populate_imap(instr_map);
	print_imap(instr_map);
}

int print_binary(unsigned int len, const uint8_t* out, void* data) {
	struct translation_data_t* td;
	unsigned int i, j, cols, rows, end;

	td = (struct translation_data_t*)data;

	printf("(i) Dumping binary translated from %s\n", td->infile);

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

int export_binary(unsigned int len, const uint8_t* out, void* data) {
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

	return translate(td.infile, 0, 0, 256, export_binary, &td);
}

int main(int argc, char* const argv[]) {
	int opt;
	char* infile;
	char* outfile;
	bool do_translate;

	infile = NULL;
	outfile = NULL;
	do_translate = false;

	while ((opt = getopt(argc, argv, "t:c:o:h")) != -1) {

		switch (opt) {
		case 't':
			infile = optarg;
			do_translate = true;

			break;
		case 'o':
			outfile = optarg;

			break;
		case 'h':
			printf("Example usage:\n\n\tsikso2 -t "
			       "test.asm -o test\n\n");

			return 0;
		case 'c':
			infile = optarg;
			do_translate = false;

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

	if (do_translate)
		return translate_file(infile, outfile);

	return -1;
}
