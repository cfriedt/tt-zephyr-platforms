# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bh_pvt_temp.c)
zephyr_library_sources_ifdef(CONFIG_TT_BH_PVT_TEMP_TRIGGER bh_pvt_temp_trigger.c)
