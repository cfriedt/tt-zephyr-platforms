/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#define TT_FLASH_LOCK_PASSWORD_SIZE 8

#define TT_FLASH_LOCK_READ_NVLOCKBITS_CMD 0xe2
#define TT_FLASH_LOCK_WRITE_NVLOCKBITS_CMD 0xe3
#define TT_FLASH_LOCK_ERASE_NVLOCKBITS_CMD 0xe4

#define TT_FLASH_LOCK_READ_GLOBALFREEZE_CMD 0xa7
#define TT_FLASH_LOCK_WRITE_GLOBALFREEZE_CMD 0xa6

#define TT_FLASH_LOCK_READ_PASSWD_CMD 0x27
#define TT_FLASH_LOCK_WRITE_PASSWD_CMD 0x28
#define TT_FLASH_LOCK_UNLOCK_PASSWD_CMD 0x29

LOG_MODULE_REGISTER(tt_flash_lock, LOG_LEVEL_DBG);

struct unified_spi_dt_spec {
    union {
        struct spi_dt_spec *spi;
        struct mspi_dt_spec *mspi;
    };
    bool is_mspi;
};

static int tt_flash_lock(const struct unified_spi_dt_spec *spec, const uint8_t *key, size_t size, k_timeout_t timeout)
{
    if (size != TT_FLASH_LOCK_PASSWORD_SIZE) {
        LOG_DBG("Key size must be %d bytes", TT_FLASH_LOCK_PASSWORD_SIZE);
        return -EINVAL;
    }

    if (IS_ENABLED(CONFIG_MSPI) && spec->is_mspi) {
        /* Use MSPI API */
        struct mspi_xfer_packet packets[] = {
            {
                .dir = MSPI_TX,
                .cb_mask = MSPI_BUS_NO_CB,
                .cmd = TT_FLASH_LOCK_WRITE_PASSWD_CMD,
                .num_bytes = TT_FLASH_LOCK_PASSWORD_SIZE,
                .data_buf = (uint8_t *)key,
            },
        };
        struct mspi_xfer req = {
            .packets = packets,
            .num_packet = ARRAY_SIZE(packets),
        };

        return mspi_transceive(spec->mspi->bus, NULL, &req);
    }

    if (IS_ENABLED(CONFIG_SPI) && !spec->is_mspi) {
        /* Use SPI API */
        struct spi_buf buffers[] = {
            {
                .buf = (uint8_t[]){ TT_FLASH_LOCK_WRITE_PASSWD_CMD },
                .len = 1,
            },
            {
                .buf = key,
                .len = TT_FLASH_LOCK_PASSWORD_SIZE,
            },
        };
        struct spi_buf_set tx_bufs = {
            .buffers = buffers,
            .count = ARRAY_SIZE(buffers),
        };

        return spi_write_dt(spec->spi, tx_bufs);
    }

    LOG_DBG("No SPI or MSPI configuration is available");
    return -ENOTSUP;
}

static int tt_flash_unlock(const struct unified_spi_dt_spec *spec, const uint8_t *key, size_t size)
{
}

static int tt_flash_is_locked(const struct unified_spi_dt_spec *spec)
{
}

#if defined(CONFIG_SPI)
static const struct spi_dt_spec *spi_spec = SPI_DT_SPEC_GET(DT_NODELABEL(spi1), SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0);
#else
static const struct spi_dt_spec *spi_spec = NULL;
#endif

#if defined(CONFIG_MSPI)
static const struct mspi_dt_spec *mspi_spec = NULL;
#else
static const struct mspi_dt_spec *mspi_spec = NULL;
#endif

ZTEST(tt_flash_lock, test_tt_flash_lock)
{
}

ZTEST(tt_flash_lock, test_tt_flash_unlock)
{
}

ZTEST(tt_flash_lock, test_tt_flash_is_locked)
{
    int ret;

    zassert_equal(tt_flash_is_locked(), 0);
}

ZTEST_SUITE(tt_flash_lock, NULL, NULL, NULL, NULL, NULL);