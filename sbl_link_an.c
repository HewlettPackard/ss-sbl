// SPDX-License-Identifier: GPL-2.0

/*
 * sbl_link_an.c
 *
 * Copyright 2019-2024 Hewlett Packard Enterprise Development LP
 */


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
 * for debug purposes we can add an extra null message page for fabric links
 */
#define SBL_SEND_EXTRA_FABRIC_NULL_PAGE                 0

static int  sbl_an_setup_tx_pages(struct sbl_inst *sbl, int port_num);
static int  sbl_an_pml_setup(struct sbl_inst *sbl, int port_num);
static int  sbl_an_hw_wait_prepare(struct sbl_inst *sbl, int port_num)  __maybe_unused;
static int  sbl_an_exchange(struct sbl_inst *sbl, int port_num);
static void sbl_an_send_base_page(struct sbl_inst *sbl, int port_num) __maybe_unused;
static u32  sbl_an_get_nonce(void) __maybe_unused;
static void sbl_an_send_next_page(struct sbl_inst *sbl, int port_num) __maybe_unused;
static void sbl_an_setup_next_page(struct sbl_inst *sbl, int port_num, int page_idx) __maybe_unused;
static void sbl_an_setup_null_page(struct sbl_inst *sbl, int port_num)  __maybe_unused;
static void sbl_an_pml_an_reset(struct sbl_inst *sbl, int port_num, u64 reset_state);
static int  sbl_an_ability_match(struct sbl_inst *sbl, int port_num);
static void sbl_an_update_timeout(struct sbl_inst *sbl, int port_num);
static void sbl_an_dump_state(struct sbl_inst *sbl, int port_num) __maybe_unused;
static bool sbl_an_base_is_complete(struct sbl_inst *sbl, int port_num) __maybe_unused;
static bool sbl_an_base_is_page_recv(struct sbl_inst *sbl, int port_num) __maybe_unused;
static bool sbl_an_is_base_page(struct sbl_inst *sbl, int port_num) __maybe_unused;
static bool sbl_an_next_is_complete(struct sbl_inst *sbl, int port_num) __maybe_unused;
static bool sbl_an_is_next_page(struct sbl_inst *sbl, int port_num) __maybe_unused;
#ifdef CONFIG_SBL_PLATFORM_ROS_HW
static int  sbl_an_sm_is_np_exchange_done(struct sbl_inst *sbl, int port_num, u64 *sm_state);
static bool sbl_an_sm_is_exchange_done(struct sbl_inst *sbl, int port_num, u64 sm_state);
#endif  /* CONFIG_SBL_PLATFORM_ROS_HW */
static bool sbl_an_100cr4_fixup(struct sbl_inst *sbl, int port_num);

int sbl_link_autoneg(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	u32 base = SBL_PML_BASE(port_num);
	u32 random;
	u64 cfg_pcs_reg;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	/* set starting point */
	link = sbl->link + port_num;
	link->an_options = 0;
	link->link_mode  = link->blattr.link_mode;

	switch (link->blattr.pec.an_mode) {

	case SBL_AN_MODE_OFF:
		sbl_dev_dbg(sbl->dev, "an %d: AN off", port_num);
		/* nothing more to do here */
		return 0;

	case SBL_AN_MODE_ON:
	case SBL_AN_MODE_FIXED:
		/* ok */
		break;

	default:
		sbl_dev_err(sbl->dev, "an %d: invalid AN mode (%s)", port_num,
				sbl_an_mode_str(link->blattr.pec.an_mode));
		return -EINVAL;
	}

	/*
	 * we can never be in loopback mode as the nonce will be the same!
	 */
	if (link->blattr.loopback_mode != SBL_LOOPBACK_MODE_OFF) {
		sbl_dev_err(sbl->dev, "an %d: cannot autoneg in loopback mode", port_num);
		return -ENOTUNIQ;
	}

	/*
	 * pcs must not be enabled and autoneg must be off
	 */
	cfg_pcs_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	if (SBL_PML_CFG_PCS_PCS_ENABLE_GET(cfg_pcs_reg)) {
		sbl_dev_err(sbl->dev, "an %d: pcs is enabled", port_num);
		err = -EBUSY;
		goto out;
	}
	if (SBL_PML_CFG_PCS_ENABLE_AUTO_NEG_GET(cfg_pcs_reg)) {
		sbl_dev_err(sbl->dev, "an %d: autoneg already enabled", port_num);
		err = -EBUSY;
		goto out;
	}

	/*
	 * sort out the data to send/receive
	 */
	err = sbl_an_setup_tx_pages(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "an %d, page setup failed [%d]", port_num, err);
		goto out;
	}

	/*
	 * setup
	 */
	err = sbl_pml_install_intr_handler(sbl, port_num, SBL_AUTONEG_ERR_FLGS);
	if (err)
		goto out;
	link->an_100cr4_fixup_applied = false;

	/*
	 * try to negotiate
	 */
	link->an_try_count = 0;
	while (1) {

		link->an_try_count++;

		/* keep trying until up timeout expires or cancelled */
		if (sbl_start_timeout(sbl, port_num)) {
			err = -ETIMEDOUT;
			break;
		}
		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			err = -ECANCELED;
			break;
		}

		/* setup the serdes and pml */
		err = sbl_an_serdes_start(sbl, port_num);
		if (err)
			break;
		msleep(1);

		err = sbl_an_pml_setup(sbl, port_num);
		if (err)
			break;

		/* try to exchange pages */
		err = sbl_an_exchange(sbl, port_num);
		if (err == -EPROTO) {
			/*
			 * we have received an unexpected base page
			 * The lp must have restarted autoneg and we should too
			 *
			 * However we know that some Mellanox cards will do this if there is no
			 * ability match - particularly only supporting 100KR4 not 100CR4
			 * See if we can try to fix this up
			 */
			if (link->blattr.options & SBL_OPT_AUTONEG_100CR4_FIXUP) {
				if (sbl_an_100cr4_fixup(sbl, port_num)) {
					/* fixup applied */
					link->an_100cr4_fixup_applied = true;
					/* carry on to ability match */
				} else {
					/* fixup failed - retry exchange */
					sbl_an_serdes_stop(sbl, port_num);
					continue;
				}
			}
		} else if (err == -ETIME) {
			/* we have timed out */
			if (link->an_try_count < link->blattr.pec.an_max_retry) {
				/* retry */
				sbl_an_serdes_stop(sbl, port_num);
				/* Random delay of 1-5 before retry */
				get_random_bytes(&random, sizeof(random));
				random = 1u + (random % 5u);
				msleep(random);
				continue;
			} else {
				/* give up */
				break;
			}
		} else if (err) {
			/* other error */
			sbl_dev_dbg(sbl->dev, "an %d: exchange failed [%d] (sm_state %s)",
					port_num, err, sbl_an_get_sm_state(sbl, port_num));
			break;
		}

		/* pages have been exchanged - try to resolve the mode etc */
		err = sbl_an_ability_match(sbl, port_num);
		if (err) {
			/* no match - try again (in case they change) */
			continue;
		}

		/* see if we need to update the start timeout */
		sbl_an_update_timeout(sbl, port_num);

		/* we know the lp is there */
		link->lp_detected = true;
		break;
	}

	/* cleanup */
	if (link->sstate == SBL_SERDES_STATUS_AUTONEG)
		sbl_an_serdes_stop(sbl, port_num);
	sbl_pml_disable_intr_handler(sbl, port_num, SBL_AUTONEG_ERR_FLGS);
	sbl_pml_remove_intr_handler(sbl, port_num);

out:
	sbl_pml_err_flgs_clear_all(sbl, port_num);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_ANEG);

	return err;
}


/*
 * perform page exchange
 *
 *
 *   Rosetta has a hw bug whereby some error flags cannot be cleared by the normal method
 *   as the an state machine will continually reassert them. (The failed, complete and
 *   next page received err flags) so instead we disable intrs for these flags when they
 *   are received and reset them in wait prepare.
 *
 *   For next pages we would like to do:
 *
 *      wait_prepare()
 *      send_next_page()
 *      wait_for_completion()
 *
 *   However this sequence will cause the err flags to be immediately set in the wait
 *   prepare and the intr fire.
 *
 *   The best we can do is
 *
 *      preempt_disable()
 *      send_next_page()
 *      wait_prepare()
 *      preempt_enable()
 *      wait_for_completion()
 *
 *     But we can still miss intrs
 *     There would seem to be nothing we can do about this
 *
 *    26 Feb 2021 - added SM exchange to work around rosetta interrupt bug
 *
 */
#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM)
static int sbl_an_exchange(struct sbl_inst *sbl, int port_num)
{
	if (sbl_base_link_start_cancelled(sbl, port_num))
		return -ECANCELED;

	if (sbl_start_timeout(sbl, port_num))
		return -ETIMEDOUT;

	return 0;
}
#else
static int sbl_an_exchange(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	unsigned long timeout = msecs_to_jiffies(1000*link->blattr.pec.an_retry_timeout);
	unsigned long remaining_jiffies;
	u64 sts_autoneg_base_reg;
	u64 sts_autoneg_next_reg;
	int xcng_count;
	int err;
#ifdef CONFIG_SBL_PLATFORM_ROS_HW
	u64 sm_state = 0;
#endif /* CONFIG_SBL_PLATFORM_ROS_HW */

	sbl_dev_dbg(sbl->dev, "an %d: exchange start", port_num);

	if (sbl_base_link_start_cancelled(sbl, port_num))
		return -ECANCELED;
	if (sbl_start_timeout(sbl, port_num))
		return -ETIMEDOUT;

	/* clear any previous pages */
	memset(link->an_rx_page, 0, SBL_AN_MAX_RX_PAGES*sizeof(u64));
	link->an_rx_count = 0;

	/* update the nonce */
	link->an_nonce = sbl_an_get_nonce();
	link->an_tx_page[0] &= ~AN_CW_T_MASK;
	link->an_tx_page[0] |= ((link->an_nonce << AN_CW_T_BASE_BIT) & AN_CW_T_MASK);

	/*
	 * base page exchange
	 */

	err = sbl_an_hw_wait_prepare(sbl, port_num);
	if (err)
		return err;

	sbl_an_send_base_page(sbl, port_num);

	remaining_jiffies = wait_for_completion_timeout(&link->an_hw_change, timeout);
	if (remaining_jiffies == 0) {
		sbl_dev_dbg(sbl->dev, "an %d: base page exchange timeout (nonce %.2x, sm_state %s)",
			port_num, link->an_nonce, sbl_an_get_sm_state(sbl, port_num));
		sbl_an_dump_state(sbl, port_num);

		/* check we have not missed the err flg */
		if (sbl_an_base_is_complete(sbl, port_num) || sbl_an_base_is_page_recv(sbl, port_num))
			sbl_dev_err(sbl->dev, "an %d: MISSED A BASE PAGE!!!!", port_num);

		return -ETIME;
	}

	/* check it is a base page */
	if (!sbl_an_is_base_page(sbl, port_num)) {
		sbl_dev_err(sbl->dev, "an %d: missing base page indication", port_num);
		sbl_an_dump_state(sbl, port_num);
		return -EBADE;
	}

	/* copy page to return buffer */
	sts_autoneg_base_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_OFFSET);
	link->an_rx_page[0] = SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_BASE_PAGE_GET(sts_autoneg_base_reg);
	link->an_rx_count = 1;

	/* check if an is complete i.e. no one has more to send */
	if (sbl_an_base_is_complete(sbl, port_num))
		goto out_success;

	/* since not complete, we must have received a page - check this */
	if (!sbl_an_base_is_page_recv(sbl, port_num)) {
		sbl_dev_err(sbl->dev, "an %d: expected page received indication", port_num);
		sbl_an_dump_state(sbl, port_num);
		return -EBADE;
	}

	/*
	 * next page exchange
	 */

	xcng_count = 1;  /* start at first next page entry */

#ifdef CONFIG_SBL_PLATFORM_ROS_HW

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
			link->an_rx_page[xcng_count] = SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(sts_autoneg_next_reg);
			sbl_dev_dbg(sbl->dev, "an %d: rx next page: 0x%llx", port_num, link->an_rx_page[xcng_count]);
			link->an_rx_count++;
		}

		/* go to the next next page */
		xcng_count++;

		/* Too many Rx pages ?*/
		if (xcng_count >= SBL_AN_MAX_RX_PAGES) {
			sbl_dev_err_ratelimited(sbl->dev, "an %d: rx next page: too many pages %d", port_num, xcng_count);
			sbl_an_dump_state(sbl, port_num);
			return -EPROTO;
		}

		sbl_dev_dbg(sbl->dev, "an %d: sm_state = %s", port_num, sbl_an_state_str(sm_state));

	/* check to see if the exchange is done */
	} while (!sbl_an_sm_is_exchange_done(sbl, port_num, sm_state));

#else /* CONFIG_SBL_PLATFORM_ROS_HW */

	/* -- Interrupt based next page exchange sequence -- */

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

		/* setup and enable interrupt */
		err = sbl_an_hw_wait_prepare(sbl, port_num);
		if (err)
			return err;

		/* send the next page */
		sbl_an_send_next_page(sbl, port_num);

		/* wait for next page interrupt */
		remaining_jiffies = wait_for_completion_timeout(&link->an_hw_change, remaining_jiffies);
		if (!remaining_jiffies) {
			sbl_dev_err(sbl->dev, "an %d: next page %d: exchange timeout", port_num, xcng_count);
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
			link->an_rx_page[xcng_count] = SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(sts_autoneg_next_reg);
			sbl_dev_dbg(sbl->dev, "an %d: rx next page: 0x%llx", port_num, link->an_rx_page[xcng_count]);
			link->an_rx_count++;
		}

		/* go to the next next page */
		xcng_count++;

		/* Too many Rx pages ?*/
		if (xcng_count >= SBL_AN_MAX_RX_PAGES) {
			sbl_dev_err_ratelimited(sbl->dev, "an %d: rx next page: too many pages %d", port_num, xcng_count);
			sbl_an_dump_state(sbl, port_num);
			return -EPROTO;
		}

	/* check to see if the exchange is done */
	} while (!sbl_an_next_is_complete(sbl, port_num));

#endif /* CONFIG_SBL_PLATFORM_ROS_HW */

out_success:

	/* dump the final state on the way out */
	sbl_an_dump_state(sbl, port_num);

	sbl_dev_dbg(sbl->dev, "an %d: exchange complete", port_num);

	return 0;
}
#endif /* defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM) */


static void sbl_an_send_base_page(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	/* first page is the base page (whole reg) */
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_BASE_PAGE_OFFSET, link->an_tx_page[0]);
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_BASE_PAGE_OFFSET);

	sbl_dev_dbg(sbl->dev, "an %d: tx base page: 0x%llx", port_num, link->an_tx_page[0]);

	/* start autoneg by taking an sm out of reset*/
	sbl_an_pml_an_reset(sbl, port_num, 0);
}


static void sbl_an_send_next_page(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 cfg_pcs_autoneg_reg;

	cfg_pcs_autoneg_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET);
	cfg_pcs_autoneg_reg = SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_LOADED_UPDATE(cfg_pcs_autoneg_reg, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET, cfg_pcs_autoneg_reg);
}


static bool sbl_an_base_is_complete(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_autoneg_base_reg;

	sts_autoneg_base_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_OFFSET);

	return SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_COMPLETE_GET(sts_autoneg_base_reg);
}


static bool sbl_an_base_is_page_recv(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_autoneg_base_reg;
	bool val;

	sts_autoneg_base_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_OFFSET);

	val = (SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_ABILITY_GET(sts_autoneg_base_reg)) &&
	      (SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_PAGE_RECEIVED_GET(sts_autoneg_base_reg));

	return val;
}

static bool sbl_an_is_base_page(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_autoneg_base_reg;
	u64 state;
	bool val;

	sts_autoneg_base_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_OFFSET);

	state = SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(sts_autoneg_base_reg);

	val = (state == SBL_PML_AUTONEG_STATE_COMPLETE_ACK) ||
	      (state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK);

	val = val && (SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_PAGE_RECEIVED_GET(sts_autoneg_base_reg)) &&
	      (SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_BASE_PAGE_GET(sts_autoneg_base_reg)) &&
	      (SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_ABILITY_GET(sts_autoneg_base_reg));

	return val;
}


static bool sbl_an_next_is_complete(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_autoneg_next_reg;

	sts_autoneg_next_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET);

	return SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(sts_autoneg_next_reg);
}

static bool sbl_an_is_next_page(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_autoneg_next_reg;
	u64 state;
	bool value;

	sts_autoneg_next_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET);

	state = SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(sts_autoneg_next_reg);

	value =	 (state == SBL_PML_AUTONEG_STATE_COMPLETE_ACK) ||
		(state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK);

	value = value && (SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_ABILITY_GET(sts_autoneg_next_reg)) &&
		(!SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_BASE_PAGE_GET(sts_autoneg_next_reg));

	return value;
}


#ifdef CONFIG_SBL_PLATFORM_ROS_HW
/*
 * np exchange - check for complete or done
 */
static int sbl_an_sm_is_np_exchange_done(struct sbl_inst *sbl, int port_num, u64 *sm_state)
{
	u64           pcs_an_next_page_reg = 0;
	unsigned long to_jiffy             = jiffies + msecs_to_jiffies(100);

	do {

		if (sbl_base_link_start_cancelled(sbl, port_num))
			return -ECANCELED;
		if (sbl_start_timeout(sbl, port_num))
			return -ETIMEDOUT;

		msleep(1);

		pcs_an_next_page_reg = sbl_read64(sbl, (SBL_PML_BASE(port_num) | SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET));
		*sm_state            = SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(pcs_an_next_page_reg);

		if (*sm_state == SBL_PML_AUTONEG_STATE_COMPLETE_ACK)
			return 0;
		if (*sm_state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK)
			return 0;

	} while (time_before(jiffies, to_jiffy));

	return -ETIME;
}
#endif /* CONFIG_SBL_PLATFORM_ROS_HW */


#ifdef CONFIG_SBL_PLATFORM_ROS_HW
/*
 * exchange - check for done
 */
static bool sbl_an_sm_is_exchange_done(struct sbl_inst *sbl, int port_num, u64 sm_state)
{
	u64 pcs_an_next_page_reg = 0;

	if (sm_state == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK) {
		return true;
	}

	pcs_an_next_page_reg = sbl_read64(sbl, (SBL_PML_BASE(port_num) | SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET));

	return (SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(pcs_an_next_page_reg) == SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK);
}
#endif /* CONFIG_SBL_PLATFORM_ROS_HW */


/*
 * Setup the pages
 */
static int sbl_an_setup_tx_pages(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "an %d: run setup tx pages", port_num);

	/* init to known non-zero value so it's easy to tell used pages from init pages */
	memset(link->an_tx_page, 0xAA, sizeof(link->an_tx_page));

	/*
	 * build the base page
	 */

	link->an_tx_page[0] = 0;
	link->an_tx_page[0] |= AN_CW_S_802_3;

	if (link->blattr.pec.an_mode == SBL_AN_MODE_FIXED) {

		switch (link->blattr.link_mode) {

		case SBL_LINK_MODE_BS_200G:
			link->an_tx_page[0] |= (AN_CW_A_200GBASE_CR4) << AN_CW_A_BASE_BIT;
			break;

		case SBL_LINK_MODE_BJ_100G:
			link->an_tx_page[0] |= (AN_CW_A_100GBASE_CR4) << AN_CW_A_BASE_BIT;
			break;

		case SBL_LINK_MODE_CD_100G:
			link->an_tx_page[0] |= (AN_CW_A_100GBASE_CR2) << AN_CW_A_BASE_BIT;
			break;

		case SBL_LINK_MODE_CD_50G:
			link->an_tx_page[0] |= (AN_CW_A_50GBASE_CR) << AN_CW_A_BASE_BIT;
			break;

		default:
			sbl_dev_err(sbl->dev, "an %d: bad an blattr mode", port_num);
			return -EINVAL;
		}
	} else {
		/* all modes we support */
		link->an_tx_page[0] |= (AN_CW_A_200GBASE_CR4) << AN_CW_A_BASE_BIT;
		link->an_tx_page[0] |= (AN_CW_A_100GBASE_CR4) << AN_CW_A_BASE_BIT;
		link->an_tx_page[0] |= (AN_CW_A_100GBASE_CR2) << AN_CW_A_BASE_BIT;
		link->an_tx_page[0] |= (AN_CW_A_50GBASE_CR)   << AN_CW_A_BASE_BIT;
	}

	/*
	 * Not sure what to set here - both Arista and Mellanox say zero so for
	 * now we will do the same and set nothing
	 */

	/* TODO: Add PAUSE */
	link->an_tx_page[0] |= (AN_CW_C_SYMMETRIC) << AN_CW_C_BASE_BIT;

	sbl_dev_dbg(sbl->dev, "an %d: bp = 0x%llx", port_num, link->an_tx_page[0]);

	/* check for disabled next pages */
	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_DISABLE_AN_NEXT_PAGES)) {
		sbl_dev_dbg(sbl->dev, "an %d: next pages disabled", port_num);
		link->an_tx_count = 1;
		return 0;
	}

	/*
	 * build HPE OUI next pages
	 */

	/* have next page (set in previous page) */
	link->an_tx_page[0] |= AN_CW_NP_MASK;

	/* OUI message page */
	link->an_tx_page[1] = 0ULL;
	link->an_tx_page[1] |= AN_NP_MP_MASK;
	link->an_tx_page[1] |= AN_NP_CODE_OUI_EXTENDED_MSG;
	link->an_tx_page[1] |= AN_NP_OUI_HPE;

	sbl_dev_dbg(sbl->dev, "an %d: np mp = 0x%llx", port_num, link->an_tx_page[1]);

	/* have next page (set in previous page) */
	link->an_tx_page[1] |= AN_CW_NP_MASK;

	/* unformatted page with OUI message page */
	link->an_tx_page[2] = 0ULL;
	link->an_tx_page[2] |= AN_NP_OUI_VER_0_1;
	/* LLR options here */
	if ((link->blattr.llr_mode == SBL_LLR_MODE_AUTO)) {
		if (!(link->blattr.options & SBL_OPT_DISABLE_AN_LLR)) {
			link->an_tx_page[2] |= (AN_OPT_LLR << AN_OPT_BASE_BIT);
			if (link->blattr.options & SBL_OPT_ENABLE_ETHER_LLR) {
				link->an_tx_page[2] |= (AN_OPT_ETHER_LLR << AN_OPT_BASE_BIT);
			}
			if (link->blattr.options & SBL_OPT_ENABLE_IFG_HPC_WITH_LLR) {
				link->an_tx_page[2] |= (AN_OPT_HPC_WITH_LLR << AN_OPT_BASE_BIT);
			}
			/* TODO: add IPV4 option */
		}
	}
#ifdef CONFIG_SBL_PLATFORM_CAS_HW
	/* cassini version here */
	/* TODO: read version from HW and set this correctly based on that */
	link->an_tx_page[2] |= ((SBL_LP_SUBTYPE_CASSINI_V1 & AN_LP_SUBTYPE_MASK) << AN_LP_SUBTYPE_BASE_BIT);
#endif

	sbl_dev_dbg(sbl->dev, "an %d: np ufp = 0x%llx", port_num, link->an_tx_page[2]);

	link->an_tx_count = SBL_AN_MAX_TX_PAGES;

	return 0;
}


static void sbl_an_setup_next_page(struct sbl_inst *sbl, int port_num, int page_idx)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);

	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_OFFSET,
		    link->an_tx_page[page_idx]);
	sbl_dev_dbg(sbl->dev, "an %d: tx next page: 0x%llx", port_num,
		link->an_tx_page[page_idx]);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_OFFSET);
}


static void sbl_an_setup_null_page(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 null_page;

	/*
	 * Construct a null message page as in 802.3-2015: 28.2.3.4.1 & annex 28C
	 */
	null_page  = sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_OFFSET);
	null_page ^= AN_NP_T_MASK;          /* toggle bit 11 */
	null_page &= ~AN_NP_NP_MASK;        /* clear next page bit */
	null_page |= AN_NP_MP_MASK;         /* set this is msg page */
	null_page &= ~AN_NP_MSG_MASK;       /* clear msg code */
	null_page |= AN_NP_CODE_NULL_MSG;   /* set null msg code */
	null_page &= ~AN_NP_UCF_MASK;       /* clear unformatted code field code */
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_OFFSET, null_page);
	sbl_dev_dbg(sbl->dev, "an %d: tx null next page: 0x%llx", port_num,
		null_page);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_OFFSET);
}


static int sbl_an_pml_setup(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 cfg_pcs_autoneg_reg;
	u64 cfg_pcs_reg;
	u64 cfg_rx_pcs_reg;
	u64 val64;

	/*
	 * disable pcs autoneg
	 *   as pcs is also disabled this will reset the pcs-serdes CDC logic
	 */
	cfg_pcs_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	cfg_pcs_reg = SBL_PML_CFG_PCS_ENABLE_AUTO_NEG_UPDATE(cfg_pcs_reg, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET, cfg_pcs_reg);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);

	/* configure an */
	cfg_pcs_autoneg_reg = SBL_PML_CFG_PCS_AUTONEG_RX_LANE_SET(sbl->switch_info->ports[port_num].rx_an_swizzle) |
			      SBL_PML_CFG_PCS_AUTONEG_TX_LANE_SET(sbl->switch_info->ports[port_num].tx_an_swizzle) |
			      SBL_PML_CFG_PCS_AUTONEG_RESET_SET(1ULL) |       /* start held in reset */
			      SBL_PML_CFG_PCS_AUTONEG_RESTART_SET(0ULL) |     /* not used - always zero */
			      SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_LOADED_SET(0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET,
			cfg_pcs_autoneg_reg);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET);

	/* reset alignment locking */
	cfg_rx_pcs_reg = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	cfg_rx_pcs_reg = SBL_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(cfg_rx_pcs_reg, 0ULL);
	cfg_rx_pcs_reg = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(cfg_rx_pcs_reg, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, cfg_rx_pcs_reg);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);

	/* config an timers */
	val64 = SBL_PML_CFG_PCS_AUTONEG_TIMERS_DFLT;
	/* disable fault timeout by setting it to its max value  */
	val64 = SBL_PML_CFG_PCS_AUTONEG_TIMERS_LINK_FAIL_INHIBIT_TIMER_MAX_UPDATE(val64, 0xffffffffULL);
#ifdef CONFIG_SBL_FAST_AUTONEG
	/* for emulator reduce waiting time before start */
	val64 = SBL_PML_CFG_PCS_AUTONEG_TIMERS_BREAK_LINK_TIMER_MAX_UPDATE(val64, 100000ULL);
#endif
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_TIMERS_OFFSET, val64);

	/* clear err flags */
	sbl_pml_err_flgs_clear_all(sbl, port_num);

	/*
	 * enable autoneg
	 *   this will take CDC logic out of reset
	 */
	cfg_pcs_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	cfg_pcs_reg = SBL_PML_CFG_PCS_ENABLE_AUTO_NEG_UPDATE(cfg_pcs_reg, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET, cfg_pcs_reg);

	/* an reset is still asserted, it will be deasserted after base page is loaded */

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_ANEG);

	return 0;
}


/*
 * set the reset state
 */
static void sbl_an_pml_an_reset(struct sbl_inst *sbl, int port_num, u64 reset_state)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 cfg_pcs_autoneg_reg;

	/* reset */
	cfg_pcs_autoneg_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET);
	cfg_pcs_autoneg_reg = SBL_PML_CFG_PCS_AUTONEG_RESET_UPDATE(cfg_pcs_autoneg_reg, reset_state);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET,
			cfg_pcs_autoneg_reg);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET);
}


/*
 * setup to detect complete or page received error flags become set
 */
static int sbl_an_hw_wait_prepare(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int err;

	init_completion(&link->an_hw_change);

	sbl_pml_err_flgs_clear(sbl, port_num, SBL_AUTONEG_ERR_FLGS);

	err = sbl_pml_enable_intr_handler(sbl, port_num, SBL_AUTONEG_ERR_FLGS);
	if (err) {
		sbl_dev_err(sbl->dev, "an %d: intr enable failed [%d]", port_num, err);
		return err;
	}

	return 0;
}


static u32 sbl_an_get_nonce(void)
{
	u16 buf;

	while (1) {
		get_random_bytes(&buf, 2);
		buf &= 0x1f;
		if (buf)
			return buf;
	}
}


/*
 * If there is no common mode between this port and our link partner
 * and if the lp advertises 100KR4 and we advertise 100CR4 then add 100CR4
 * to the lp's abilities.
 *
 * This is necessary because recent Mellanox software no longer advertises
 * 100CR even though it seems to be able to tune in this mode. Adding 100CR4
 * to the lp abilities means that we will resolve 100CR4 and start using this
 * mode.
 *
 */
#define SBL_AN_KR4_BIT (1ULL << (AN_CW_A_BASE_BIT + AN_CW_A_100GBASE_KR4_BIT))
#define SBL_AN_CR4_BIT (1ULL << (AN_CW_A_BASE_BIT + AN_CW_A_100GBASE_CR4_BIT))

static bool sbl_an_100cr4_fixup(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	/* check no match */
	if (link->an_tx_page[0] & link->an_rx_page[0] & AN_CW_A_MASK)
		return false;

	/* check we want cr4 */
	if (!(link->an_tx_page[0] & SBL_AN_CR4_BIT))
		return false;

	/* check lp wants kr4 */
	if (!(link->an_rx_page[0] & SBL_AN_KR4_BIT))
		return false;

	/* add cr4 to lp abilities */
	link->an_rx_page[0] |= SBL_AN_CR4_BIT;

	/* fixup applied */
	sbl_dev_info(sbl->dev, "an %d: 100cr4 fixup applied", port_num);
	return true;
}


static int sbl_an_ability_match(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 fec_ability;
	u32 tech_ability;
	u32 pause_ability;
	u64  last_msg_code = 0;
	bool expect_hpe    = false;
	int  x             = 0;

	sbl_dev_dbg(sbl->dev, "an %d: rx count = %d", port_num, link->an_rx_count);

#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM)
	/* Netsim/Z1: No AN happening, so just force the response data to the
	 *   tx data to ensure capability match
	 */
	link->an_rx_page[0] = link->an_tx_page[0];
#endif /* defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM) */

	/*
	 * abilities
	 */
	tech_ability = (link->an_tx_page[0] & link->an_rx_page[0] & AN_CW_A_MASK)
			>> AN_CW_A_BASE_BIT;
	fec_ability = (link->an_tx_page[0] & link->an_rx_page[0] & AN_CW_F_MASK)
					>> AN_CW_F_BASE_BIT;
	pause_ability = (link->an_tx_page[0] & link->an_rx_page[0] & AN_CW_C_MASK)
					>> AN_CW_C_BASE_BIT;
	sbl_dev_dbg(sbl->dev, "an %d: tech 0x%x, fec 0x%x, pause %d", port_num,
			tech_ability, fec_ability, pause_ability);

	/*
	 * FEC mode (we only do RS)
	 */
	if (fec_ability & AN_CW_F_25G_RS_REQ) {
		/* ignore for now - set in config */
	} else if (fec_ability & AN_CW_F_25G_BASER_REQ) {
		sbl_dev_warn(sbl->dev, "an %d: cannot do fec mode baser",
				port_num);
	} else {
		sbl_dev_dbg(sbl->dev, "an %d: no matching fec mode (partner fec 0x%x)",
				port_num, fec_ability);
	}

	/*
	 * TODO pause
	 */

	/*
	 * link mode (speed)
	 */
	if (tech_ability & AN_CW_A_200GBASE_CR4) {
		link->link_mode = SBL_LINK_MODE_BS_200G;
	} else if (tech_ability & AN_CW_A_100GBASE_CR4) {
		link->link_mode = SBL_LINK_MODE_BJ_100G;
	} else if (tech_ability & AN_CW_A_100GBASE_CR2) {
		link->link_mode = SBL_LINK_MODE_CD_100G;
	} else if (tech_ability & AN_CW_A_50GBASE_KR) {
		link->link_mode = SBL_LINK_MODE_CD_50G;
	} else {
		sbl_dev_err(sbl->dev, "an %d: no matching mode (partner ability 0x%x)",
				port_num, tech_ability);
		return -ENOENT;
	}

	/*
	 * look at next page(s)
	 */
	link->lp_subtype = SBL_LP_SUBTYPE_UNKNOWN;
	for (x = 1; x < link->an_rx_count; ++x) {

		/* do message page */
		if (link->an_rx_page[x] & AN_NP_MP_MASK) {
			last_msg_code = link->an_rx_page[x] & AN_NP_MSG_MASK;
			sbl_dev_dbg(sbl->dev, "an %d: msg_code = %llu", port_num, last_msg_code);
			switch (last_msg_code) {
			case AN_NP_CODE_NULL_MSG:
			case AN_NP_CODE_OUI_MSG:
				/* nothing to do */
				break;
			case AN_NP_CODE_OUI_EXTENDED_MSG:
				switch (link->an_rx_page[x] & AN_NP_OUI_MASK) {
				case AN_NP_OUI_HPE:
					/* have HPE OUI message - expect an unformatted page next */
					expect_hpe = true;
					break;
				default:
					sbl_dev_dbg(sbl->dev, "an %d: unknown OUI = 0x%llX", port_num, (link->an_rx_page[x] & AN_NP_OUI_MASK) >> AN_NP_OUI_BASE_BIT);
					expect_hpe = false;
					break;
				}
				break;
			default:
				sbl_dev_dbg(sbl->dev, "an %d: unknown msg_code = 0x%llX", port_num, last_msg_code);
				break;
			}
			/* go on to next page */
			continue;
		}

		/* do unformatted page */
		switch (last_msg_code) {
		case AN_NP_CODE_OUI_EXTENDED_MSG:
			if (expect_hpe) {
				/* got a HPE OUI message - check the unformatted page and set options */
				switch (link->an_rx_page[x] & AN_NP_OUI_VER_MASK) {
				case AN_NP_OUI_VER_0_1:
					/* save away the LLR options here */
					if (link->blattr.llr_mode == SBL_LLR_MODE_AUTO) {
						if (!(link->blattr.options & SBL_OPT_DISABLE_AN_LLR)) {
							if (link->an_rx_page[x] & (AN_OPT_LLR << AN_OPT_BASE_BIT)) {
								link->an_options |= AN_OPT_LLR;
							}
							if (link->an_rx_page[x] & (AN_OPT_ETHER_LLR << AN_OPT_BASE_BIT)) {
								if (link->blattr.options & SBL_OPT_ENABLE_ETHER_LLR) {
									link->an_options |= AN_OPT_ETHER_LLR;
								}
							}
							if (link->an_rx_page[x] & (AN_OPT_HPC_WITH_LLR << AN_OPT_BASE_BIT)) {
								if (link->blattr.options & SBL_OPT_ENABLE_IFG_HPC_WITH_LLR) {
									link->an_options |= AN_OPT_HPC_WITH_LLR;
								}
							}
							/* TODO: add IPV4 option */
						}
					}
					/* save away cassini version here */
					link->lp_subtype = ((link->an_rx_page[x] >> AN_LP_SUBTYPE_BASE_BIT) & AN_LP_SUBTYPE_MASK);
					sbl_dev_dbg(sbl->dev, "an %d: HPE an_options = 0x%X, lp_subtype = %d", port_num, link->an_options, link->lp_subtype);
					break;
				default:
					sbl_dev_dbg(sbl->dev, "an %d: unknown OUI version 0x%llX", port_num, link->an_rx_page[x] & AN_NP_OUI_VER_MASK);
					break;
				}
				expect_hpe = false;
			}
			break;
		default:
			sbl_dev_dbg(sbl->dev, "an %d: unknown ufp = 0x%llX", port_num, link->an_rx_page[x]);
			break;
		}
	}

	return 0;
}


static void sbl_an_update_timeout(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	unsigned int new_timeout = 0;

	if (link->blattr.options & SBL_OPT_AUTONEG_TIMEOUT_SSHOT) {
		/* use slingshot timeouts - which are longer than the IEEE defaults */
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			new_timeout = SBL_LINK_AUTONEG_TIMEOUT_SSHOT_200;
			break;
		case SBL_LINK_MODE_BJ_100G:
		case SBL_LINK_MODE_CD_100G:
			new_timeout = SBL_LINK_AUTONEG_TIMEOUT_SSHOT_100;
			break;
		case SBL_LINK_MODE_CD_50G:
			new_timeout = SBL_LINK_AUTONEG_TIMEOUT_SSHOT_50;
			break;
		default:
			sbl_dev_warn(sbl->dev, "an %d: bad mode in update_timeout",
					port_num);
		}
	}

	if (link->blattr.options & SBL_OPT_AUTONEG_TIMEOUT_IEEE) {
		/* use IEEE timeouts */
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			new_timeout = SBL_LINK_AUTONEG_TIMEOUT_IEEE_200;
			break;
		case SBL_LINK_MODE_BJ_100G:
		case SBL_LINK_MODE_CD_100G:
			new_timeout = SBL_LINK_AUTONEG_TIMEOUT_IEEE_100;
			break;
		case SBL_LINK_MODE_CD_50G:
			new_timeout = SBL_LINK_AUTONEG_TIMEOUT_IEEE_50;
			break;
		default:
			sbl_dev_warn(sbl->dev, "an %d: bad mode in update_timeout",
					port_num);
		}
	}

	if (new_timeout) {
		sbl_dev_dbg(sbl->dev, "an %d: update start timeout to %d ms",
				port_num, new_timeout);
		link->an_timeout_active = true;
		sbl_link_update_start_timeout(sbl, port_num, new_timeout);
	}
}

const char *sbl_an_get_sm_state(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_pcs_autoneg_reg;
	u64 sm_state;

	sts_pcs_autoneg_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_OFFSET);
	sm_state = SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(sts_pcs_autoneg_reg);

	return  sbl_an_state_str(sm_state);
}

/*
 * return an pages
 *
 *    on input count is the number of pages alocated for pages
 *    on out this is the number of pages received
 *    if there is sufficient space in pages, received pages will be copied
 */
int sbl_get_an_pages(struct sbl_inst *sbl, int port_num, int *count, u64 *pages)
{
	struct sbl_link *link;
	int i;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	if (!count || !pages)
		return -EINVAL;

	link = sbl->link + port_num;

	/* check we did autoneg successfully */
	if ((link->blattr.pec.an_mode != SBL_AN_MODE_ON) &&
		(link->blattr.pec.an_mode != SBL_AN_MODE_FIXED)) {
		sbl_dev_info(sbl->dev, "%d: not autoneg mode - no pages to get", port_num);
		return -ENODATA;
	}
	if (!link->an_rx_count) {
		sbl_dev_info(sbl->dev, "%d: no an pages to get", port_num);
		return -ENODATA;
	}

	if (link->an_rx_count > *count) {
		/*
		 * insufficient space
		 * update size to say how much we need and return
		 */
		*count = link->an_rx_count;
		return -ENOSPC;
	}

	for (i = 0; i < link->an_rx_count; ++i)
		*pages++ = link->an_rx_page[i];
	*count = link->an_rx_count;

	return 0;
}
EXPORT_SYMBOL(sbl_get_an_pages);


static void sbl_an_dump_state(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	int i;

	val64 = sbl_read64(sbl, base|SBL_PML_ERR_FLG_OFFSET);
	sbl_dev_dbg(sbl->dev, "an %d: dump: err flgs       0x%llx", port_num, val64);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	sbl_dev_dbg(sbl->dev, "an %d: dump: cfg pcs        0x%llx", port_num, val64);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET);
	sbl_dev_dbg(sbl->dev, "an %d: dump: cfg pcs an     0x%llx", port_num, val64);

	val64 = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_OFFSET);
	sbl_dev_dbg(sbl->dev, "an %d: dump: sts an base pg 0x%llx", port_num, val64);

	sbl_dev_dbg(sbl->dev, "%d base, complete (%lld), state (%lld) (%s), received (%lld) , base page (%lld), lp (%lld), lp_base (%lld) ",
			port_num,
			SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_COMPLETE_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_STATE_GET(val64),
			sbl_an_get_sm_state(sbl, port_num),
			SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_PAGE_RECEIVED_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_BASE_PAGE_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_ABILITY_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_BASE_PAGE_LP_BASE_PAGE_GET(val64));

	val64 = sbl_read64(sbl, base|SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_OFFSET);

	sbl_dev_dbg(sbl->dev, "%d next, complete (%lld), state (%lld) (%s), received (%lld) , base page (%lld), lp (%lld), lp_next (%lld) ",
			port_num,
			SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_COMPLETE_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_STATE_GET(val64),
			sbl_an_get_sm_state(sbl, port_num),
			SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_PAGE_RECEIVED_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_BASE_PAGE_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_ABILITY_GET(val64),
			SBL_PML_STS_PCS_AUTONEG_NEXT_PAGE_LP_NEXT_PAGE_GET(val64));

	sbl_dev_dbg(sbl->dev, "an %d: dump: sts an next pg 0x%llx", port_num, val64);

	for (i = 0; i < link->an_tx_count; ++i) {
		sbl_dev_dbg(sbl->dev, "an %d: dump: tx pg %d: 0x%llx", port_num,
			i, link->an_tx_page[i]);
	}
	for (i = 0; i < link->an_rx_count; ++i) {
		sbl_dev_dbg(sbl->dev, "an %d: dump: rx pg %d: 0x%llx", port_num,
			i, link->an_rx_page[i]);
	}
}
