/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <pyocdprog/pyocdprog.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/sem.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(pyocdprog, CONFIG_PYOCDPROG_LOG_LEVEL);

static const struct device *const flashdev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(flash1));

volatile enum pyocdprog_op pyocdprog_op;
volatile struct pyocdprog_args pyocdprog_args;
volatile int pyocdprog_result;

BUILD_ASSERT(CONFIG_PYOCDPROG_BUFFER_SIZE % sizeof(uint32_t) == 0,
	     "Invalid selection for CONFIG_PYOCDPROG_BUFFER_SIZE");
BUILD_ASSERT(CONFIG_PYOCDPROG_BUFFER_SIZE >= sizeof(uint32_t),
	     "Invalid selection for CONFIG_PYOCDPROG_BUFFER_SIZE");
BUILD_ASSERT(CONFIG_PYOCDPROG_BUFFER_COUNT >= 1,
	     "Invalid selection for CONFIG_PYOCDPROG_BUFFER_COUNT");

uint32_t __used pyocdprog_data[CONFIG_PYOCDPROG_BUFFER_COUNT];
uint8_t __used __aligned(256) pyocdprog_buffer[CONFIG_PYOCDPROG_BUFFER_COUNT]
					      [CONFIG_PYOCDPROG_BUFFER_SIZE];

int pyocdprog_op_init(uintptr_t address, uintptr_t clock, enum pyocdprog_initop value)
{
	/*
	 * TODO: possibly fill in this function to
	 * Set pinmux to SPI mode.
	 * Initialize the SPI controller
	 */

	LOG_DBG("init: address: 0x%lx, clock: %lu, value: %s", address, clock,
		pyocdprog_initop_to_string(value));

	return 0;
}

int pyocdprog_op_uninit(enum pyocdprog_initop value)
{
	/*
	 * TODO: possibly fill in this function to
	 * De-initialize the SPI controller
	 * Set pinmux to high impedence.
	 */

	LOG_DBG("uninit: value: %s", pyocdprog_initop_to_string(value));

	return 0;
}

uint32_t pyocdprog_op_compute_crc(uintptr_t begin_data, uintptr_t len)
{
	if ((begin_data < (uintptr_t)(void *)pyocdprog_buffer) ||
	    (begin_data >= (uintptr_t)pyocdprog_buffer + sizeof(pyocdprog_buffer))) {
		LOG_DBG("invalid address for data: 0x%lx", begin_data);
		return -EINVAL;
	}

	LOG_DBG("crc: begin_data: 0x%lx, len: %lu", begin_data, len);

	return crc32_ieee((uint8_t *)begin_data, len);
}

int pyocdprog_op_program_page(uintptr_t address, uintptr_t len, uintptr_t begin_data)
{
	int ret;

	if ((begin_data < (uintptr_t)(void *)pyocdprog_buffer) ||
	    (begin_data + len >= (uintptr_t)pyocdprog_buffer + sizeof(pyocdprog_buffer))) {
		LOG_DBG("invalid address for data: 0x%lx", begin_data);
		return -EINVAL;
	}

	LOG_DBG("program_page: address: 0x%lx, len: %lu, begin_data: 0x%lx", address, len,
		begin_data);

	ret = flash_write(flashdev, address, (void *)begin_data, len);
	if (ret < 0) {
		LOG_ERR("%s() failed: %d", "flash_write", ret);
		return ret;
	}

	return 0;
}

int pyocdprog_op_erase_sector(uintptr_t address)
{
	int ret;
	struct flash_pages_info page_info;

	LOG_DBG("erase_sector: address: 0x%lx", address);

	ret = flash_get_page_info_by_offs(flashdev, address, &page_info);
	if (ret < 0) {
		LOG_ERR("%s() failed: %d", "flash_get_page_info_by_offs", ret);
		return ret;
	}

	ret = flash_erase(flashdev, page_info.start_offset, page_info.size);
	if (ret < 0) {
		LOG_ERR("%s() failed: %d", "flash_erase", ret);
		return ret;
	}

	return 0;
}

int pyocdprog_op_erase_all(void)
{
	int ret;
	uint64_t flash_size;

	ret = flash_get_size(flashdev, &flash_size);
	if (ret < 0) {
		LOG_ERR("%s() failed: %d", "flash_get_size", ret);
		return ret;
	}

	ret = flash_erase(flashdev, 0, flash_size);
	if (ret < 0) {
		LOG_ERR("%s() failed: %d", "flash_erase", ret);
		return ret;
	}

	return 0;
}
