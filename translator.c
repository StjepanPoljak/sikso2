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
#define JMP_IND_RE RE_BEGIN INS_RE_STR RE_SEP "(" RE_SEP LBL_RE_STR RE_SEP ")" RE_END

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

#define instr_name_to_chars(iel) iel->name[0], iel->name[1], iel->name[2]

extern instr_t* get_instr_list(void);
extern size_t get_instr_list_size(void);

struct instr_el_t {
	char name[4];
	opcode_t opcode;
	unsigned int arg;
	unsigned int addr;

	bool labels_pending;
	struct instr_el_t* next;
};

struct lbl_node_t {
	char label[MAX_LINE_SIZE];
	struct instr_el_t* instr;
	struct lbl_node_t* left;
	struct lbl_node_t* right;
};

typedef struct {
	regex_t emp_re;

	regex_t sin_re;
	regex_t cin_re;
	regex_t lbe_re;
	regex_t lbi_re;
	regex_t bra_re;
	regex_t jin_re;

	regex_t imm_re;
	regex_t gen_re;
	regex_t gxy_re;
	regex_t inx_re;
	regex_t iny_re;

	unsigned int load_addr;
	unsigned int curr_addr;
	unsigned int zero_page;
	unsigned int page_size;

	struct instr_el_t* instr_head;
	struct lbl_node_t* label_pool;

} translator_t;

/* ========= translator init / deinit ========= */

static int translator_init(translator_t* trans) {
	int ret;

	REGCOMP(emp_re, RE_EMPTY);

	REGCOMP(lbe_re, LBL_EMP_RE);
	REGCOMP(lbi_re, LBL_INS_RE);
	REGCOMP(sin_re, SIN_RE_STR);
	REGCOMP(cin_re, CIN_RE_STR);
	REGCOMP(bra_re, BRA_RE_STR);
	REGCOMP(jin_re, JMP_IND_RE);

	REGCOMP(imm_re, IMM_ADR_RE);
	REGCOMP(gen_re, GEN_ADR_RE);
	REGCOMP(gxy_re, GEN_AXY_RE);
	REGCOMP(inx_re, IND_ADX_RE);
	REGCOMP(iny_re, IND_ADY_RE);

	trans->instr_head = NULL;
	trans->label_pool = NULL;

	return ret;
}

/* ========= label operations ========= */

static void lbl_init(struct lbl_node_t* lbl, const char name[MAX_LINE_SIZE], unsigned int name_size) {
	unsigned int i = 0;

	for (i = 0; i < MAX_LINE_SIZE; i++) {

		if (i < name_size)
			lbl->label[i] = name[i];
		else
			lbl->label[i] = 0;
	}

	lbl->left = NULL;
	lbl->right = NULL;

	return;
}
static void free_lbl(struct lbl_node_t* lbl) {
	struct lbl_node_t* temp_lbl;

	temp_lbl = lbl;
	free(temp_lbl);

	return;
}

static bool add_lbl(translator_t* trans, const char name[MAX_LINE_SIZE],
		    unsigned int name_size, struct instr_el_t* curr_instr) {
	struct lbl_node_t* new_lbl;
	struct lbl_node_t* curr;
	int res;

	new_lbl = malloc(sizeof(*new_lbl));

	lbl_init(new_lbl, name, name_size);
	new_lbl->instr = curr_instr;

	curr = trans->label_pool;

	while (curr) {
		res = strncmp(curr->label, new_lbl->label, name_size);

		if (res > 0) {

			if (curr->right)
				curr = curr->right;
			else {
				curr->right = new_lbl;

				return true;
			}
		}
		else if (res < 0) {

			if (curr->left)
				curr = curr->left;
			else {
				curr->left = new_lbl;

				return true;
			}
		}
		else {
			logt_err("Label %s already exists.", name);

			return false;
		}
	}

	trans->label_pool = new_lbl;

	return true;
}

static void lbl_push(struct lbl_node_t* lbl, struct lbl_node_t*** stack,
		     int* last, int* size) {

	if (!(*size)) {
		*size = 1;
		*last = -1;
		*stack = malloc(sizeof(**stack) * *size);
	}
	else if ((*last + 1) == (*size)) {
		*size *= 2;
		*stack = realloc(*stack, sizeof(**stack) * *size);
	}

	(*stack)[++(*last)] = lbl;

	return;
}

static struct lbl_node_t* lbl_pop(struct lbl_node_t** stack, int* last) {

	return *last < 0 ? NULL : stack[(*last)--];
}

static void deinit_lbl(translator_t* trans) {
	struct lbl_node_t** stack;
	struct lbl_node_t* curr;
	int last;
	int size;

	size = 0;

	ttracei("Deinitializing label pool...");

	lbl_push(trans->label_pool, &stack, &last, &size);

	while ((curr = lbl_pop(stack, &last))) {
		ttrace("Freeing %s", curr->label);

		if (curr->left)
			lbl_push(curr->left, &stack, &last, &size);

		if (curr->right)
			lbl_push(curr->right, &stack, &last, &size);

		free_lbl(curr);
	}

	free(stack);

	return;
}

static void free_ins(struct instr_el_t* instr) {
	struct instr_el_t* temp_el;

	ttrace("Freeing instruction: %c%c%c", instr_name_to_chars(instr));

	temp_el = instr;
	free(temp_el);

	return;
}

static void translator_deinit(translator_t* trans) {
	struct instr_el_t* curr_ins;
	struct instr_el_t* next_ins;

	ttracei("Deinitializing translator.");

	curr_ins = trans->instr_head;

	regfree(&(trans->emp_re));

	regfree(&(trans->cin_re));
	regfree(&(trans->sin_re));
	regfree(&(trans->lbe_re));
	regfree(&(trans->lbi_re));
	regfree(&(trans->bra_re));
	regfree(&(trans->jin_re));

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

	deinit_lbl(trans);

	return;
}

/* ========= instruction operations ========= */

static void instr_init(struct instr_el_t* iel) {
	unsigned int i = 0;

	iel->next = NULL;
	iel->labels_pending = false;

	for (i = 0; i < 3; i++)
		iel->name[i] = 0;

	return;
}

static struct instr_el_t* add_empty_instr(translator_t* trans) {
	struct instr_el_t* curr;
	struct instr_el_t* new_instr;

	ttrace("Allocating memory for the new instruction.");

	new_instr = malloc(sizeof(*new_instr));
	instr_init(new_instr);

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

static void update_instr_addr(translator_t* trans, struct instr_el_t* iel,
			      unsigned int length) {
	static unsigned int curr_addr = 0;

	if (!curr_addr)
		curr_addr = trans->load_addr;

	iel->addr = curr_addr;

	curr_addr += length;

	return;
}

static void set_instr_name(struct instr_el_t* iel, const char* name) {
	unsigned int i = 0;

	for (i = 0; i < 3; i++)
		iel->name[i] = name[i];

	return;
}

/* ========= address handling  ========= */

static int get_arg(const char* str) {
	int ret;

	errno = 0;
	ret = (int)strtol(str, NULL, 16);

	if (errno) {
		logt_err("%s", strerror(errno));

		return -1;
	}

	return ret;
}

#define set_new_instr_arg(buff) \
	new_instr->arg = get_arg(buff); \
	if (new_instr->arg < 0) return -1;

#define set_opcode_and_length(s, new_instr, mode) \
	if (!get_subinstr((new_instr)->name, mode, &s)) { \
		logt_err("Opcode for %c%c%c not found", instr_name_to_chars(new_instr)); \
		ret = -1; \
	} \
	else { \
		ttracei("Setting opcode for %c%c%c (@%p)", instr_name_to_chars(new_instr), s); \
		(new_instr)->opcode = s->opcode; \
		ret = s->length; \
	}

static int handle_arg(translator_t* trans, struct instr_el_t* new_instr,
		       const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	unsigned int i;
	int ret;
	char match_buff[MAX_LINE_SIZE] = { 0 };
	instr_mode_t mode;
	subinstr_t* s;

	ret = 0;

	REGEXEC(imm_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Immediate: %s", match_buff);
		set_new_instr_arg(match_buff);
		clean_pmatch(match_buff, 1);

		set_opcode_and_length(s, new_instr, MODE_IMMEDIATE);

		return ret;
	}

	REGEXEC(gen_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Zero page / Absolute: %s", match_buff);
		set_new_instr_arg(match_buff);
		clean_pmatch(match_buff, 1);
		mode = new_instr->arg >= trans->zero_page + trans->page_size ? MODE_ABSOLUTE : MODE_ZERO_PAGE;

		set_opcode_and_length(s, new_instr, mode);

		return ret;
	}

	REGEXEC(gxy_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Zero page / Absolute: %s", match_buff);
		set_new_instr_arg(match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ttrace("Register: %s", match_buff);
		mode = new_instr->arg >= trans->zero_page + trans->page_size
		     ? (match_buff[0] == 'X' ? MODE_ABSOLUTE_X : MODE_ABSOLUTE_Y)
		     : (match_buff[0] == 'X' ? MODE_ZERO_PAGE_X : MODE_ZERO_PAGE_Y);
		clean_pmatch(match_buff, 2);

		set_opcode_and_length(s, new_instr, mode);

		return ret;
	}

	REGEXEC(inx_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Indirect X: %s", match_buff);
		set_new_instr_arg(match_buff);
		mode = MODE_INDIRECT_X;
		clean_pmatch(match_buff, 1);

		set_opcode_and_length(s, new_instr, mode);

		return ret;
	}

	REGEXEC(iny_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Indirect Y: %s", match_buff);
		set_new_instr_arg(match_buff);
		mode = MODE_INDIRECT_Y;
		clean_pmatch(match_buff, 1);

		set_opcode_and_length(s, new_instr, mode);

		return ret;
	}

	return -1;
}

/* ========= instruction / line handling ========= */

#define get_last_instr(trans, instr) \
	instr = (trans)->instr_head; \
	if (instr) while ((instr)->next) instr = (instr)->next;

#define update_addr(trans) \
	new_instr->addr = trans->curr_addr; \
	trans->curr_addr += ret;

static int handle_line(translator_t* trans, const char* str,
		       struct instr_el_t* old_instr) {
	regmatch_t pmatch[PMATCH_SIZE];
	unsigned int i;
	int ret;
	bool labels_pending;
	struct instr_el_t* new_instr;
	struct instr_el_t* last_instr;
	char match_buff[MAX_LINE_SIZE] = { 0 };
	subinstr_t* s;

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
		add_lbl(trans, match_buff, strlen(match_buff), new_instr);
		clean_pmatch(match_buff, 1);

		return 0;
	}

	REGEXEC(lbi_re) {

		if (!labels_pending)
			last_instr->labels_pending = true;

		get_pmatch_to(match_buff, 1);
		ttrace("Complex label %s (%ld)", match_buff,
		       strlen(match_buff));
		add_lbl(trans, match_buff, strlen(match_buff), new_instr);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ret = handle_line(trans, match_buff, new_instr);
		if (ret < 0)
			return ret;
		clean_pmatch(match_buff, 2);

		return 0;
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
		ret = handle_arg(trans, new_instr, match_buff);
		if (ret < 0)
			return ret;
		clean_pmatch(match_buff, 2);

		update_addr(trans);

		return 0;
	}

	REGEXEC(sin_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Simple instruction: %s", match_buff);
		set_instr_name(new_instr, match_buff);
		clean_pmatch(match_buff, 1);

		set_opcode_and_length(s, new_instr, 0);

		update_addr(trans);

		return ret;
	}

	REGEXEC(bra_re) {
		get_pmatch_to(match_buff, 1);
		ttrace("Branch / Jump: %s", match_buff);
		set_instr_name(new_instr, match_buff);
		clean_pmatch(match_buff, 1);

		get_pmatch_to(match_buff, 2);
		ttrace("Label: %s", match_buff);
		clean_pmatch(match_buff, 2);

		set_opcode_and_length(s, new_instr, strncmp(new_instr->name, "JMP", 3) ? 0 : MODE_ABSOLUTE);
		if (ret < 0)
			return ret;
		update_addr(trans);

		return 0;
	}

	return -1;
}

/* ========= debug functions ========= */

static void dump_instr_list(translator_t* trans) {
	struct instr_el_t* curr;

	for_each_instr_el(trans, curr) {
		ttrace("%.4x: %c%c%c %u", curr->addr, instr_name_to_chars(curr), curr->arg);
	}
}

/* ========= translation main ========= */

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
	trans.curr_addr = load_addr;
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

		if (ret) {
			logt_err("Syntax error at line %d: %s",
				 line, line_buffer);

			break;
		}

		for (i = 0; i <= last; i++)
			line_buffer[i] = 0;
	}

	dump_instr_list(&trans);

	fclose(f);

	translator_deinit(&trans);

	return ret;
}
