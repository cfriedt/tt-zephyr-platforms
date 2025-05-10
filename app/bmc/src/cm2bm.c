/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tenstorrent/bh_arc.h>
#include <tenstorrent/bh_chip.h>
#include <tenstorrent/fan_ctrl.h>
#include <zephyr/reboot.h>

void process_cm2bm_message(struct bh_chip *chip)
{
	cm2bmMessageRet msg = bh_chip_get_cm2bm_message(chip);

	if (msg.ret == 0) {
		cm2bmMessage message = msg.msg;

		switch (message.msg_id) {
		case 0x1:
			switch (message.data) {
			case 0x0:
				jtag_bootrom_reset_sequence(chip, true);
				break;
			case 0x3:
				/* Trigger reboot; will reset asic and reload bmfw
				 */
				if (IS_ENABLED(CONFIG_REBOOT)) {
					sys_reboot(SYS_REBOOT_COLD);
				}
				break;
			}
			break;
		case 0x2:
			/* Respond to ping request from CMFW */
			bharc_smbus_word_data_write(&chip->config.arc, 0x21, 0xA5A5);
			break;
		case 0x3:
			if (IS_ENABLED(CONFIG_TT_FAN_CTRL)) {
				set_fan_speed((uint8_t)message.data & 0xFF);
			}
			break;
		}
	}
}
