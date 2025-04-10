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
static int sbl_an_sm_is_np_exchange_done(struct sbl_inst *sbl, int port_num, u64 *sm_state)
{
	u64 pcs_an_next_page_reg = 0;
	unsigned long to_jiffy = jiffies + msecs_to_jiffies(100);

	do {
		if (sbl_base_link_start_cancelled(sbl, port_num))
			return -ECANCELED;
		if (sbl_start_timeout(sbl, port_num))
			return -ETIMEDOUT;

		usleep_range(1000, 2000);
		pcs_an_next_page_reg = sbl_read64(sbl, (SBL_PML_BASE(port_num) | SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET));
		*sm_state = SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(pcs_an_next_page_reg);

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
static bool sbl_an_sm_is_exchange_done(struct sbl_inst *sbl, int port_num, u64 sm_state)
{
	u64 pcs_an_next_page_reg = 0;

	if (sm_state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK)
		return true;

	pcs_an_next_page_reg = sbl_read64(sbl, (SBL_PML_BASE(port_num) | SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET));

	return (SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(pcs_an_next_page_reg) == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK);
}

int sbl_an_page_exchange(struct sbl_inst *sbl, int port_num, unsigned long remaining_jiffies)
{
	struct sbl_link *link = sbl->link + port_num;
	int xcng_count = 1;
	u64 sm_state = 0;
	u64 sts_autoneg_next_reg;
	u32 base = SBL_PML_BASE(port_num);
	int err;

	/* -- State Machine polling based next page exchange sequence -- */
	do {
		sbl_dev_dbg(sbl->dev, "an %d: next page %d: start", port_num, xcng_count);

		if (sbl_base_link_start_cancelled(sbl, port_num))
			return -ECANCELED;
		if (sbl_start_timeout(sbl, port_num))
			return -ETIMEDOUT;

		/* setup the next page (real or null) */
		if (xcng_count < link->an_tx_count)
			sbl_an_setup_next_page(sbl, port_num, xcng_count);
		else
			sbl_an_setup_null_page(sbl, port_num);

		/* send the next page */
		sbl_an_send_next_page(sbl, port_num);

		/* check if the next page exchange is done */
		err = sbl_an_sm_is_np_exchange_done(sbl, port_num, &sm_state);
		if (err) {
			sbl_dev_dbg(sbl->dev, "an %d: next page %d: exchange timeout", port_num, xcng_count);
			sbl_an_dump_state(sbl, port_num);
			return -ETIME;
		}

		/* check if received page is a next page */
		if (!sbl_an_is_next_page(sbl, port_num)) {
			sbl_dev_dbg_ratelimited(sbl->dev, "an %d: next page %d: missing next page indication, resend next-page",
					port_num, xcng_count);
			sbl_an_dump_state(sbl, port_num);
			continue;
		}

		/* put the received page in the buffer if indicated */
		if (link->an_rx_page[xcng_count - 1] & AN_NP_NP_MASK) {
			sts_autoneg_next_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET);
			link->an_rx_page[xcng_count] =
				SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(sts_autoneg_next_reg);
			sbl_dev_dbg(sbl->dev, "an %d: rx next page: 0x%llx",
					port_num, link->an_rx_page[xcng_count]);
			link->an_rx_count++;
		}

		/* go to the additional next page */
		xcng_count++;

		/* Too many Rx pages ?*/
		if (xcng_count >= SBL_AN_MAX_RX_PAGES) {
			sbl_dev_err_ratelimited(sbl->dev, "an %d: rx next page: too many pages %d",
						port_num, xcng_count);
			sbl_an_dump_state(sbl, port_num);
			return -EPROTO;
		}

		sbl_dev_dbg(sbl->dev, "an %d: sm_state = %s", port_num, sbl_an_state_str(sm_state));

		/* check to see if the exchange is done */
	} while (!sbl_an_sm_is_exchange_done(sbl, port_num, sm_state));

	return 0;
}

/* Dummy funtion-nothing to do here */
void sbl_an_version_read(struct sbl_inst *sbl, int port_num)
{
}
