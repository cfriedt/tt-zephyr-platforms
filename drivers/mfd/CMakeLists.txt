# Copyright 2025 Tenstorrent AI ULC
# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_MFD_TT_BH_PVT mfd_tt_bh_pvt.c)
# zephyr-keep-sorted-stop
