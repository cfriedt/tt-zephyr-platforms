/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tenstorrent_bh_pvt

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfd_tt_bh_pvt, CONFIG_MFD_LOG_LEVEL);

struct mfd_tt_bh_pvt_config {
	uintptr_t irq_regs;
	uintptr_t pvt_regs;
	void (*irq_config_fn)(const struct device *dev);
};

struct mfd_tt_bh_pvt_data {
	struct k_spinlock lock;
};

static int mfd_tt_bh_pvt_init(const struct device *dev)
{
	const struct mfd_tt_bh_pvt_config *config = dev->config;

	if (config->irq_config_fn) {
		config->irq_config_fn(dev);
	}

	return 0;
}

__maybe_unused static void mfd_tt_bh_pvt_isr(const struct device *dev);

#define MFD_TT_BH_PVT_IRQ_FUNC(_inst)                                                              \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(_inst, irq),			\
	(static void mfd_tt_bh_pvt_irq_config_##_inst(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(_inst),				\
			    DT_INST_IRQ(_inst, priority),			\
			    mfd_tt_bh_pvt_isr,				\
			    DEVICE_DT_INST_GET(_inst), 0);			\
		irq_enable(DT_INST_IRQN(_inst));				\
	}), ())

#define MFD_TT_BH_PVT_IRQ_FUNC_INIT(_inst)                                                         \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(_inst, irq),(mfd_tt_bh_pvt_irq_config_##_inst),(NULL))

#define DEFINE_MFD_TT_BH_PVT(_inst)                                                                \
	MFD_TT_BH_PVT_IRQ_FUNC(_inst);                                                             \
                                                                                                   \
	static struct mfd_tt_bh_pvt_data mfd_tt_bh_pvt_data_##_inst;                               \
                                                                                                   \
	static const struct mfd_tt_bh_pvt_config mfd_tt_bh_pvt_config_##_inst = {                  \
		.irq_regs = DT_INST_REG_ADDR_BY_NAME_OR(_inst, irq, 0),                            \
		.pvt_regs = DT_INST_REG_ADDR(_inst),                                               \
		.irq_config_fn = MFD_TT_BH_PVT_IRQ_FUNC_INIT(_inst),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_inst, mfd_tt_bh_pvt_init, NULL, &mfd_tt_bh_pvt_data_##_inst,        \
			      &mfd_tt_bh_pvt_config_##_inst, POST_KERNEL,                          \
			      CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_MFD_TT_BH_PVT)

static void mfd_tt_bh_pvt_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* FIXME: devise a way for child nodes to register interrupt handlers */
}
