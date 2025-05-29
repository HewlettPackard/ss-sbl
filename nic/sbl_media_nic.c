// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sbl/sbl_pml.h>
#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_internal.h"

#ifdef CONFIG_SYSFS

/**
 * sbl_media_type_sysfs_sprint() - Report media type into buffer
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @buf: Destination buffer to write the data
 * @size: Size of data to write
 *
 * This function reads the physical type of media used based on mconfigured
 * and writes a formatted description of the type into the provided buffer
 *
 * Return: Number of characters written on success
 */
int sbl_media_type_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);

	if (link->mconfigured)
		s += snprintf(buf+s, size-s, "%s", sbl_link_media_str(link->mattr.media));
	else
		s += snprintf(buf+s, size-s, "NA");

	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_media_type_sysfs_sprint);
#endif
