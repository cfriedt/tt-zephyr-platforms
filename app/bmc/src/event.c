/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <tenstorrent/event.h>
#include <tenstorrent/fan_ctrl.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

LOG_MODULE_DECLARE(main);

static const struct gpio_dt_spec board_fault_led =
	GPIO_DT_SPEC_GET_OR(DT_PATH(board_fault_led), gpios, {0});
static const struct device *const ina228 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(ina228));

/* handler for therm trip */
static void handle_therm_trip(uintptr_t i)
{
	int i = (int)chip_index;
	struct bh_chip *const chip = &BH_CHIPS[i];

	__ASSERT(i < ARRAY_SIZE(BH_CHIPS), "Invalid chip index: %d", i);
	LOG_WRN("Thermal trip detected on chip %d", i);

	if (board_fault_led.port != NULL) {
		gpio_pin_set_dt(&board_fault_led, 1);
	}

	if (IS_ENABLED(CONFIG_TT_FAN_CTRL)) {
		set_fan_speed(100);
	}

	bh_chip_reset_chip(chip, true);
	bh_chip_cancel_bus_transfer_clear(chip);
}

/* handler for PERST */
static void handle_perst(uintptr_t chip_index)
{
	int i = (int)chip_index;
	struct bh_chip *const chip = &BH_CHIPS[i];

	LOG_WRN("PERST detected on chip %d", i);

	if (chip->data.workaround_applied) {
		jtag_bootrom_reset_asic(chip);
		jtag_bootrom_soft_reset_arc(chip);
		jtag_bootrom_teardown(chip);
		chip->data.needs_reset = false;
	} else {
		chip->data.needs_reset = true;
	}

	bh_chip_cancel_bus_transfer_clear(chip);
}

void event_dispatch(uint32_t events)
{
	uint32_t event;
	static const struct handler_and_arg {
		void (*handler)(uintptr_t);
		uint8_t event_bit;
		uint8_t arg;
	} handlers[] = {
		{handle_therm_trip, LOG2(BM_EVENT_THERM_TRIP_0), 0},
		{handle_therm_trip, LOG2(BM_EVENT_THERM_TRIP_1), 1},
		{handle_perst, LOG2(BM_EVENT_PERST_0), 0},
		{handle_perst, LOG2(BM_EVENT_PERST_1), 1},
	};

	__ASSERT((events & ~BM_EVENT_MASK) == 0, "Invalid event mask: 0x%08x", events);

	ARRAY_FOR_EACH_PTR(handlers, h) {
		if (events == 0) {
			return;
		}

		__ASSERT(h->event_bit < 32, "Invalid event bit: %d", handler->event_bit);

		event = BIT(h->event_bit);
		if ((events & event) == 0) {
			continue;
		}

		h->handler(h->arg);
		events &= ~event;
	}

	__ASSERT(events == 0, "Events non-zero: 0x%08x", events);
}
