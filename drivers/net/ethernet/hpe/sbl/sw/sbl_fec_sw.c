// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include <linux/hpe/sbl/sbl.h>
#include <linux/hpe/sbl/sbl_fec.h>
#include <linux/hpe/sbl/sbl_kconfig.h>

#include <sbl/sbl_pml.h>

#include "sbl_internal.h"

void sbl_fec_ucw_bad_get(struct sbl_fec *fec_prmts, u64 *ucw_bad, u64 *ucw_hwm)
{
	spin_lock(&fec_prmts->fec_cw_lock);
	*ucw_hwm = fec_prmts->fec_ucw_hwm;
	*ucw_bad = fec_prmts->fec_ucw_thresh;
	spin_unlock(&fec_prmts->fec_cw_lock);
}

void sbl_fec_ccw_bad_get(struct sbl_fec *fec_prmts, bool use_stp_thresh,
			u64 *ccw_bad, u64 *ccw_hwm)
{
	spin_lock(&fec_prmts->fec_cw_lock);
	*ccw_hwm = fec_prmts->fec_ccw_hwm;

	if (use_stp_thresh)
		*ccw_bad = fec_prmts->fec_stp_ccw_thresh;
	else
		*ccw_bad = fec_prmts->fec_ccw_thresh;

	spin_unlock(&fec_prmts->fec_cw_lock);
}
