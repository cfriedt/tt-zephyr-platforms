
/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pyocdprog/pyocdprog.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(pyocdprog);

extern volatile enum pyocdprog_op pyocdprog_op;
extern volatile struct pyocdprog_args pyocdprog_args;
extern volatile int pyocdprog_result;
extern volatile uint8_t pyocdprog_buffer[CONFIG_PYOCDPROG_BUFFER_COUNT]
					[CONFIG_PYOCDPROG_BUFFER_SIZE];
extern volatile uint32_t pyocdprog_data[CONFIG_PYOCDPROG_BUFFER_COUNT];

int pyocdprog_main(void)
{
	LOG_DBG("load_address: %p", (void *)(uintptr_t)CONFIG_SRAM_BASE_ADDRESS);
	LOG_DBG("pc_init: %p", pyocdprog_op_init);
	LOG_DBG("pc_unInit: %p", pyocdprog_op_uninit);
	LOG_DBG("pc_compute_crc: %p", pyocdprog_op_compute_crc);
	LOG_DBG("pc_program_page: %p", pyocdprog_op_program_page);
	LOG_DBG("pc_erase_sector: %p", pyocdprog_op_erase_sector);
	LOG_DBG("pc_eraseAll: %p", pyocdprog_op_erase_all);
	LOG_DBG("begin_stack: %p",
		(void *)(uintptr_t)(CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE));
	LOG_DBG("begin_data: %p", (void *)pyocdprog_data); /* I think this is mainly just used for
							      crc's for each buffer? */
	LOG_DBG("buffer_size: %u", CONFIG_PYOCDPROG_BUFFER_SIZE);
	LOG_DBG("buffer_count: %u", CONFIG_PYOCDPROG_BUFFER_COUNT);
	ARRAY_FOR_EACH(pyocdprog_buffer, i) {
		LOG_DBG("page_buffers[%u]: %p", i, (void *)&pyocdprog_buffer[i][0]);
	}
	LOG_DBG("min_program_length: %u", CONFIG_PYOCDPROG_BUFFER_SIZE);

	do {
		k_msleep(CONFIG_PYOCDPROG_POLLING_INTERVAL_MS);

		enum pyocdprog_op op = pyocdprog_op;
		pyocdprog_op = PYOCDPROG_OP_NONE;

		switch (op) {
		case PYOCDPROG_OP_INIT:
            pyocdprog_result = pyocdprog_op_init(pyocdprog_args.r0, pyocdprog_args.r1, pyocdprog_args.r2);
			break;
		case PYOCDPROG_OP_UNINIT:
            pyocdprog_result = pyocdprog_op_uninit(pyocdprog_args.r0);
			break;
		case PYOCDPROG_OP_COMPUTE_CRC:
            pyocdprog_result = pyocdprog_op_compute_crc(pyocdprog_args.r0, pyocdprog_args.r1);
			break;
		case PYOCDPROG_OP_PROGRAM_PAGE:
            pyocdprog_result = pyocdprog_op_program_page(pyocdprog_args.r0, pyocdprog_args.r1, pyocdprog_args.r2);
			break;
		case PYOCDPROG_OP_ERASE_SECTOR:
            pyocdprog_result = pyocdprog_op_erase_sector(pyocdprog_args.r0);
			break;
		case PYOCDPROG_OP_ERASE_ALL:
            pyocdprog_result = pyocdprog_op_erase_all();
			break;
		case PYOCDPROG_OP_NONE:
            pyocdprog_result = 0;
			break;
		default:
            pyocdprog_result = -ENOTSUP;
			break;
		};

	} while (true);

	return 0;
}
