/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MFD_TT_BH_PVT_H_
#define ZEPHYR_DRIVERS_MFD_TT_BH_PVT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Relative to the base address in Devicetree */
#define PVT_CNTL_IRQ_EN_REG_ADDR             0x40
#define PVT_CNTL_TS_00_IRQ_ENABLE_REG_ADDR   0xC0
#define PVT_CNTL_PD_00_IRQ_ENABLE_REG_ADDR   0x340
#define PVT_CNTL_VM_00_IRQ_ENABLE_REG_ADDR   0xA00
#define PVT_CNTL_TS_00_ALARMA_CFG_REG_ADDR   0xE0
#define PVT_CNTL_TS_00_ALARMB_CFG_REG_ADDR   0xE4
#define PVT_CNTL_TS_CMN_CLK_SYNTH_REG_ADDR   0x80
#define PVT_CNTL_PD_CMN_CLK_SYNTH_REG_ADDR   0x300
#define PVT_CNTL_VM_CMN_CLK_SYNTH_REG_ADDR   0x800
#define PVT_CNTL_PD_CMN_SDIF_STATUS_REG_ADDR 0x308
#define PVT_CNTL_PD_CMN_SDIF_REG_ADDR        0x30C
#define PVT_CNTL_TS_CMN_SDIF_STATUS_REG_ADDR 0x88
#define PVT_CNTL_TS_CMN_SDIF_REG_ADDR        0x8C
#define PVT_CNTL_PD_CMN_SDIF_STATUS_REG_ADDR 0x308
#define PVT_CNTL_PD_CMN_SDIF_REG_ADDR        0x30C
#define PVT_CNTL_VM_CMN_SDIF_STATUS_REG_ADDR 0x808
#define PVT_CNTL_VM_CMN_SDIF_REG_ADDR        0x80C
#define PVT_CNTL_TS_00_SDIF_DONE_REG_ADDR    0xD4
#define PVT_CNTL_TS_00_SDIF_DATA_REG_ADDR    0xD8
#define PVT_CNTL_VM_00_SDIF_RDATA_REG_ADDR   0xA30
#define PVT_CNTL_PD_00_SDIF_DONE_REG_ADDR    0x354
#define PVT_CNTL_PD_00_SDIF_DATA_REG_ADDR    0x358

/* these macros are used for registers specific for each sensor */
#define TS_PD_OFFSET                  0x40
#define VM_OFFSET                     0x200
#define GET_TS_REG_ADDR(ID, REG_NAME) (ID * TS_PD_OFFSET + PVT_CNTL_TS_00_##REG_NAME##_REG_ADDR)
#define GET_PD_REG_ADDR(ID, REG_NAME) (ID * TS_PD_OFFSET + PVT_CNTL_PD_00_##REG_NAME##_REG_ADDR)
#define GET_VM_REG_ADDR(ID, REG_NAME) (ID * VM_OFFSET + PVT_CNTL_VM_00_##REG_NAME##_REG_ADDR)

#ifdef __cplusplus
}
#endif

#endif
