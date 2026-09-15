// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include <linux/hpe/sbl/sbl.h>
#include <linux/hpe/sbl/sbl_kconfig.h>

#include <sbl/sbl_pml.h>

#include "sbl_pml_fn.h"
#include "sbl_link.h"
#include "sbl_serdes_map.h"
#include "sbl_internal.h"
#include "sbl_serdes_fn.h"

#ifdef CONFIG_SYSFS
/**
 * sbl_pml_pcs_speed_sysfs_sprint() - Report PCS mode into buffer
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @buf: Destination buffer to write the data
 * @size: Size of data to write
 *
 * Based on PCS config, this function writes the link mode into
 * the provided buffer
 *
 * Context: Process context, Acquires lock and release link->lock <spin_lock>
 *
 * Return: Number of characters written on success
 */
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
