/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tenstorrent_bh_pvt_vmon

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bh_temp, CONFIG_SENSOR_LOG_LEVEL);

struct bh_temp_config {
	uint16_t sample_period_ms;
	uint8_t inst;
};

struct bh_temp_data {
	struct sensor_value value;
};

static int bh_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bh_temp_data *const data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		LOG_ERR("invalid channel %d", chan);
		return -EINVAL;
	}

	*val = data->value;

	return -ENOSYS;
}

static int bh_temp_channel_get(const struct device *dev, enum sensor_channel chan,
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

static DEVICE_API(sensor, bh_temp_api) = {
	.sample_fetch = bh_temp_sample_fetch,
	.channel_get = bh_temp_channel_get,
};

bool pvt_is_configured;

static int bh_temp_init(const device *dev)
{
	int ret;
	const struct bh_temp_config *config = dev->config;
	struct bh_temp_data *data = dev->data;

	return 0;
}

#define DEFINE_BH_TEMP(_inst)                                                                      \
	static struct bh_temp_data bh_temp_data_##_inst;                                           \
                                                                                                   \
	static const struct bh_temp_config bh_temp_config_##_inst = {                              \
		.sample_period_ms = DT_INST_PROP(_inst, sample_period_ms),                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(index, &bh_temp_init, NULL, &bh_temp_data_##_inst,            \
				     &bh_temp_config_##_inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &bh_temp_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BH_TEMP)
