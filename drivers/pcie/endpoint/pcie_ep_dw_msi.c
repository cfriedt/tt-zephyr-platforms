/*
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "pcie_ep_iproc.h"

#include <errno.h>

 #include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
 #include <zephyr/logging/log.h>
 
 LOG_MODULE_DECLARE(pcie_ep_dw, CONFIG_PCIE_EP_LOG_LEVEL);
 
 static int pcie_ep_dw_gen_msix(const struct device *dev, const uint32_t msix_num)
 {
	return -ENOSYS;
 }
 

 int pcie_ep_dw_generate_msix(const struct device *dev, const uint32_t msix_num)
{
	return pcie_ep_dw_gen_msix(dev, msix_num);
}
