#include "instr.h"

void run_cpu(void) {
	DEFINE_INSTR_MAP(instr_map);
	populate_imap(instr_map);
	print_imap(instr_map);
}

int main(int argc, const char* argv[]) {

	run_cpu();

	return 0;
}
