# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bh_pvt_vmon.c)
zephyr_library_sources_ifdef(CONFIG_TT_BH_PVT_VMON_TRIGGER bh_pvt_vmon_trigger.c)
