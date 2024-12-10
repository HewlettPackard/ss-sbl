// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019, 2022 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>

#include "sbl.h"
#include "sbl_internal.h"

/*
 * create and initialise the SBL link's counter array
 */
int sbl_link_counters_init(struct sbl_link *link)
{
	int i;

	if (!link->counters) {
		link->counters = kzalloc(sizeof(atomic_t)*SBL_LINK_NUM_COUNTERS, GFP_KERNEL);
		if (!link->counters)
			return -ENOMEM;
	}

	for (i = 0; i < SBL_LINK_NUM_COUNTERS; ++i)
		atomic_set(link->counters + i, 0);

	return 0;
}

/*
 * destroy the SBL link's counter array
 */
void sbl_link_counters_term(struct sbl_link *link)
{
	kfree(link->counters);
	link->counters = NULL;
}

/*
 * Get a block of SBL link counters
 */
int sbl_link_counters_get(struct sbl_inst *sbl, int port_num,
		int *counters, u16 first, u16 count)
{
	struct sbl_link *link;
	int i;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return -EINVAL;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return -EINVAL;

	link = sbl->link + port_num;

	if ((first + count) > SBL_LINK_NUM_COUNTERS)
		return -EINVAL;

	if (!counters)
		return -EINVAL;

	for (i = first; i < first + count; ++i)
		*counters++ = atomic_read(link->counters + i);

	return 0;
}
EXPORT_SYMBOL(sbl_link_counters_get);

/*
 * Returns value of a SBL link counter
 */
int sbl_link_counters_read(struct sbl_inst *sbl, int port_num, u16 counter)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return 0;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return 0;

	link = sbl->link + port_num;

	if (counter >= SBL_LINK_NUM_COUNTERS)
		return 0;

	return atomic_read(link->counters + counter);

}
EXPORT_SYMBOL(sbl_link_counters_read);

/*
 * Increment a SBL link counter
 */
int sbl_link_counters_incr(struct sbl_inst *sbl, int port_num, u16 counter)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return -EINVAL;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return -EINVAL;

	link = sbl->link + port_num;

	if (counter >= SBL_LINK_NUM_COUNTERS)
		return -EINVAL;

	atomic_inc(link->counters + counter);

	return 0;
}
