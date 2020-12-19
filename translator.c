#include "translator.h"

#include <regex.h>
#include <stdbool.h>
#include <stdlib.h> /* strtol */
#include <stdio.h>
#include <errno.h>
#include <string.h> /* strerror */

#include "instr.h"

#define MAX_LINE_SIZE 80

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
		ret = regcomp(&(trans->VAR), NAME, REG_NEWLINE); \
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
	ret = regexec(&(trans->REG), str, PMATCH_SIZE, pmatch, 0); \
		if (ret == REG_NOMATCH) ret = -1; \
		if (ret != -1)

#define for_each_instr_el(trans, instr_el) \
	for (instr_el = (trans)->instr_head; instr_el != NULL; \
	     instr_el = (instr_el)->next)

#define for_each_label_in(instr_el, lbl) \
	for (lbl = (instr_el)->lbl_head; lbl != NULL; lbl = (lbl)->next)

extern instr_t* get_instr_list(void);
extern size_t get_instr_list_size(void);

struct lbl_list_t {
	char label[MAX_LINE_SIZE];
	struct lbl_list_t* next;
};

struct instr_el_t {
	char name[3];
	unsigned int arg;
	char reg;
	unsigned int addr;
	bool labels_pending;
	char labelp[MAX_LINE_SIZE];
	opcode_t opcode;
	struct lbl_list_t* lbl_head;
	struct instr_el_t* next;
};

typedef struct {
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

	unsigned int load_addr;
	unsigned int zero_page;
	unsigned int page_size;

	struct instr_el_t* instr_head;
} translator_t;

static void update_instr_addr(translator_t* trans, struct instr_el_t* iel,
			      unsigned int length) {
	static unsigned int curr_addr = 0;

	if (!curr_addr)
		curr_addr = trans->load_addr;

	iel->addr = curr_addr;

	curr_addr += length;

	return;
}

static void init_instr(struct instr_el_t* iel) {
	unsigned int i = 0;

	iel->lbl_head = NULL;
	iel->next = NULL;
	iel->labels_pending = false;

	for (i = 0; i < 3; i++)
		iel->name[i] = 0;

	return;
}

static void set_instr_name(struct instr_el_t* iel, const char* name) {
	unsigned int i = 0;

	for (i = 0; i < 3; i++)
		iel->name[i] = name[i];

	return;
}

static void set_instr_labelp(struct instr_el_t* iel, const char* labelp) {
	unsigned int i = 0;

	for (i = 0; i < strlen(labelp); i++)
		iel->labelp[i] = labelp[i];

	return;
}

static void init_lbl(struct lbl_list_t* lbl) {
	unsigned int i = 0;

	for (i = 0; i < MAX_LINE_SIZE; i++)
		lbl->label[i] = 0;

	lbl->next = NULL;

	return;
}

static void add_label_to(struct instr_el_t* iel, const char* lbl,
			 unsigned int lbl_size) {
	struct lbl_list_t* curr;
	struct lbl_list_t* new_lbl;
	unsigned int i;

	ttrace("Adding label %s.", lbl);

	new_lbl = malloc(sizeof(*new_lbl));

	for (i = 0; i < lbl_size; i++)
		new_lbl->label[i] = lbl[i];

	new_lbl->next = NULL;

	curr = iel->lbl_head;

	if (!curr) {
		iel->lbl_head = new_lbl;

		return;
	}

	if (curr->next)
		while (curr->next) curr = curr->next;

	curr->next = new_lbl;

	return;
}

static struct instr_el_t* add_empty_instr(translator_t* trans) {
	struct instr_el_t* curr;
	struct instr_el_t* new_instr;

	ttrace("Allocating memory for the new instruction.");

	new_instr = malloc(sizeof(*new_instr));
	init_instr(new_instr);

	curr = trans->instr_head;

	if (!curr) {
		trans->instr_head = new_instr;

		return new_instr;
	}

	if (curr->next)
		while (curr->next) curr = curr->next;

	curr->next = new_instr;

	return new_instr;
}

static int translator_init(translator_t* trans) {
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

	trans->instr_head = NULL;

	return ret;
}

static void free_lbl(struct lbl_list_t* lbl) {
	struct lbl_list_t* temp_lbl;

	temp_lbl = lbl;
	free(temp_lbl);

	return;
}

static void deinit_lbl(struct instr_el_t* instr) {
	struct lbl_list_t* curr_lbl;
	struct lbl_list_t* next_lbl;

	if (instr->lbl_head) {

		curr_lbl = instr->lbl_head;

		if (!curr_lbl->next)
			free(curr_lbl);
		else {
			while (curr_lbl) {
				next_lbl = curr_lbl->next;
				free_lbl(curr_lbl);
				curr_lbl = next_lbl;
			}
		}
	}

	return;
}

static void free_ins(struct instr_el_t* instr) {
	struct instr_el_t* temp_el;

	temp_el = instr;
	deinit_lbl(instr);
	free(temp_el);

	return;
}

static void translator_deinit(translator_t* trans) {
	struct instr_el_t* curr_ins;
	struct instr_el_t* next_ins;

	curr_ins = trans->instr_head;

	regfree(&(trans->emp_re));
	regfree(&(trans->cin_re));
	regfree(&(trans->sin_re));
	regfree(&(trans->lbe_re));
	regfree(&(trans->lbi_re));
	regfree(&(trans->bra_re));
	regfree(&(trans->imm_re));
	regfree(&(trans->gen_re));
	regfree(&(trans->gxy_re));
	regfree(&(trans->inx_re));
	regfree(&(trans->iny_re));

	if (curr_ins) {

		if (!curr_ins->next)
			free(curr_ins);

		else {
			while (curr_ins) {
				next_ins = curr_ins->next;
				free_ins(curr_ins);
				curr_ins = next_ins;
			}
		}
	}

	return;
}

static int get_addr(const char* str) {
	int ret;

	errno = 0;
	ret = (int)strtol(str, NULL, 16);

	if (errno) {
		logt_err("%s", strerror(errno));

		return -1;
	}

	return ret;
}

#define set_new_instr_addr(buff) \
	new_instr->arg = get_addr(buff); \
	if (new_instr->arg < 0) return -1;

static int handle_addr(translator_t* trans, struct instr_el_t* new_instr,
		       const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	unsigned int i;
	int ret;
	char match_buff[MAX_LINE_SIZE] = { 0 };

	ret = 0;

	REGEXEC(imm_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Immediate: %s", match_buff);
		set_new_instr_addr(match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(gen_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Zero page / Absolute: %s", match_buff);
		set_new_instr_addr(match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(gxy_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Zero page / Absolute: %s", match_buff);
		set_new_instr_addr(match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ttrace("Register: %s", match_buff);
		new_instr->reg = match_buff[0];
		clean_pmatch(match_buff, 2);

		return ret;
	}

	REGEXEC(inx_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Indirect X: %s", match_buff);
		set_new_instr_addr(match_buff);
		new_instr->reg = 'X';
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(iny_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Indirect Y: %s", match_buff);
		set_new_instr_addr(match_buff);
		new_instr->reg = 'Y';
		clean_pmatch(match_buff, 1);

		return ret;
	}

	return ret;
}

#define get_last_instr(trans, instr) \
	instr = (trans)->instr_head; \
	if (instr) while ((instr)->next) instr = (instr)->next;

static int handle_line(translator_t* trans, const char* str,
		       struct instr_el_t* old_instr) {
	regmatch_t pmatch[PMATCH_SIZE];
	unsigned int i;
	int ret;
	bool labels_pending;
	struct instr_el_t* new_instr;
	struct instr_el_t* last_instr;
	char match_buff[MAX_LINE_SIZE] = { 0 };

	get_last_instr(trans, last_instr)
	if (last_instr) labels_pending = last_instr->labels_pending;

	ret = 0;

	ttracei("Parsing line \"%s\"", str);

	REGEXEC(emp_re) {
		ttrace("Empty line.");

		return ret;
	}

	if (!labels_pending) {
		new_instr = add_empty_instr(trans);
		last_instr = new_instr;
	}

	else if (old_instr) {
		new_instr = old_instr;
		last_instr = old_instr;
	}

	REGEXEC(lbe_re) {

		if (!labels_pending)
			last_instr->labels_pending = true;

		get_pmatch_to(match_buff, 1);
		ttrace("Empty label %s (%ld)", match_buff,
		       strlen(match_buff));
		add_label_to(new_instr, match_buff, strlen(match_buff));
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(lbi_re) {

		if (!labels_pending)
			last_instr->labels_pending = true;

		get_pmatch_to(match_buff, 1);
		ttrace("Complex label %s (%ld)", match_buff,
		       strlen(match_buff));
		add_label_to(new_instr, match_buff, strlen(match_buff));
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ret = handle_line(trans, match_buff, new_instr);
		if (ret)
			return ret;
		clean_pmatch(match_buff, 2);

		return ret;
	}

	if (last_instr->labels_pending) {
		last_instr->labels_pending = false;
		ttrace("No labels pending after this instruction.");
	}

	REGEXEC(cin_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Complex instruction: %s", match_buff);
		set_instr_name(new_instr, match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ret = handle_addr(trans, new_instr, match_buff);
		if (ret)
			return ret;
		clean_pmatch(match_buff, 2);

		return ret;
	}

	REGEXEC(sin_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Simple instruction: %s", match_buff);
		set_instr_name(new_instr, match_buff);
		clean_pmatch(match_buff, 1);

		return ret;
	}

	REGEXEC(bra_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Branch / Jump: %s", match_buff);
		set_instr_name(new_instr, match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ttrace("Label: %s", match_buff);
		set_instr_labelp(new_instr, match_buff);
		clean_pmatch(match_buff, 2);

		return ret;
	}

	return ret;
}

int translate(const char* filename, unsigned int load_addr,
	      unsigned int zero_page, unsigned int page_size) {
	static bool init = 0;
	translator_t trans;
	int ret, curr, last, i, line;
	bool reading, entered_comment, started_newl;
	FILE* f;
	char line_buffer[MAX_LINE_SIZE] = { 0 };

	ret = 0;
	trans.load_addr = load_addr;
	trans.zero_page = zero_page;
	trans.page_size = page_size;

	if (!init) {
		ret = translator_init(&trans);
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
		started_newl = true;

		while (curr != '\n' && curr != EOF) {
			curr = fgetc(f);
		
			if (started_newl && (curr == '\t' || curr == ' '))
				continue;
			else
				started_newl = false;

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

		ret = handle_line(&trans, line_buffer, NULL);

		/* for now ignore errors */
		if (ret) {
			logt_err("Syntax error at line %d: %s",
				 line, line_buffer);

			break;
		}

		for (i = 0; i <= last; i++)
			line_buffer[i] = 0;
	}

	fclose(f);

	translator_deinit(&trans);

	return ret;
}
