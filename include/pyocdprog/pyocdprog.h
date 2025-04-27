/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PYOCDPROG_PYOCDPROG_H_
#define PYOCDPROG_PYOCDPROG_H_

#include <stdint.h>

#ifdef __ZEPHYR__
#include <zephyr/sys/util.h>
#else
#ifndef BIT
#define BIT(x) (1UL << (x))
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PYOCDPROG_TIMEOUT_ERROR -1

enum pyocdprog_initop {
	PYOCDPROG_INITOP_ERASE = 1,
	PYOCDPROG_INITOP_PROGRAM = 2,
	PYOCDPROG_INITOP_VERIFY = 3,
};

enum pyocdprog_op {
	PYOCDPROG_OP_NONE = 0,
	PYOCDPROG_OP_INIT = BIT(0),
	PYOCDPROG_OP_UNINIT = BIT(1),
	PYOCDPROG_OP_COMPUTE_CRC = BIT(2),
	PYOCDPROG_OP_PROGRAM_PAGE = BIT(3),
	PYOCDPROG_OP_ERASE_SECTOR = BIT(4),
	PYOCDPROG_OP_ERASE_ALL = BIT(5),
};

struct pyocdprog_args {
	uintptr_t r0;
	uintptr_t r1;
	uintptr_t r2;
	uintptr_t r3;
};

void pyocdprog_set_op_and_args(enum pyocdprog_op op, uintptr_t r0, uintptr_t r1, uintptr_t r2,
			       uintptr_t r3);
int pyocdprog_get_result(void);

int pyocdprog_op_init(uintptr_t address, uintptr_t clock, enum pyocdprog_initop value);
int pyocdprog_op_uninit(enum pyocdprog_initop value);
uint32_t pyocdprog_op_compute_crc(uintptr_t begin_data, uintptr_t len);
int pyocdprog_op_program_page(uintptr_t address, uintptr_t len, uintptr_t begin_data);
int pyocdprog_op_erase_sector(uintptr_t address);
int pyocdprog_op_erase_all(void);

int pyocdprog_main(void);

static inline const char *const pyocdprog_initop_to_string(enum pyocdprog_initop op)
{
	switch (op) {
	case PYOCDPROG_INITOP_ERASE:
		return "ERASE";
	case PYOCDPROG_INITOP_PROGRAM:
		return "PROGRAM";
	case PYOCDPROG_INITOP_VERIFY:
		return "VERIFY";
	default:
		return "unknown";
	}
}

static inline const char *const pyocdprog_op_to_string(enum pyocdprog_op op)
{
	switch (op) {
	case PYOCDPROG_OP_INIT:
		return "INIT";
	case PYOCDPROG_OP_UNINIT:
		return "UNINIT";
	case PYOCDPROG_OP_COMPUTE_CRC:
		return "COMPUTE_CRC";
	case PYOCDPROG_OP_PROGRAM_PAGE:
		return "PROGRAM_PAGE";
	case PYOCDPROG_OP_ERASE_SECTOR:
		return "ERASE_SECTOR";
	case PYOCDPROG_OP_ERASE_ALL:
		return "ERASE_ALL";
	default:
		return "unknown";
	}
}

#ifdef __cplusplus
}
#endif

#endif /* PYOCDPROG_PYOCDPROG_H_ */
