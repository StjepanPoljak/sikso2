#include <regex.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_LINE_SIZE 256
#define TRANSLATOR_TRACE

#define logt_err(FMT, ...) printf("(!) " FMT "\n", ## __VA_ARGS__)

#ifdef TRANSLATOR_TRACE
#define ttrace(FMT, ...) printf("  -> " FMT "\n", ## __VA_ARGS__)
#define ttracei(FMT, ...) printf("(i) " FMT "\n", ## __VA_ARGS__)
#else
#define ttrace(FMT, ...) ;
#define ttracei(FMT, ...) ;
#endif

#define RE_BEGIN "^\\s*"
#define RE_END "\\s*$"
#define RE_SEP "\\s\\+"
#define RE_EMPTY "^\\s\\+$"

#define ADR_FMT_RE "\\$\\([0-9a-fA-F]\\+\\)"
#define IMM_ADR_RE RE_BEGIN "#" ADR_FMT_RE RE_END
#define GEN_ADR_RE RE_BEGIN ADR_FMT_RE RE_END
#define GEN_AXY_RE RE_BEGIN ADR_FMT_RE "\\s*,\\s*\\([XY]\\)" RE_END
#define IND_ADX_RE RE_BEGIN "(\\s*" ADR_FMT_RE "\\s*,\\s*\\X\\s*)" RE_END
#define IND_ADY_RE RE_BEGIN "(\\s*" ADR_FMT_RE "\\s*)\\s*,\\s*Y" RE_END

#define INS_RE_STR "\\([a-zA-Z]..\\)"
#define LBL_RE_STR "\\([a-zA-Z0-9_]\\+\\)"
#define ADR_GEN_RE "[#\\$0-9a-fA-F()]\\+\\s*,\\?\\s*[XY()]*"

#define LBL_EMP_RE RE_BEGIN LBL_RE_STR ":" RE_END
#define LBL_INS_RE RE_BEGIN LBL_RE_STR ":\\(.*\\)$"

#define SIN_RE_STR RE_BEGIN INS_RE_STR RE_END
#define BRA_RE_STR RE_BEGIN INS_RE_STR RE_SEP LBL_RE_STR RE_END
#define CIN_RE_STR RE_BEGIN INS_RE_STR RE_SEP "\\(" ADR_GEN_RE "\\)" RE_END

#define REGCOMP(VAR, NAME) \
	do { \
		ret = regcomp(&VAR, NAME, REG_NEWLINE); \
		if (ret) { \
			logt_err("Could not compile " #NAME " regex."); \
			return ret; \
		} \
	} while (0)

#define get_pmatch_to(buff, match) \
	do { \
		i = 0; \
		while ((i < pmatch[match].rm_eo - pmatch[match].rm_so)) { \
			buff[i] = str[i + pmatch[match].rm_so]; \
			i++; \
		} \
	} while (0)

#define clean_pmatch(buff, match) \
	do { \
		for (i = 0; i < pmatch[match].rm_eo - pmatch[match].rm_so; \
			i++) { \
			buff[i] = 0; \
		} \
	} while (0)

#define PMATCH_SIZE 3

#define REGEXEC(REG) \
	ret = regexec(&REG, str, PMATCH_SIZE, pmatch, 0); \
		if (ret != REG_NOMATCH)

regex_t emp_re;

regex_t sin_re;
regex_t cin_re;
regex_t lbe_re;
regex_t lbi_re;
regex_t bra_re;

regex_t imm_re;
regex_t gen_re;
regex_t gxy_re;
regex_t inx_re;
regex_t iny_re;

static int translator_init(void) {
	int ret;

	REGCOMP(emp_re, RE_EMPTY);

	REGCOMP(lbe_re, LBL_EMP_RE);
	REGCOMP(lbi_re, LBL_INS_RE);
	REGCOMP(sin_re, SIN_RE_STR);
	REGCOMP(cin_re, CIN_RE_STR);
	REGCOMP(bra_re, BRA_RE_STR);

	REGCOMP(imm_re, IMM_ADR_RE);
	REGCOMP(gen_re, GEN_ADR_RE);
	REGCOMP(gxy_re, GEN_AXY_RE);
	REGCOMP(inx_re, IND_ADX_RE);
	REGCOMP(iny_re, IND_ADY_RE);

	return ret;
}

static void translator_deinit(void) {

	regfree(&emp_re);

	regfree(&cin_re);
	regfree(&sin_re);
	regfree(&lbe_re);
	regfree(&lbi_re);
	regfree(&bra_re);

	regfree(&imm_re);
	regfree(&gen_re);
	regfree(&gxy_re);
	regfree(&inx_re);
	regfree(&iny_re);

	return;
}

static int handle_addr(const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	unsigned int i;
	int ret;
	char match_buff[MAX_LINE_SIZE] = { 0 };

	ret = 0;

	REGEXEC(imm_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Immediate: %s", match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(gen_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Zero page / Absolute: %s", match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(gxy_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Zero page / Absolute: %s", match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ttrace("Register: %s", match_buff);
		clean_pmatch(match_buff, 2);

		return ret;
	}

	return ret;
}

static int handle_line(const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	unsigned int i;
	int ret;
	char match_buff[MAX_LINE_SIZE] = { 0 };

	ret = 0;

	ttracei("Parsing line \"%s\"", str);

	REGEXEC(emp_re) {
		ttrace("Empty line.");

		return ret;
	}

	REGEXEC(lbe_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Empty label %s", match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(lbi_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Complex label %s", match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ret = handle_line(match_buff);
		if (ret)
			return ret;
		clean_pmatch(match_buff, 2);

		return ret;
	}

	REGEXEC(cin_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Complex instruction: %s", match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ret = handle_addr(match_buff);
		if (ret)
			return ret;
		clean_pmatch(match_buff, 2);

		return ret;
	}

	REGEXEC(sin_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Simple instruction: %s", match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(bra_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Branch / Jump: %s", match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ttrace("Label: %s", match_buff);
		clean_pmatch(match_buff, 2);

		return ret;
	}

	return ret;
}

int translate(const char* filename) {

	static bool init = 0;
	int ret, curr, last, i, line;
	bool reading, entered_comment;
	FILE* f;
	char line_buffer[MAX_LINE_SIZE] = { 0 };

	ret = 0;

	if (!init) {
		ret = translator_init();
		if (ret)
			return ret;

		init = 1;
	}

	f = fopen(filename, "r");
	if (!f) {
		logt_err("Could not open file %s.", filename);
		return -1;
	}

	reading = true;
	line = 0;

	while (reading) {

		curr = 0;
		last = 0;
		entered_comment = false;

		while (curr != '\n' && curr != EOF) {
			curr = fgetc(f);
		
			if (curr == EOF) {
				reading = false;

				break;
			}

			if (curr == ';')
				entered_comment = true;

			if (!entered_comment && last >= MAX_LINE_SIZE) {
				logt_err("Line buffer too small.");
				ret = -1;

				break;
			}

			if (!entered_comment && curr != '\n' && curr != EOF)
				line_buffer[last++] = curr;
		}

		if (ret)
			break;

		line++;

		if (last <= 1) {
			continue;
		}

		ret = handle_line(line_buffer);

		/* for now ignore errors */
		if (ret) {
			logt_err("Syntax error at line %d: %s", line, line_buffer);

			break;
		}

		for (i = 0; i <= last; i++)
			line_buffer[i] = 0;
	}

	fclose(f);

	translator_deinit();

	return ret;
}
