/*
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dw_pcie_ep

#include "pcie_ep_dw.h"

#include <errno.h>

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pcie_ep_dw, CONFIG_PCIE_EP_LOG_LEVEL);

static int pcie_ep_dw_conf_read(const struct device *dev, uint32_t offset,
				uint32_t *data)
{
	return -ENOSYS;
}

static void pcie_ep_dw_conf_write(const struct device *dev, uint32_t offset,
				  uint32_t data)
{
}

static int pcie_ep_dw_map_addr(const struct device *dev, uint64_t pcie_addr,
			       uint64_t *mapped_addr, uint32_t size,
			       enum pcie_ob_mem_type ob_mem_type)
{
	return -ENOSYS;
}

static void pcie_ep_dw_unmap_addr(const struct device *dev,
				  uint64_t mapped_addr)
{
}

static int pcie_ep_dw_raise_irq(const struct device *dev,
				enum pci_ep_irq_type irq_type,
				uint32_t irq_num)
{
	return -ENOSYS;
}

static int pcie_ep_dw_register_reset_cb(const struct device *dev,
					enum pcie_reset reset,
					pcie_ep_reset_callback_t cb, void *arg)
{
	return -ENOSYS;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(dmas)
static int pcie_ep_dw_dma_xfer(const struct device *dev,
				     uint64_t mapped_addr,
				     uintptr_t local_addr, uint32_t size,
				     const enum xfer_direction dir)
{
	return -ENOSYS;
}
#endif

static int pcie_ep_dw_init(const struct device *dev)
{
	return -ENOSYS;
}

static DEVICE_API(pcie_ep, pcie_ep_dw_api) = {
	.conf_read = pcie_ep_dw_conf_read,
	.conf_write = pcie_ep_dw_conf_write,
	.map_addr = pcie_ep_dw_map_addr,
	.unmap_addr = pcie_ep_dw_unmap_addr,
	.raise_irq = pcie_ep_dw_raise_irq,
	.register_reset_cb = pcie_ep_dw_register_reset_cb,
#if DT_INST_NODE_HAS_PROP(0, dmas)
	.dma_xfer = pcie_ep_dw_dma_xfer,
#endif
};

#define DEFINE_PCIE_EP_DW(_inst)						\
									\
	PM_DEVICE_DT_INST_DEFINE(_inst, pcie_ep_dw_pm_action);	\
									\
	DEVICE_DT_INST_DEFINE(_num, pcie_ep_dw_init,			\
				PM_DEVICE_DT_INST_GET(_inst),		\
				&pcie_ep_dw_data_##_inst,			\
				&pcie_ep_dw_config_##_inst, POST_KERNEL,	\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
				&pcie_ep_dw_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_PCIE_EP_DW)
