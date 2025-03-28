/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tenstorrent_bh_pvt_temp

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bh_pvt_temp, CONFIG_SENSOR_LOG_LEVEL);

struct bh_pvt_temp_config {
	uint16_t sample_period_ms;
	uint8_t inst;
};

struct bh_pvt_temp_data {
	struct sensor_value value;
};

static int bh_pvt_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bh_pvt_temp_data *const data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		LOG_ERR("invalid channel %d", chan);
		return -EINVAL;
	}

	*val = data->value;

	return 0;
}

static int bh_pvt_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		LOG_ERR("invalid channel %d", chan);
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(sensor, bh_pvt_temp_api) = {
	.sample_fetch = bh_pvt_temp_sample_fetch, .channel_get = bh_pvt_temp_channel_get,
	/* TODO: support setting attributes at runtime (e.g. sample rate) */
};

static int bh_pvt_temp_init(const device *dev)
{
	int ret;
	struct bh_pvt_temp_data *data = dev->data;
	const struct bh_pvt_temp_config *config = dev->config;

	SdifWrite(PVT_CNTL_TS_CMN_SDIF_STATUS_REG_ADDR, PVT_CNTL_TS_CMN_SDIF_REG_ADDR, IP_TMR_ADDR,
		  0x100); /* 256 cycles for TS */
	SdifWrite(PVT_CNTL_TS_CMN_SDIF_STATUS_REG_ADDR, PVT_CNTL_TS_CMN_SDIF_REG_ADDR, IP_CFG0_ADDR,
		  0x0); /* use 12-bit resolution, MODE_RUN_0 */
	SdifWrite(PVT_CNTL_TS_CMN_SDIF_STATUS_REG_ADDR, PVT_CNTL_TS_CMN_SDIF_REG_ADDR, IP_CNTL_ADDR,
		  0x108); /* ip_run_cont */

	return 0;
}

#define DEFINE_BH_PVT_TEMP(_inst)                                                                  \
	static struct bh_pvt_temp_data bh_pvt_temp_data_##_inst;                                   \
                                                                                                   \
	static const struct bh_pvt_temp_config bh_pvt_temp_config_##_inst = {                      \
		.sample_period_ms = DT_INST_PROP(_inst, sample_period_ms),                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(index, &bh_pvt_temp_init, NULL, &bh_pvt_temp_data_##_inst,    \
				     &bh_pvt_temp_config_##_inst, POST_KERNEL,                     \
				     CONFIG_SENSOR_INIT_PRIORITY, &bh_pvt_temp_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BH_PVT_TEMP)
