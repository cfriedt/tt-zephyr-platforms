# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory(tenstorrent)
# zephyr-keep-sorted-end

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_PYOCDPROG pyocdprog)
# zephyr-keep-sorted-end
