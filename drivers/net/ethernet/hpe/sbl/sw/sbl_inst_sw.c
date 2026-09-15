// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>

#include <linux/hpe/sbl/sbl.h>
#include <linux/hpe/sbl/sbl_kconfig.h>

#include "sbl_internal.h"

int sbl_switch_info_get(struct sbl_inst *sbl, struct sbl_init_attr *init_attr)
{
	sbl->is_hw = true;
	sbl->switch_info = sbl_get_switch_info(NULL);
	if (sbl->switch_info == NULL) {
		sbl_dev_err(sbl->dev, "Unable to get sbl_switch_info\n");
		return -ENOMSG;
	}
	return 0;
}
