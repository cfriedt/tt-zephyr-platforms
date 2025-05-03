/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <pyocdprog/pyocdprog.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

int main()
{
	if (IS_ENABLED(CONFIG_PYOCDPROG_MAIN)) {
		return pyocdprog_main();
	}

	return 0;
}
