// SPDX-License-Identifier: GPL-2.0
/* Copyright 2019-2020,2022-2024 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/device.h>

#include "uapi/sbl_kconfig.h"
#include "sbl.h"
#include "uapi/sbl.h"
#include "sbl_constants.h"
#include "sbl_internal.h"
#include "sbl_serdes_map.h"

/*
 * validate a port (link) number
 */
int sbl_validate_port_num(struct sbl_inst *sbl, int port_num)
{
	if (!((port_num >= 0) && (port_num < sbl->switch_info->num_ports)))
		return -EINVAL;
	return 0;
}

