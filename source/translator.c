#include "translator.h"

#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> /* strerror */

#include "instr.h"
#include "common.h"

#define TSIG "TRA"

#define logt_err(FMT, ...) log_err(TSIG, FMT, ## __VA_ARGS__)

#define MIN_LINE_SIZE 16
#define MAX_INSTR_LENGTH 3
#define PMATCH_SIZE 3

#define TRANS_ERROR_FAIL -2
#define TRANS_ERROR_MAYBE_LABEL -1
#define TRANS_ERROR_LABEL_NOT_FOUND -3

#ifdef TRANSLATOR_TRACE
#define ttrace(FMT, ...) trace(FMT, ## __VA_ARGS__)
#define ttracei(FMT, ...) tracei(TSIG, FMT, ## __VA_ARGS__)
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

#define LBL_EMP_RE RE_BEGIN "\\([a-zA-Z0-9_]\\+\\):" RE_END

#define INS_RE_STR "\\([a-zA-Z]..\\)"
#define SIN_RE_STR RE_BEGIN INS_RE_STR RE_END
#define CIN_RE_STR RE_BEGIN INS_RE_STR RE_SEP "\\(.\\+\\)" RE_END

#define GEN_ADR_RE(REGEX) RE_BEGIN REGEX RE_END
#define GEN_AXY_RE(REGEX) RE_BEGIN REGEX "\\s*,\\s*\\([XY]\\)" RE_END
#define IND_ADX_RE(REGEX) RE_BEGIN "(\\s*" REGEX "\\s*,\\s*\\X\\s*)" RE_END
#define IND_ADY_RE(REGEX) RE_BEGIN "(\\s*" REGEX "\\s*)\\s*,\\s*Y" RE_END

#define ACC_REGEX RE_BEGIN "\\(ASL\\|LSR\\|ROL\\|ROR\\)" RE_SEP "A" RE_END

#define REGCOMP(VAR, NAME) \
	do { \
		ret = regcomp(&(trans->VAR), NAME, REG_NEWLINE); \
		if (ret) { \
			logt_err("Could not compile " #NAME " regex."); \
			return ret; \
		} \
	} while (0)

#define get_pmatch_to(buff, match) do { \
	i = 0; \
	ret = 0; \
	while ((i < pmatch[match].rm_eo - pmatch[match].rm_so)) { \
		ret = appendc(&buff, &i, &size, \
			      str[i + pmatch[match].rm_so]); \
		if (ret) break; \
	} \
	if (!ret) appendc(&buff, &i, &size, '\0'); \
} while (0)

#define check_pmatch(end_lbl) do { \
	if (ret < 0) { \
		ret = TRANS_ERROR_FAIL; \
		goto end_lbl; \
	} \
} while(0)

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
	uint16_t arg;
	uint16_t addr;
	uint8_t length;

	char *label_pending;
	struct instr_el_t* next;
};

struct lbl_node_t {
	char *label;
	unsigned int addr;
	struct lbl_node_t* left;
	struct lbl_node_t* right;
};

typedef struct {
	regex_t emp_re;

	regex_t sin_re;
	regex_t cin_re;
	regex_t lbe_re;

	regex_t imm_re;

	regex_t gen_re;
	regex_t gxy_re;
	regex_t inx_re;
	regex_t iny_re;

	regex_t adr_re;

	regex_t acc_re;

	unsigned int load_addr;

	unsigned int curr_addr;
	unsigned int total_len;

	struct instr_el_t* instr_head;
	struct instr_el_t* last_instr;
	struct lbl_node_t* label_pool;

	const char* infile;

} translator_t;

/* ========= translator init / deinit ========= */

static int translator_init(translator_t* trans) {
	int ret;

	REGCOMP(emp_re, RE_EMPTY);

	REGCOMP(lbe_re, LBL_EMP_RE);
	REGCOMP(sin_re, SIN_RE_STR);
	REGCOMP(cin_re, CIN_RE_STR);

	REGCOMP(imm_re, IMM_ADR_RE);

	REGCOMP(gen_re, GEN_ADR_RE("\\(.\\+\\)"));
	REGCOMP(gxy_re, GEN_AXY_RE("\\(.\\+\\)"));
	REGCOMP(inx_re, IND_ADX_RE("\\(.\\+\\)"));
	REGCOMP(iny_re, IND_ADY_RE("\\(.\\+\\)"));

	REGCOMP(adr_re, RE_BEGIN ADR_FMT_RE RE_END);

	REGCOMP(acc_re, ACC_REGEX);

	trans->instr_head = NULL;
	trans->last_instr = NULL;
	trans->label_pool = NULL;

	return ret;
}

/* ========= label operations ========= */

static void lbl_init(struct lbl_node_t* lbl, const char* name,
		     unsigned int addr) {

	lbl->label = malloc(sizeof(*(lbl->label)) * (strlen(name) + 1));

	strcpy(lbl->label, name);

	lbl->left = NULL;
	lbl->right = NULL;

	lbl->addr = addr;

	return;
}

static void free_lbl(struct lbl_node_t* lbl) {
	struct lbl_node_t* temp_lbl;

	temp_lbl = lbl;
	free(temp_lbl->label);
	free(temp_lbl);

	return;
}

static bool add_lbl(translator_t* trans, const char* name) {
	struct lbl_node_t* new_lbl;
	struct lbl_node_t* curr;
	int res;

	new_lbl = malloc(sizeof(*new_lbl));

	lbl_init(new_lbl, name, trans->curr_addr);

	curr = trans->label_pool;

	while (curr) {
		res = strcmp(curr->label, new_lbl->label);

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

static int get_lbl_addr(translator_t* trans, const char* name) {
	struct lbl_node_t* curr;
	int res;

	curr = trans->label_pool;

	while (curr) {
		res = strcmp(name, curr->label);

		if (res > 0) {
			if (curr->left)
				curr = curr->left;
			else
				return TRANS_ERROR_LABEL_NOT_FOUND;
		}
		else if (res < 0) {
			if (curr->right)
				curr = curr->right;
			else
				return TRANS_ERROR_LABEL_NOT_FOUND;
		}
		else
			return curr->addr;
	}

	return TRANS_ERROR_LABEL_NOT_FOUND;
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

/* ========= instruction operations ========= */

static void instr_init(struct instr_el_t* iel) {
	unsigned int i = 0;

	iel->next = NULL;
	iel->label_pending = NULL;
	iel->length = 0;
	iel->arg = 0;
	iel->addr = 0;

	for (i = 0; i < 3; i++)
		iel->name[i] = 0;

	return;
}

static struct instr_el_t* add_empty_instr(translator_t* trans) {
	struct instr_el_t* new_instr;

	ttrace("Allocating memory for the new instruction.");

	new_instr = malloc(sizeof(*new_instr));
	instr_init(new_instr);

	if (!trans->instr_head) {
		trans->instr_head = new_instr;
		trans->last_instr = new_instr;

		return new_instr;
	}

	trans->last_instr->next = new_instr;
	trans->last_instr = new_instr;

	return new_instr;
}

static void set_instr_name(struct instr_el_t* iel, const char* name) {
	unsigned int i;

	for (i = 0; i < 3; i++)
		iel->name[i] = name[i];

	return;
}

static void remove_pending_label(struct instr_el_t* iel) {
	char* lp;

	lp = iel->label_pending;
	free(lp);
	iel->label_pending = NULL;

	return;
}

static int translate_instr(translator_t* trans, struct instr_el_t* iel,
			   uint8_t mcode[MAX_INSTR_LENGTH]) {
	int ret;

	if (iel->label_pending) {

		ret = get_lbl_addr(trans, iel->label_pending);

		if (ret == TRANS_ERROR_LABEL_NOT_FOUND) {
			logt_err("Label %s not found!", iel->label_pending);

			return ret;
		}
		else {
			iel->arg = (uint16_t)ret;
			ttrace("Found label: %s (%.4x).",
			       iel->label_pending, iel->arg);
			remove_pending_label(iel);
		}
	}

	mcode[0] = (uint8_t)iel->opcode;

	switch (iel->length) {
	case 2:
		mcode[1] = (uint8_t)(iel->arg & 0xFF);
		break;
	case 3:
		mcode[1] = (uint8_t)(iel->arg & 0xFF);
		mcode[2] = (uint8_t)((iel->arg & 0xFF00) >> 8);
		break;
	default:
		break;
	}
/*
	for (i = 1; i < iel->length; i++) {
		s = (((iel->length - i) - 1) * 8);
		mcode[i] = (uint8_t)(((iel->arg &
			  ((uint16_t)0xFF << s)) >> s));
	}
*/
	return 0;
}

static void free_ins(struct instr_el_t* instr) {
	struct instr_el_t* temp_el;

	ttrace("Freeing instruction: %c%c%c", instr_name_to_chars(instr));

	temp_el = instr;

	if (instr->label_pending)
		free(instr->label_pending);

	free(temp_el);

	return;
}

/* ========= deinit translator ========= */

static void translator_deinit(translator_t* trans) {
	struct instr_el_t* curr_ins;
	struct instr_el_t* next_ins;

	ttracei("Deinitializing translator.");

	curr_ins = trans->instr_head;

	regfree(&(trans->emp_re));

	regfree(&(trans->cin_re));
	regfree(&(trans->sin_re));
	regfree(&(trans->lbe_re));

	regfree(&(trans->imm_re));

	regfree(&(trans->gen_re));
	regfree(&(trans->gxy_re));
	regfree(&(trans->inx_re));
	regfree(&(trans->iny_re));

	regfree(&(trans->adr_re));

	regfree(&(trans->acc_re));

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

/* ========= address handling  ========= */

static void on_err(int errno) {
	logt_err("%s", strerror(errno));

	return;
}

static int parse_arg(const char* str) {

	return parse_str(str, 16, on_err);
}

static int get_arg(translator_t* trans, const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	int ret, size, i;
	char* match_buff;

	ret = 0;
	i = 0;
	match_buff = NULL;
	size = MIN_LINE_SIZE;

	match_buff = malloc(sizeof(*match_buff) * size);
	if (!match_buff) {
		logt_err("Could not allocate memory");
		ret = TRANS_ERROR_FAIL;
		goto end_get_arg;
	}

	REGEXEC(adr_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_get_arg);
		ttrace("Got pmatch: %s", match_buff);

		ret = parse_arg(match_buff);
		check_pmatch(end_get_arg);
		ttrace("Got address: %.4x", ret);

		goto end_get_arg;
	}

	ret = TRANS_ERROR_MAYBE_LABEL;

end_get_arg:
	if (match_buff)
		free(match_buff);

	return ret;
}

#define set_new_instr_arg(buff) \
	ret = get_arg(trans, buff); \
	if (ret == TRANS_ERROR_FAIL) return ret; \
	else if (ret > 0) new_instr->arg = ret;

static int get_length_set_opcode(struct instr_el_t* new_instr,
				 instr_mode_t mode) {
	subinstr_t* s;
	int ret;

	if (!get_subinstr(new_instr->name, mode, &s)) {
		logt_err("Opcode for %c%c%c not found",
			 instr_name_to_chars(new_instr));
		ret = -1;
	}
	else {
		ttrace("Setting opcode for %c%c%c",
			instr_name_to_chars(new_instr));
		new_instr->opcode = s->opcode;
		ret = s->length;
	}

	return ret;
}

#define set_label_pending(new_instr, buff) \
	ttrace("Setting label of size %lu", strlen(buff)); \
	(new_instr)->label_pending = malloc(strlen(buff) + 1); \
	memcpy((new_instr)->label_pending, buff, strlen(buff)); \
	(new_instr)->label_pending[strlen(buff)] = 0;

static int handle_arg(translator_t* trans, struct instr_el_t* new_instr,
		      const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	int ret, size, i;
	char* match_buff;
	instr_mode_t mode;

	ret = 0;
	size = MIN_LINE_SIZE;
	match_buff = NULL;

	match_buff = malloc(sizeof(*match_buff) * size);
	if (!match_buff) {
		logt_err("Could not allocate memory.");
		ret = TRANS_ERROR_FAIL;
		goto end_handle_arg;
	}

	REGEXEC(imm_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_arg);
		ttrace("Immediate: %s", match_buff);

		ret = parse_arg(match_buff);
		check_pmatch(end_handle_arg);
		new_instr->arg = ret;

		ret = get_length_set_opcode(new_instr, MODE_IMMEDIATE);

		goto end_handle_arg;
	}

	REGEXEC(gen_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_arg);
		ttrace("Zero page / Absolute: %s", match_buff);
		set_new_instr_arg(match_buff);

		if (ret != TRANS_ERROR_MAYBE_LABEL) {
			mode = (new_instr->arg & 0xFF00)
			     ? MODE_ABSOLUTE : MODE_ZERO_PAGE; }
		else {
			mode = MODE_ABSOLUTE;
			set_label_pending(new_instr, match_buff);
		}

		ret = get_length_set_opcode(new_instr, mode);

		goto end_handle_arg;
	}

	REGEXEC(gxy_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_arg);
		ttrace("Zero page / Absolute: %s", match_buff);
		set_new_instr_arg(match_buff);

		get_pmatch_to(match_buff, 2);
		check_pmatch(end_handle_arg);
		ttrace("Register: %s", match_buff);
		if (ret != TRANS_ERROR_MAYBE_LABEL)
			mode = (new_instr->arg & 0xFF00)
			     ? (match_buff[0] == 'X'
					? MODE_ABSOLUTE_X
					: MODE_ABSOLUTE_Y)
			     : (match_buff[0] == 'X'
					? MODE_ZERO_PAGE_X
					: MODE_ZERO_PAGE_Y);
		else {
			mode = (match_buff[0] == 'X'
					? MODE_ABSOLUTE_X
					: MODE_ABSOLUTE_Y);
			set_label_pending(new_instr, match_buff);
		}

		ret = get_length_set_opcode(new_instr, mode);

		goto end_handle_arg;
	}

	REGEXEC(inx_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_arg);
		ttrace("Indirect X: %s", match_buff);

		set_new_instr_arg(match_buff);
		mode = MODE_INDIRECT_X;
		if (ret == TRANS_ERROR_MAYBE_LABEL)
			set_label_pending(new_instr, match_buff);

		ret = get_length_set_opcode(new_instr, mode);

		goto end_handle_arg;
	}

	REGEXEC(iny_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_arg);
		ttrace("Indirect Y: %s", match_buff);

		set_new_instr_arg(match_buff);
		mode = MODE_INDIRECT_Y;
		if (ret == TRANS_ERROR_MAYBE_LABEL)
			set_label_pending(new_instr, match_buff);
		ret = get_length_set_opcode(new_instr, mode);

		goto end_handle_arg;
	}

	ret = -1;

end_handle_arg:

	if (match_buff)
		free(match_buff);

	return ret;
}

/* ========= instruction / line handling ========= */

static void update_addr_and_length(translator_t* trans,
				   struct instr_el_t* new_instr,
				   int length) {
	ttrace("Updating address and length.");
	new_instr->length = length;
	new_instr->addr = trans->curr_addr;
	trans->curr_addr += length;
	trans->total_len += length;

	return;
}

static int handle_line(translator_t* trans, const char* str) {
	regmatch_t pmatch[PMATCH_SIZE];
	int ret, i, size;
	struct instr_el_t* new_instr = NULL;
	char* match_buff;

	ret = 0;
	i = 0;
	size = MIN_LINE_SIZE;
	match_buff = NULL;

	match_buff = malloc(sizeof(*match_buff) * size);
	if (!match_buff) {
		logt_err("Could not allocate memory.");
		ret = TRANS_ERROR_FAIL;
		goto end_handle_line;
	}

	ttracei("Parsing line \"%s\"", str);

	REGEXEC(emp_re) {
		ttrace("Empty line.");

		goto end_handle_line;
	}

	REGEXEC(lbe_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_line);
		ttrace("Empty label %s (%ld)", match_buff,
		       strlen(match_buff));
		add_lbl(trans, match_buff);

		goto end_handle_line;
	}

	new_instr = add_empty_instr(trans);

	REGEXEC(acc_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_line);
		ttrace("Accumulator operator: %s", match_buff);
		set_instr_name(new_instr, match_buff);

		ret = get_length_set_opcode(new_instr, MODE_ACCUMULATOR);
		check_pmatch(end_handle_line);
		update_addr_and_length(trans, new_instr, ret);

		goto end_handle_line;
	}

	REGEXEC(cin_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_line);
		ttrace("Complex instruction: %s", match_buff);
		set_instr_name(new_instr, match_buff);

		get_pmatch_to(match_buff, 2);
		check_pmatch(end_handle_line);
		ret = handle_arg(trans, new_instr, match_buff);
		if (new_instr->label_pending) {
			ttrace("Is %s a label?", match_buff);
		}
		else if (ret == TRANS_ERROR_FAIL) {
			goto end_handle_line;
		}

		update_addr_and_length(trans, new_instr, ret);

		goto end_handle_line;
	}

	REGEXEC(sin_re) {
		get_pmatch_to(match_buff, 1);
		check_pmatch(end_handle_line);
		ttrace("Simple instruction: %s", match_buff);
		set_instr_name(new_instr, match_buff);

		ret = get_length_set_opcode(new_instr, 0);
		check_pmatch(end_handle_line);
		update_addr_and_length(trans, new_instr, ret);

		goto end_handle_line;
	}

	ret = -1;

end_handle_line:

	if (match_buff)
		free(match_buff);

	return ret;
}

/* ========= debug functions ========= */

#ifdef TRANSLATOR_TRACE
static void dump_instr_list(translator_t* trans) {
	struct instr_el_t* curr;
	uint8_t mcode[MAX_INSTR_LENGTH] = { 0 };
	unsigned int mcode_print;
	uint8_t i;

	ttracei("Dumping instruction list for %s:", trans->infile);

	for_each_instr_el(trans, curr) {

		(void)translate_instr(trans, curr, mcode);
		mcode_print = 0;

		for (i = 0; i < curr->length; i++) {
			mcode_print <<= 8;
			mcode_print |= mcode[i];
		}

		ttrace("%.4x: %x (%c%c%c %u) (len=%u)", curr->addr,
		       mcode_print, instr_name_to_chars(curr),
		       curr->arg, curr->length);
	}
}
#endif

/* ========= translation main ========= */

static int dump_binary(translator_t* trans, uint8_t** out) {
	struct instr_el_t* curr;
	unsigned int pos;
	uint8_t i;
	uint8_t mcode[MAX_INSTR_LENGTH] = { 0 };
	int ret;

	*out = malloc(trans->total_len);
	pos = 0;

	memset(*out, 0, trans->total_len);

	for_each_instr_el(trans, curr) {

		ret = translate_instr(trans, curr, mcode);
		if (ret) {
			logt_err("Could not translate instruction.");
			free(*out);

			return ret;
		}

		for (i = 0; i < curr->length; i++)
			(*out)[pos++] = mcode[i];
	}

	return 0;
}

int translate(const char* infile, unsigned int load_addr,
	      int(*op)(unsigned int, const uint8_t*, void*),
	      void *data) {
	translator_t trans;
	int ret, curr, last, line, size;
	bool reading, entered_comment, started_newl;
	FILE* f;
	char *line_buffer = NULL;
	uint8_t *outdata;

	if (!op) {
		logt_err("No operation on binary has been provided.");

		return -1;
	}

	ret = 0;
	trans.load_addr = load_addr;
	trans.curr_addr = load_addr;
	trans.total_len = 0;
	trans.infile = infile;

	ret = translator_init(&trans);
	if (ret) {
		logt_err("Could not initialize translator.");

		return ret;
	}

	f = fopen(infile, "r");
	if (!f) {
		logt_err("Could not open file %s.", infile);

		return -1;
	}

	reading = true;
	line = 0;
	size = MIN_LINE_SIZE;
	line_buffer = NULL;
	line_buffer = malloc(sizeof(*line_buffer) * size);
	if (!line_buffer) {
		logt_err("Could not allocate memory.");
		ret = TRANS_ERROR_FAIL;
		goto exit_translator;
	}

	while (reading) {

		curr = 0;
		last = 0;
		entered_comment = false;
		started_newl = true;

		while (curr != '\n' && curr != EOF) {

			if (curr == ':' && !entered_comment)
				break;

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

			if ((!entered_comment) && curr != '\n' && curr != EOF) {
				ret = appendc(&line_buffer, &last, &size, curr);
				if (ret) {
					logt_err("Error appending char.");
					ret = TRANS_ERROR_FAIL;
					goto exit_translator;
				}

			}
		}

		line++;

		if (last <= 1)
			continue;

		ret = appendc(&line_buffer, &last, &size, '\0');
		if (ret) {
			logt_err("Error appending char.");
			ret = TRANS_ERROR_FAIL;
			goto exit_translator;
		}

		ret = handle_line(&trans, line_buffer);

		if (ret < 0) {
			logt_err("Syntax error at line %d: %s",
				 line, line_buffer);
			ret = TRANS_ERROR_FAIL;
			goto exit_translator;
		}
	}

#ifdef TRANSLATOR_TRACE
	dump_instr_list(&trans);
#endif

	if (dump_binary(&trans, &outdata))
		goto exit_translator;

	ret = op(trans.total_len, outdata, data);
	free(outdata);

exit_translator:

	if (line_buffer)
		free(line_buffer);

	fclose(f);

	translator_deinit(&trans);

	return ret;
}

typedef union {
	uint16_t arg;
	uint8_t mem[2];
} arg_conv_t;

struct disasm_list_t {
	char *line;
	struct disasm_list_t* next;
};

/* NOTE: pass mode with 0xF mask here */
static int add_string(struct disasm_list_t** head,
		      struct disasm_list_t** last,
		      disasm_mode_t dmode,
		      const char name[4], uint16_t arg,
		      const uint8_t* bytes, uint16_t instr_len,
		      instr_mode_t mode) {

	struct disasm_list_t* newd;
	int ret;
	char* instr_part;
	char mc_part[9] = { 0 };
	char arg_part[5] = { 0 };
	unsigned int size;

	newd = NULL;
	ret = 0;
	instr_part = NULL;
	size = MIN_LINE_SIZE;

	instr_part = malloc(sizeof(*instr_part) * size);
	if (!instr_part) {
		logt_err("Could not allocate memory.");
		return TRANS_ERROR_FAIL;
	}

	switch (instr_len) {
	case 1:
		sprintf(mc_part, "%.2x", bytes[0]);
		break;
	case 2:
		sprintf(mc_part, "%.2x %.2x", bytes[0], bytes[1]);
		sprintf(arg_part, "%.2x", (uint8_t)arg);
		break;
	case 3:
		sprintf(mc_part, "%.2x %.2x %.2x",
			bytes[0], bytes[1], bytes[2]);
		sprintf(arg_part, "%.4x", arg);
		break;
	default:
		/* properly handled in disassemble() */
		break;
	}

	switch (mode) {
	case MODE_ZERO_PAGE_X:
	case MODE_ZERO_PAGE_Y:
	case MODE_ABSOLUTE_X:
	case MODE_ABSOLUTE_Y:
		print_to_str(&instr_part, &size, "%s $%s, %c", name, arg_part,
		       (mode == MODE_ZERO_PAGE_X
		     || mode == MODE_ABSOLUTE_X)
		      ? 'X' : 'Y');
		break;
	case MODE_INDIRECT_X:
		print_to_str(&instr_part, &size, "%s ($%s, X)", name, arg_part);
		break;
	case MODE_INDIRECT_Y:
		print_to_str(&instr_part, &size, "%s ($%s), Y", name, arg_part);
		break;
	case MODE_INDIRECT:
		print_to_str(&instr_part, &size, "%s ($%s)", name, arg_part);
		break;
	case MODE_ACCUMULATOR:
		print_to_str(&instr_part, &size, "%s A", name);
		break;
	case MODE_IMMEDIATE:
		print_to_str(&instr_part, &size, "%s #$%s", name, arg_part);
		break;
	default:
		if (instr_len > 1)
			print_to_str(&instr_part, &size, "%s $%s", name, arg_part);
		else
			print_to_str(&instr_part, &size, "%s", name);
		break;
	}

	newd = malloc(sizeof(*newd));
	if (!newd) {
		logt_err("Could not allocate memory.");
		free(instr_part);
		return -1;
	}

	newd->next = NULL;
	newd->line = NULL;

	switch (dmode) {
	case DISASM_SIMPLE:
		print_to_str(&(newd->line), &size, "%s", instr_part);
		break;
	case DISASM_PRETTY:
		print_to_str(&(newd->line), &size, "%-8s\t%s", mc_part, instr_part);
		break;
	default:
		logt_err("Invalid disassembly mode.");
		return -1;
	}

	free(instr_part);

	if (!(newd->line)) {
		logt_err("Line was not written properly.");
		free(newd);
		return -1;
	}

	if (!(*last)) {
		*head = newd;
		*last = newd;
	}
	else {
		(*last)->next = newd;
		*last = newd;
	}

	return ret;
}

static void free_disasm(struct disasm_list_t* curr) {
	struct disasm_list_t* tmp;

	tmp = curr;
	free(tmp);

	return;
}
static void free_disasm_list(struct disasm_list_t* head) {
	struct disasm_list_t* curr;
	struct disasm_list_t* temp;

	curr = head;

	while (curr) {
		temp = curr->next;
		free(curr->line);
		free_disasm(curr);
		curr = temp;
	}

	return;
}

int disassemble(const char* infile, instr_map_t* map,
		disasm_mode_t disasm_mode) {
	uint8_t* bytes;
	unsigned int len;
	unsigned int i, j;
	uint8_t opc;
	char name[4];
	uint16_t arg;
	struct disasm_list_t* head;
	struct disasm_list_t* last;
	int ret;

	bytes = load_file(infile, &len);
	if (!bytes) {
		logt_err("Could not open file %s.", infile);
		return -1;
	}

	i = 0;
	name[3] = 0;
	head = NULL;
	last = NULL;

	while (i < len) {
		j = i;
		opc = bytes[i++];

		if (!map[opc].instr) {
			logt_err("Invalid opcode: %.2x", opc);
			free(bytes);
			return -1;
		}

		memcpy(name, map[opc].instr->name, 3);
		switch (map[opc].subinstr->length) {
		case 1:
			break;
		case 2:
			arg = (uint16_t)bytes[i++];
			break;
		case 3:
			arg = ((arg_conv_t*)(&(bytes[i])))->arg;
			i += 2;
			break;
		default:
			logt_err("Invalid length (%u) for %s (0x%.2x).",
				 map[opc].subinstr->length, name, opc);
			free(bytes);
			return -1;
		}

		ret = add_string(&head, &last, disasm_mode,
				 name, arg, &(bytes[j]),
				 map[opc].subinstr->length,
				 map[opc].subinstr->mode & 0xF);
		if (ret)
			goto exit_disasm;
	}

	last = head;

	while (last) {
		printf("%s\n", last->line);
		last = last->next;
	}

exit_disasm:

	if (head)
		free_disasm_list(head);

	if (bytes)
		free(bytes);

	return 0;
}
