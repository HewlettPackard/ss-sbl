// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <sbl/sbl_pml.h>

#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_serdes.h"
#include "sbl_constants.h"
#include "sbl_an.h"
#include "sbl_serdes_map.h"
#include "sbl_internal.h"

/*
 * np exchange - check for complete or done
 */
int sbl_an_sm_is_np_exchange_done(struct sbl_inst *sbl, int port_num, u64 *sm_state)
{
	u64           pcs_an_next_page_reg = 0;
	unsigned long to_jiffy             = jiffies + msecs_to_jiffies(100);

	do {

		if (sbl_base_link_start_cancelled(sbl, port_num))
			return -ECANCELED;
		if (sbl_start_timeout(sbl, port_num))
			return -ETIMEDOUT;

		usleep_range(1000, 2000);

		pcs_an_next_page_reg = sbl_read64(sbl, (SBL_PML_BASE(port_num) | SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET));
		*sm_state            = SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(pcs_an_next_page_reg);

		if (*sm_state == SBL_PML_AUTONEG_STATE_COMPLETE_ACK)
			return 0;
		if (*sm_state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK)
			return 0;

	} while (time_before(jiffies, to_jiffy));

	return -ETIME;
}

/*
 * exchange - check for done
 */
bool sbl_an_sm_is_exchange_done(struct sbl_inst *sbl, int port_num, u64 sm_state)
{
	u64 pcs_an_next_page_reg = 0;

	if (sm_state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK)
		return true;

	pcs_an_next_page_reg = sbl_read64(sbl, (SBL_PML_BASE(port_num) | SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET));

	return (SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(pcs_an_next_page_reg) == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK);
}
