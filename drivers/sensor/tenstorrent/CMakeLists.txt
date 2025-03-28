# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_TT_BH_PVT_VMON bh_pvt_vmon)
add_subdirectory_ifdef(CONFIG_TT_BH_PVT_TEMP bh_pvt_temp)
# zephyr-keep-sorted-stop
