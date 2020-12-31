#include <stdlib.h>

#include "cpu.h"
#include "instr.h"

subinstr_t adc_list[] = {
	(subinstr_t) {
		.opcode = 0x69,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x65,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x75,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x6d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x7d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x79,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x61,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x71,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t and_list[] = {
	(subinstr_t) {
		.opcode = 0x29,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x25,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x35,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x2d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x3d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x39,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x21,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x31,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t asl_list[] = {
	(subinstr_t) {
		.opcode = 0xa,
		.cycles = 2,
		.length = 1,
		.mode = MODE_ACCUMULATOR,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x6,
		.cycles = 5,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x16,
		.cycles = 6,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xe,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x1e,
		.cycles = 7,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bit_list[] = {
	(subinstr_t) {
		.opcode = 0x24,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x2c,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bpl_single[] = {
	(subinstr_t) {
		.opcode = 0x10,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bmi_single[] = {
	(subinstr_t) {
		.opcode = 0x30,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bvc_single[] = {
	(subinstr_t) {
		.opcode = 0x50,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bvs_single[] = {
	(subinstr_t) {
		.opcode = 0x70,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bcc_single[] = {
	(subinstr_t) {
		.opcode = 0x90,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bcs_single[] = {
	(subinstr_t) {
		.opcode = 0xb0,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t bne_single[] = {
	(subinstr_t) {
		.opcode = 0xd0,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t beq_single[] = {
	(subinstr_t) {
		.opcode = 0xf0,
		.cycles = 2,
		.length = 2,
		.mode = MODE_BRANCH | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t brk_single[] = {
	(subinstr_t) {
		.opcode = 0x0,
		.cycles = 7,
		.length = 1,
		.mode = MODE_IMPLIED,
		.supported = CPU_6502_CORE
	}
};

subinstr_t cmp_list[] = {
	(subinstr_t) {
		.opcode = 0xc9,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xc5,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xd5,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xcd,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xdd,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xd9,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xc1,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xd1,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t cpx_list[] = {
	(subinstr_t) {
		.opcode = 0xe0,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xe4,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xec,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t cpy_list[] = {
	(subinstr_t) {
		.opcode = 0xc0,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xc4,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xcc,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t dec_list[] = {
	(subinstr_t) {
		.opcode = 0xc6,
		.cycles = 5,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xd6,
		.cycles = 6,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xce,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xde,
		.cycles = 7,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	}
};

subinstr_t eor_list[] = {
	(subinstr_t) {
		.opcode = 0x49,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x45,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x55,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x4d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x5d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x59,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x41,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x51,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t clc_single[] = {
	(subinstr_t) {
		.opcode = 0x18,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t sec_single[] = {
	(subinstr_t) {
		.opcode = 0x38,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t cli_single[] = {
	(subinstr_t) {
		.opcode = 0x58,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t sei_single[] = {
	(subinstr_t) {
		.opcode = 0x78,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t clv_single[] = {
	(subinstr_t) {
		.opcode = 0xb8,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t cld_single[] = {
	(subinstr_t) {
		.opcode = 0xd8,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t sed_single[] = {
	(subinstr_t) {
		.opcode = 0xf8,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STATUS,
		.supported = CPU_6502_CORE
	}
};

subinstr_t inc_list[] = {
	(subinstr_t) {
		.opcode = 0xe6,
		.cycles = 5,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xf6,
		.cycles = 6,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xee,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xfe,
		.cycles = 7,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	}
};

subinstr_t jmp_list[] = {
	(subinstr_t) {
		.opcode = 0x4c,
		.cycles = 3,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x6c,
		.cycles = 5,
		.length = 3,
		.mode = MODE_INDIRECT,
		.supported = CPU_6502_CORE
	}
};

subinstr_t jsr_single[] = {
	(subinstr_t) {
		.opcode = 0x20,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t lda_list[] = {
	(subinstr_t) {
		.opcode = 0xa9,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xa5,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xb5,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xad,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xbd,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xb9,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xa1,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xb1,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t ldx_list[] = {
	(subinstr_t) {
		.opcode = 0xa2,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xa6,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xb6,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_Y,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xae,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xbe,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t ldy_list[] = {
	(subinstr_t) {
		.opcode = 0xa0,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xa4,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xb4,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xac,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xbc,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t lsr_list[] = {
	(subinstr_t) {
		.opcode = 0x4a,
		.cycles = 2,
		.length = 1,
		.mode = MODE_ACCUMULATOR,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x46,
		.cycles = 5,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x56,
		.cycles = 6,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x4e,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x5e,
		.cycles = 7,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	}
};

subinstr_t nop_single[] = {
	(subinstr_t) {
		.opcode = 0xea,
		.cycles = 2,
		.length = 1,
		.mode = MODE_IMPLIED,
		.supported = CPU_6502_CORE
	}
};

subinstr_t ora_list[] = {
	(subinstr_t) {
		.opcode = 0x9,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x5,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x15,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xd,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x1d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x19,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x1,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x11,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t tax_single[] = {
	(subinstr_t) {
		.opcode = 0xaa,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t txa_single[] = {
	(subinstr_t) {
		.opcode = 0x8a,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t dex_single[] = {
	(subinstr_t) {
		.opcode = 0xca,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t inx_single[] = {
	(subinstr_t) {
		.opcode = 0xe8,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t tay_single[] = {
	(subinstr_t) {
		.opcode = 0xa8,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t tya_single[] = {
	(subinstr_t) {
		.opcode = 0x98,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t dey_single[] = {
	(subinstr_t) {
		.opcode = 0x88,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t iny_single[] = {
	(subinstr_t) {
		.opcode = 0xc8,
		.cycles = 2,
		.length = 1,
		.mode = MODE_REGISTER,
		.supported = CPU_6502_CORE
	}
};

subinstr_t rol_list[] = {
	(subinstr_t) {
		.opcode = 0x2a,
		.cycles = 2,
		.length = 1,
		.mode = MODE_ACCUMULATOR,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x26,
		.cycles = 5,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x36,
		.cycles = 6,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x2e,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x3e,
		.cycles = 7,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	}
};

subinstr_t ror_list[] = {
	(subinstr_t) {
		.opcode = 0x6a,
		.cycles = 2,
		.length = 1,
		.mode = MODE_ACCUMULATOR,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x66,
		.cycles = 5,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x76,
		.cycles = 6,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x6e,
		.cycles = 6,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x7e,
		.cycles = 7,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	}
};

subinstr_t rti_single[] = {
	(subinstr_t) {
		.opcode = 0x40,
		.cycles = 6,
		.length = 1,
		.mode = MODE_IMPLIED,
		.supported = CPU_6502_CORE
	}
};

subinstr_t rts_single[] = {
	(subinstr_t) {
		.opcode = 0x60,
		.cycles = 6,
		.length = 1,
		.mode = MODE_IMPLIED,
		.supported = CPU_6502_CORE
	}
};

subinstr_t sbc_list[] = {
	(subinstr_t) {
		.opcode = 0xe9,
		.cycles = 2,
		.length = 2,
		.mode = MODE_IMMEDIATE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xe5,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xf5,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xed,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xfd,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_X | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xf9,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xe1,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0xf1,
		.cycles = 5,
		.length = 2,
		.mode = MODE_INDIRECT_Y | MODE_EXTRA_CYCLE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t sta_list[] = {
	(subinstr_t) {
		.opcode = 0x85,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x95,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x8d,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x9d,
		.cycles = 5,
		.length = 3,
		.mode = MODE_ABSOLUTE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x99,
		.cycles = 5,
		.length = 3,
		.mode = MODE_ABSOLUTE_Y,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x81,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x91,
		.cycles = 6,
		.length = 2,
		.mode = MODE_INDIRECT_Y,
		.supported = CPU_6502_CORE
	}
};

subinstr_t txs_single[] = {
	(subinstr_t) {
		.opcode = 0x9a,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STACK,
		.supported = CPU_6502_CORE
	}
};

subinstr_t tsx_single[] = {
	(subinstr_t) {
		.opcode = 0xba,
		.cycles = 2,
		.length = 1,
		.mode = MODE_STACK,
		.supported = CPU_6502_CORE
	}
};

subinstr_t pha_single[] = {
	(subinstr_t) {
		.opcode = 0x48,
		.cycles = 3,
		.length = 1,
		.mode = MODE_STACK,
		.supported = CPU_6502_CORE
	}
};

subinstr_t pla_single[] = {
	(subinstr_t) {
		.opcode = 0x68,
		.cycles = 4,
		.length = 1,
		.mode = MODE_STACK,
		.supported = CPU_6502_CORE
	}
};

subinstr_t php_single[] = {
	(subinstr_t) {
		.opcode = 0x8,
		.cycles = 3,
		.length = 1,
		.mode = MODE_STACK,
		.supported = CPU_6502_CORE
	}
};

subinstr_t plp_single[] = {
	(subinstr_t) {
		.opcode = 0x28,
		.cycles = 4,
		.length = 1,
		.mode = MODE_STACK,
		.supported = CPU_6502_CORE
	}
};

subinstr_t stx_list[] = {
	(subinstr_t) {
		.opcode = 0x86,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x96,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_Y,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x8e,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	}
};

subinstr_t sty_list[] = {
	(subinstr_t) {
		.opcode = 0x84,
		.cycles = 3,
		.length = 2,
		.mode = MODE_ZERO_PAGE,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x94,
		.cycles = 4,
		.length = 2,
		.mode = MODE_ZERO_PAGE_X,
		.supported = CPU_6502_CORE
	},
	(subinstr_t) {
		.opcode = 0x8c,
		.cycles = 4,
		.length = 3,
		.mode = MODE_ABSOLUTE,
		.supported = CPU_6502_CORE
	}
};

instr_t instr_list[] = {
	(instr_t) {
		.name = "ADC",
		.list = adc_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "AND",
		.list = and_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "ASL",
		.list = asl_list,
		.size = 5,
		.action = NULL
	},
	(instr_t) {
		.name = "BIT",
		.list = bit_list,
		.size = 2,
		.action = NULL
	},
	(instr_t) {
		.name = "BPL",
		.list = bpl_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BMI",
		.list = bmi_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BVC",
		.list = bvc_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BVS",
		.list = bvs_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BCC",
		.list = bcc_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BCS",
		.list = bcs_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BNE",
		.list = bne_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BEQ",
		.list = beq_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "BRK",
		.list = brk_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "CMP",
		.list = cmp_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "CPX",
		.list = cpx_list,
		.size = 3,
		.action = NULL
	},
	(instr_t) {
		.name = "CPY",
		.list = cpy_list,
		.size = 3,
		.action = NULL
	},
	(instr_t) {
		.name = "DEC",
		.list = dec_list,
		.size = 4,
		.action = NULL
	},
	(instr_t) {
		.name = "EOR",
		.list = eor_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "CLC",
		.list = clc_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "SEC",
		.list = sec_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "CLI",
		.list = cli_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "SEI",
		.list = sei_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "CLV",
		.list = clv_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "CLD",
		.list = cld_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "SED",
		.list = sed_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "INC",
		.list = inc_list,
		.size = 4,
		.action = NULL
	},
	(instr_t) {
		.name = "JMP",
		.list = jmp_list,
		.size = 2,
		.action = NULL
	},
	(instr_t) {
		.name = "JSR",
		.list = jsr_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "LDA",
		.list = lda_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "LDX",
		.list = ldx_list,
		.size = 5,
		.action = NULL
	},
	(instr_t) {
		.name = "LDY",
		.list = ldy_list,
		.size = 5,
		.action = NULL
	},
	(instr_t) {
		.name = "LSR",
		.list = lsr_list,
		.size = 5,
		.action = NULL
	},
	(instr_t) {
		.name = "NOP",
		.list = nop_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "ORA",
		.list = ora_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "TAX",
		.list = tax_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "TXA",
		.list = txa_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "DEX",
		.list = dex_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "INX",
		.list = inx_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "TAY",
		.list = tay_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "TYA",
		.list = tya_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "DEY",
		.list = dey_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "INY",
		.list = iny_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "ROL",
		.list = rol_list,
		.size = 5,
		.action = NULL
	},
	(instr_t) {
		.name = "ROR",
		.list = ror_list,
		.size = 5,
		.action = NULL
	},
	(instr_t) {
		.name = "RTI",
		.list = rti_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "RTS",
		.list = rts_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "SBC",
		.list = sbc_list,
		.size = 8,
		.action = NULL
	},
	(instr_t) {
		.name = "STA",
		.list = sta_list,
		.size = 7,
		.action = NULL
	},
	(instr_t) {
		.name = "TXS",
		.list = txs_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "TSX",
		.list = tsx_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "PHA",
		.list = pha_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "PLA",
		.list = pla_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "PHP",
		.list = php_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "PLP",
		.list = plp_single,
		.size = 1,
		.action = NULL
	},
	(instr_t) {
		.name = "STX",
		.list = stx_list,
		.size = 3,
		.action = NULL
	},
	(instr_t) {
		.name = "STY",
		.list = sty_list,
		.size = 3,
		.action = NULL
	}
};

instr_t* get_instr_list(void) {
	return instr_list;
}

size_t get_instr_list_size(void) {
	return sizeof(instr_list) / sizeof(*instr_list);
}
