/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdbool.h>
 #include <stdlib.h>
 
 #include <app_version.h>
 #include <tenstorrent/bist.h>
 #include <tenstorrent/fan_ctrl.h>
 #include <tenstorrent/fwupdate.h>
 #include <zephyr/devicetree.h>
 #include <zephyr/drivers/gpio.h>
 #include <zephyr/drivers/i2c.h>
 #include <zephyr/drivers/sensor.h>
 #include <zephyr/kernel.h>
 #include <zephyr/init.h>
 #include <zephyr/logging/log.h>
 #include <zephyr/sys/reboot.h>
 
 #include <tenstorrent/tt_smbus.h>
 #include <tenstorrent/bh_chip.h>
 #include <tenstorrent/bh_arc.h>
 #include <tenstorrent/event.h>
 #include <tenstorrent/jtag_bootrom.h>
 
 LOG_MODULE_DECLARE(main);
 
 extern struct bh_chip BH_CHIPS[BH_CHIP_COUNT];
 
 static const struct gpio_dt_spec board_fault_led =
     GPIO_DT_SPEC_GET_OR(DT_PATH(board_fault_led), gpios, {0});

 int update_fw(void)
 {
     /* To get here we are already running known good fw */
     int ret;
 
     const struct gpio_dt_spec reset_spi = BH_CHIPS[BH_CHIP_PRIMARY_INDEX].config.spi_reset;
 
     if (IS_ENABLED(CONFIG_TT_FWUPDATE)) {
		if (!tt_fwupdate_is_confirmed()) {
			if (bist_rc < 0) {
				LOG_ERR("Firmware update was unsuccessful and will be rolled-back "
					"after bmfw reboot.");
				if (IS_ENABLED(CONFIG_REBOOT)) {
					sys_reboot(SYS_REBOOT_COLD);
				}
				return EXIT_FAILURE;
			}

			ret = tt_fwupdate_confirm();
			if (ret < 0) {
				LOG_ERR("%s() failed: %d", "tt_fwupdate_confirm", ret);
				return EXIT_FAILURE;
			}
		}
	}

     ret = gpio_pin_configure_dt(&reset_spi, GPIO_OUTPUT_ACTIVE);
     if (ret < 0) {
         LOG_ERR("%s() failed : %d", "gpio_pin_configure_dt", ret);
         return 0;
     }
 
     gpio_pin_set_dt(&reset_spi, 1);
     k_busy_wait(1000);
     gpio_pin_set_dt(&reset_spi, 0);
 
     if (IS_ENABLED(CONFIG_TT_FWUPDATE)) {
         /* Check for and apply a new update, if one exists (we disable reboot here) */
         ret = tt_fwupdate("bmfw", false, false);
         if (ret < 0) {
             LOG_ERR("%s() failed: %d", "tt_fwupdate", ret);
             /*
              * This might be as simple as no update being found, but it could be due to
              * something else - e.g. I/O error, failure to read from external spi,
              * failure to write to internal flash, image corruption / crc failure, etc.
              */
             return 0;
         }
 
         if (ret == 0) {
             LOG_DBG("No firmware update required");
         } else {
             LOG_INF("Reboot needed in order to apply bmfw update");
             if (IS_ENABLED(CONFIG_REBOOT)) {
                 sys_reboot(SYS_REBOOT_COLD);
             }
         }
     } else {
         ret = 0;
     }
 
     if (IS_ENABLED(CONFIG_TT_FWUPDATE)) {
		ret = tt_fwupdate_complete();
		if (ret != 0) {
			return ret;
		}
	}

     return ret;
 }
 
 void ina228_current_update(void)
 {
     struct sensor_value current_sensor_val;
     const struct device *const ina228 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(ina228));
 
     sensor_sample_fetch_chan(ina228, SENSOR_CHAN_CURRENT);
     sensor_channel_get(ina228, SENSOR_CHAN_CURRENT, &current_sensor_val);
 
     int32_t current =
         (current_sensor_val.val1 << 16) + (current_sensor_val.val2 * 65536ULL) / 1000000ULL;
 
     ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
         bh_chip_set_input_current(chip, &current);
     }
 }
 
 int main(void)
 {
     int ret;
     int bist_rc;
 
     if (IS_ENABLED(CONFIG_TT_FWUPDATE)) {
         /* Only try to update from the primary chip spi */
         ret = tt_fwupdate_init(BH_CHIPS[BH_CHIP_PRIMARY_INDEX].config.flash,
                        BH_CHIPS[BH_CHIP_PRIMARY_INDEX].config.spi_mux);
         if (ret != 0) {
             return ret;
         }
     }
 
     ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
         if (chip->config.arc.smbus.bus == NULL) {
             continue;
         }
 
         tt_smbus_stm32_set_abort_ptr(chip->config.arc.smbus.bus,
                          &((&chip->data)->bus_cancel_flag));
     }
 
     bist_rc = 0;
     if (IS_ENABLED(CONFIG_TT_BIST)) {
         bist_rc = tt_bist();
         if (bist_rc < 0) {
             LOG_ERR("%s() failed: %d", "tt_bist", bist_rc);
         } else {
             LOG_DBG("Built-in self-test succeeded");
         }
     }
 
     if (IS_ENABLED(CONFIG_TT_FAN_CTRL)) {
         ret = init_fan();
         set_fan_speed(100); /* Set fan speed to 100 by default */
         if (ret != 0) {
             LOG_ERR("%s() failed: %d", "init_fan", ret);
             return ret;
         }
     }
 
     ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
         ret = therm_trip_gpio_setup(chip);
         if (ret != 0) {
             LOG_ERR("%s() failed: %d", "therm_trip_gpio_setup", ret);
             return ret;
         }
     }
 
     if (IS_ENABLED(CONFIG_TT_FWUPDATE)) {
         if (!tt_fwupdate_is_confirmed()) {
             if (bist_rc < 0) {
                 LOG_ERR("Firmware update was unsuccessful and will be rolled-back "
                     "after bmfw reboot.");
                 if (IS_ENABLED(CONFIG_REBOOT)) {
                     sys_reboot(SYS_REBOOT_COLD);
                 }
                 return EXIT_FAILURE;
             }
 
             ret = tt_fwupdate_confirm();
             if (ret < 0) {
                 LOG_ERR("%s() failed: %d", "tt_fwupdate_confirm", ret);
                 return EXIT_FAILURE;
             }
         }
     }
 
     ret = update_fw();
     if (ret != 0) {
         return ret;
     }
 
     if (IS_ENABLED(CONFIG_TT_FWUPDATE)) {
         ret = tt_fwupdate_complete();
         if (ret != 0) {
             return ret;
         }
     }
 
     /* Force all spi_muxes back to arc control */
     ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
         if (chip->config.spi_mux.port != NULL) {
             gpio_pin_configure_dt(&chip->config.spi_mux, GPIO_OUTPUT_ACTIVE);
         }
     }
 
     if (IS_ENABLED(CONFIG_TT_ASSEMBLY_TEST) && board_fault_led.port != NULL) {
         gpio_pin_configure_dt(&board_fault_led, GPIO_OUTPUT_INACTIVE);
     }
 
     if (IS_ENABLED(CONFIG_JTAG_LOAD_BOOTROM)) {
         ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
             ret = jtag_bootrom_init(chip);
             if (ret != 0) {
                 LOG_ERR("%s() failed: %d", "jtag_bootrom_init", ret);
                 return ret;
             }
 
             ret = jtag_bootrom_reset_sequence(chip, false);
             if (ret != 0) {
                 LOG_ERR("%s() failed: %d", "jtag_bootrom_reset", ret);
                 return ret;
             }
         }
 
         LOG_DBG("Bootrom workaround successfully applied");
     }
 
     printk("BMFW VERSION " APP_VERSION_STRING "\n");
 
     if (IS_ENABLED(CONFIG_TT_ASSEMBLY_TEST) && board_fault_led.port != NULL) {
         gpio_pin_set_dt(&board_fault_led, 1);
     }
 
     /* No mechanism for getting bl version... yet */
     bmStaticInfo static_info =
         (bmStaticInfo){.version = 1, .bl_version = 0, .app_version = APPVERSION};
 
     while (true) {
         event_dispatch(bm_event_wait(BM_EVENT_MASK, K_MSEC(20)));
 
         /*
          * FIXME: create a workqueue or sensor thread to sample all sensors
          * (note: this is possible even with non-uniform sample rates).
          */
         if (IS_ENABLED(CONFIG_INA228)) {
             ina228_current_update();
         }
 
         /* FIXME: create mfd driver for fan controller and sample from sensor thread */
         if (IS_ENABLED(CONFIG_TT_FAN_CTRL)) {
             uint16_t rpm = get_fan_rpm();
 
             /* FIXME: periodically trigger this as an event */
             ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
                 bh_chip_set_fan_rpm(chip, rpm);
             }
         }
 
         /* FIXME: periodically trigger this as an event */
         ARRAY_FOR_EACH_PTR(BH_CHIPS, chip) {
             process_cm2bm_message(chip);
         }
     }
 
     return EXIT_SUCCESS;
 }
 