# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(pyocdprog.c)
zephyr_library_sources_ifdef(CONFIG_PYOCDPROG_MAIN pyocdprog_main.c)
zephyr_library_sources_ifdef(CONFIG_PYOCDPROG_SHELL pyocdprog_shell.c)
