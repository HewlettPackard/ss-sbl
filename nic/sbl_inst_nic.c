// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_serdes_map.h"
#include "sbl_internal.h"
#include "uapi/sbl_cassini.h"
#include "sbl_fec.h"

struct sbl_switch_info cas_brazos_switch_info = SBL_CASSINI_BRAZOS_SW_INFO_INITIALIZER;
struct sbl_switch_info cas_nic0_switch_info = SBL_CASSINI_NIC0_SW_INFO_INITIALIZER;
struct sbl_switch_info cas_nic1_switch_info = SBL_CASSINI_NIC1_SW_INFO_INITIALIZER;

void sbl_set_eth_name(struct sbl_inst *sbl, const char *name)
{
	if (!sbl || !name)
		return;

	strscpy(sbl->iattr.eth_if_name, name, sizeof(sbl->iattr.eth_if_name));

	sbl_dev_info(sbl->dev, "%s eth if name changed to %s", sbl->iattr.inst_name, sbl->iattr.eth_if_name);
}
EXPORT_SYMBOL(sbl_set_eth_name);

int sbl_switch_info_get(struct sbl_inst *sbl, struct sbl_init_attr *init_attr)
{
	sbl->is_hw = init_attr->is_hw;
	switch (init_attr->uc_platform) {
	case SBL_UC_PLATFORM_SAWTOOTH:
		if (init_attr->uc_nic == 0) {
			sbl->switch_info = &cas_nic0_switch_info;
		} else if (init_attr->uc_nic == 1) {
			sbl->switch_info = &cas_nic1_switch_info;
		} else {
			sbl_dev_err(sbl->dev, "Bad NIC index (%d)!\n",
					init_attr->uc_nic);
			return -EINVAL;
		}
		break;
	case SBL_UC_PLATFORM_BRAZOS:
		sbl->switch_info = &cas_brazos_switch_info;
		break;
	case SBL_UC_PLATFORM_UNDEFINED:
		sbl_dev_err(sbl->dev, "Undefined uC platform!\n");
		return -EINVAL;
	default:
		sbl_dev_err(sbl->dev, "Unknown uC platform (%d)!\n", init_attr->uc_platform);
		return -EINVAL;
	}

	return 0;
}
