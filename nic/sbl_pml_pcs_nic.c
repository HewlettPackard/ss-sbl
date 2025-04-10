// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <sbl/sbl_pml.h>

#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_serdes_map.h"
#include "sbl_internal.h"
#include "sbl_serdes_fn.h"

#ifdef CONFIG_SYSFS
int sbl_pml_pcs_speed_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);
	if (link->pcs_config)
		s += snprintf(buf+s, size-s, "%s", sbl_link_mode_str(link->link_mode));
	else
		s += snprintf(buf+s, size-s, "NA");
	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_pml_pcs_speed_sysfs_sprint);
#endif
