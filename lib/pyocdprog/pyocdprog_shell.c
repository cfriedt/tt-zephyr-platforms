/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <pyocdprog/pyocdprog.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

extern volatile uint8_t pyocdprog_buffer[CONFIG_PYOCDPROG_BUFFER_COUNT]
					[CONFIG_PYOCDPROG_BUFFER_SIZE];

static int pyocdprog_op_init_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);

	return pyocdprog_op_init(strtoul(argv[1], NULL, 0), strtoul(argv[2], NULL, 0),
				 strtoul(argv[3], NULL, 0));
}

static int pyocdprog_op_uninit_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);

	return pyocdprog_op_uninit(strtoul(argv[1], NULL, 0));
}

static int pyocdprog_op_compute_crc_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	shell_print(shell, "%08x",
		    pyocdprog_op_compute_crc(strtoul(argv[1], NULL, 0), strtoul(argv[2], NULL, 0)));

	return 0;
}

static int pyocdprog_op_program_page_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);

	return pyocdprog_op_program_page(strtoul(argv[1], NULL, 0), strtoul(argv[2], NULL, 0),
					 strtoul(argv[3], NULL, 0));
}

static int pyocdprog_op_erase_sector_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);

	return pyocdprog_op_erase_sector(strtoul(argv[1], NULL, 0));
}

static int pyocdprog_op_erase_all_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return pyocdprog_op_erase_all();
}

static int pyocdprog_fill_buff(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0; i < CONFIG_PYOCDPROG_BUFFER_SIZE; i++) {
		pyocdprog_buffer[0][i] = i % 256;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	pyocdprog_cmds,
	SHELL_CMD_ARG(init, NULL, "<address> <clock> <value>", pyocdprog_op_init_cmd, 4, 0),
	SHELL_CMD_ARG(uninit, NULL, "<value>", pyocdprog_op_uninit_cmd, 2, 0),
	SHELL_CMD_ARG(compute_crc, NULL, "<begin_data> <len>", pyocdprog_op_compute_crc_cmd, 3, 0),
	SHELL_CMD_ARG(program_page, NULL, "<address> <len> <begin_data>",
		      pyocdprog_op_program_page_cmd, 4, 0),
	SHELL_CMD_ARG(erase_sector, NULL, "<address>", pyocdprog_op_erase_sector_cmd, 2, 0),
	SHELL_CMD_ARG(erase_all, NULL, "erase the entire flash device", pyocdprog_op_erase_all_cmd,
		      1, 0),
	SHELL_CMD_ARG(fill, NULL, "fill buffer with test pattern", pyocdprog_fill_buff, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pyocdprog, &pyocdprog_cmds, "pyocdprog commands", NULL);
