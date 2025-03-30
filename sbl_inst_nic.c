// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include "sbl_kconfig.h"
#include "sbl.h"
#include "uapi/sbl_iface_constants.h"
#include "sbl_constants.h"
#include "sbl_serdes.h"
#include "sbl_config_list.h"
#include "sbl_serdes_map.h"
#include "sbl_pml.h"
#include "sbl_internal.h"
#include "uapi/sbl_cassini.h"
#include "sbl_fec.h"

void sbl_set_eth_name(struct sbl_inst *sbl, const char *name)
{
	if (!sbl || !name)
		return;

	strscpy(sbl->iattr.eth_if_name, name, sizeof(sbl->iattr.eth_if_name));

	sbl_dev_info(sbl->dev, "%s eth if name changed to %s", sbl->iattr.inst_name, sbl->iattr.eth_if_name);
}
EXPORT_SYMBOL(sbl_set_eth_name);
