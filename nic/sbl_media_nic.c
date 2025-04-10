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
