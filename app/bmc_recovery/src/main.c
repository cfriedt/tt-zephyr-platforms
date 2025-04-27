/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pyocdprog/pyocdprog.h>

int main()
{
	if (IS_ENABLED(CONFIG_PYOCDPROG_MAIN)) {
		return pyocdprog_main();
	}

	return 0;
}
