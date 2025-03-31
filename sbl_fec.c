// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sbl/sbl_pml.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_serdes_map.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_an.h"
#include "sbl_serdes.h"
#include "sbl_serdes_fn.h"
#include "sbl_internal.h"
#include "sbl_fec.h"

static int sbl_fec_rates_update(struct sbl_inst *sbl, int port_num, u32 window);
static bool sbl_fec_ccw_rate_bad(struct sbl_inst *sbl, int port_num,
				u32 thresh_adj, bool use_stp_thresh);
static bool sbl_fec_ucw_rate_bad(struct sbl_inst *sbl, int port_num,
				u32 thresh_adj);
static void sbl_fec_counts_zero(struct sbl_inst *sbl, int port_num,
				struct sbl_pcs_fec_cntrs *cntrs);
static u64 sbl_fec_rate_calc(struct sbl_inst *sbl, int port_num,
				u64 curr, u64 prev, unsigned long tdiff);
static bool sbl_fec_txr_rate_bad(struct sbl_inst *sbl, int port_num,
				u32 thresh_adj);
static void sbl_fec_rates_warnings(struct sbl_inst *sbl, int port_num,
				int *warning_count);

void sbl_fec_thresholds_clear(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	sbl_dev_dbg(sbl->dev, "%d: clearing fec thresholds", port_num);

	spin_lock(&fec_prmts->fec_cw_lock);
	fec_prmts->fec_ucw_thresh = 0;
	fec_prmts->fec_ccw_thresh = 0;
	fec_prmts->fec_stp_ccw_thresh = 0;
	fec_prmts->fecl_warn  = 0;
	spin_unlock(&fec_prmts->fec_cw_lock);
}
EXPORT_SYMBOL(sbl_fec_thresholds_clear);

/*
 *setup the fec thresholds
 *
 * The input args can either be (negative) flags or
 * explicit (positive) numbers.
 * A threshold of zero will disable threshold detection
 */
int sbl_fec_thresholds_set(struct sbl_inst *sbl, int port_num,
		s32 ucw_in, s32 ccw_in)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	int link_mode;
	u64 ucw;
	u64 ccw;
	u64 stp_ccw;
	u64 fecl_warn  = 0;
	int err = 0;

	/*
	 * Uncorrected codeword threshold
	 */
	switch (ucw_in) {
	case SBL_LINK_FEC_INVALID:
		sbl_dev_err(sbl->dev, "%d: link ucw invalid", port_num);
		goto fail;

	case SBL_LINK_FEC_OFF:
		ucw = 0;
		break;

	case SBL_LINK_FEC_IEEE:
		ucw = sbl_link_get_ucw_thresh_ieee(sbl, port_num);
		break;

	case SBL_LINK_FEC_HPE:
		ucw = sbl_link_get_ucw_thresh_hpe(sbl, port_num);
		break;

	default:

		if (ucw_in < 0) {
			sbl_dev_err(sbl->dev, "%d: bad link attr ucw (%d)", port_num,
					ucw_in);
			goto fail;
		}

		/* explicit number, just use it */
		ucw = ucw_in;
		break;
	}

	/*
	 * Corrected codeword threshold
	 */
	switch (ccw_in) {
	case SBL_LINK_FEC_INVALID:
		sbl_dev_err(sbl->dev, "%d: attr ccw invalid", port_num);
		goto fail;

	case SBL_LINK_FEC_OFF:
		ccw = 0;
		stp_ccw = 0;
		break;

	case SBL_LINK_FEC_IEEE:
		ccw = sbl_link_get_ccw_thresh_ieee(sbl, port_num);
		stp_ccw = sbl_link_get_stp_ccw_thresh_ieee(sbl, port_num);
		break;

	case SBL_LINK_FEC_HPE:
		ccw = sbl_link_get_ccw_thresh_hpe(sbl, port_num);
		stp_ccw = sbl_link_get_stp_ccw_thresh_hpe(sbl, port_num);
		break;

	default:

		if (ccw_in < 0) {
			sbl_dev_err(sbl->dev, "%d:bad link attr ccw (%d)", port_num,
					ccw_in);
			goto fail;
		}

		/* explicit number, just use it */
		ccw = ccw_in;
		stp_ccw = ccw_in;
		break;
	}

	/*
	 * ccw warning thresholds
	 */
	err = sbl_base_link_get_status(sbl, port_num,
			NULL, NULL, NULL, NULL, NULL, &link_mode);

	if (err) {
		sbl_dev_warn(sbl->dev, "%d:thresh, get mode failed [%d]-warnings disabled", port_num, err);
		fecl_warn = 0;
	} else {

		switch (link_mode) {
		case SBL_LINK_MODE_BS_200G:
			fecl_warn = ccw/8;
			break;
		case SBL_LINK_MODE_BJ_100G:
			fecl_warn = ccw/8;
			break;
		case SBL_LINK_MODE_CD_100G:
			fecl_warn = ccw/4;
			break;
		case SBL_LINK_MODE_CD_50G:
			fecl_warn = ccw/2;
			break;
		default:
			fecl_warn = 0;
			break;
		}
	}

	sbl_dev_dbg(sbl->dev, "%d: Setting fec thresh ucw %lld, ccw %lld, warn %lld",
			port_num, ucw, ccw, fecl_warn);

	spin_lock(&fec_prmts->fec_cw_lock);
	fec_prmts->fec_ucw_thresh = ucw;
	fec_prmts->fec_ccw_thresh = ccw;
	fec_prmts->fec_stp_ccw_thresh = stp_ccw;
	fec_prmts->fec_llr_tx_replay_thresh = SBL_FEC_LLR_TX_REPLAY_THRESH;
	fec_prmts->fecl_warn = fecl_warn;
	spin_unlock(&fec_prmts->fec_cw_lock);

	return 0;
fail:
	sbl_dev_warn(sbl->dev, "%d: fec monitoring disabled", port_num);
	sbl_fec_thresholds_clear(sbl, port_num);
	return -ENAVAIL;
}
EXPORT_SYMBOL(sbl_fec_thresholds_set);

/*
 * setup the fec threshold adjustments
 *
 *	An adjustment of zero means disable the test
 */
int sbl_fec_adjustments_set(struct sbl_inst *sbl, int port_num,
		u32 ucw_adj, u32 ccw_adj)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	spin_lock(&fec_prmts->fec_cw_lock);
	fec_prmts->fec_ucw_down_thresh_adj = ucw_adj;
	fec_prmts->fec_ccw_down_thresh_adj = ccw_adj;
	spin_unlock(&fec_prmts->fec_cw_lock);

	sbl_dev_dbg(sbl->dev, "%d: Setting fec thresh adjustments ucw_adj %d, ccw_adj %d",
			port_num, ucw_adj, ccw_adj);

	return 0;
}
EXPORT_SYMBOL(sbl_fec_adjustments_set);

int sbl_fec_up_check(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;
	bool ucw_err;
	bool ccw_err	 = false;
	bool stp_ccw_err = false;
	u32 ucw_thresh_adj;
	u32 ccw_thresh_adj;
	u32 stp_ccw_thresh_adj;
	int i;

	spin_lock(&fec_prmts->fec_cw_lock);
	ucw_thresh_adj = fec_prmts->fec_ucw_up_thresh_adj;
	ccw_thresh_adj = fec_prmts->fec_ccw_up_thresh_adj;
	stp_ccw_thresh_adj = fec_prmts->fec_stp_ccw_up_thresh_adj;
	spin_unlock(&fec_prmts->fec_cw_lock);

	/* initial measurement */
	msleep(SBL_FEC_UP_SETTLE_PERIOD); /* time for fec rates to settle */
	sbl_fec_counts_get(sbl, port_num, fec_prmts->fec_curr_cnts);

	for (i = 0; i < (link->blattr.options & SBL_OPT_FABRIC_LINK ?
			 SBL_FEC_UP_COUNT_FABRIC : SBL_FEC_UP_COUNT_EDGE); ++i) {

		msleep(SBL_FEC_UP_WINDOW);
		sbl_fec_rates_update(sbl, port_num, SBL_FEC_UP_WINDOW);

		ucw_err = sbl_fec_ucw_rate_bad(sbl, port_num, ucw_thresh_adj);
		if (ucw_err)
			sbl_dev_err(sbl->dev, "%d: fec up check: ucw fail", port_num);

		if ((link->dfe_tune_count == SBL_DFE_USED_SAVED_PARAMS) && (stp_ccw_thresh_adj > 0))
			stp_ccw_err = sbl_fec_ccw_rate_bad(sbl, port_num, stp_ccw_thresh_adj, true);
		else
			ccw_err = sbl_fec_ccw_rate_bad(sbl, port_num, ccw_thresh_adj, false);

		if (ccw_err)
			sbl_dev_err(sbl->dev, "%d: fec up check: ccw fail", port_num);
		else if (stp_ccw_err)
			sbl_dev_err(sbl->dev, "%d: fec up check: stp ccw fail", port_num);

		if (ucw_err || ccw_err || stp_ccw_err) {
			sbl_link_counters_incr(sbl, port_num, fec_up_fail);
			return -EOVERFLOW;
		}
	}

	return 0;
}

void sbl_fec_counts_get(struct sbl_inst *sbl, int port_num,
		struct sbl_pcs_fec_cntrs *cntrs)
{
	cntrs->fecl[0] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_00_ADDR(port_num));
	cntrs->fecl[1] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_01_ADDR(port_num));
	cntrs->fecl[2] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_02_ADDR(port_num));
	cntrs->fecl[3] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_03_ADDR(port_num));
	cntrs->fecl[4] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_04_ADDR(port_num));
	cntrs->fecl[5] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_05_ADDR(port_num));
	cntrs->fecl[6] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_06_ADDR(port_num));
	cntrs->fecl[7] = sbl_read64(sbl, SBL_PCS_FECL_ERRORS_07_ADDR(port_num));

	cntrs->ccw = sbl_read64(sbl, SBL_PCS_CORRECTED_CW_ADDR(port_num));
	cntrs->ucw = sbl_read64(sbl, SBL_PCS_UNCORRECTED_CW_ADDR(port_num));

	cntrs->llr_tx_replay = sbl_read64(sbl, SBL_LLR_TX_REPLAY_EVENT_ADDR(port_num));

	cntrs->time = jiffies;
}

static int sbl_fec_rates_update(struct sbl_inst *sbl, int port_num,
				u32 window)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;
	struct sbl_pcs_fec_cntrs *temp;
	unsigned long tdiff;
	int i;
	bool discard_rates;
	int reason;
	unsigned long irq_flags;

	spin_lock(&fec_prmts->fec_cnt_lock);

	/*
	 * make sure the period is sufficient to update rates
	 * otherwise zero rates so nothing fires
	 */
	if (time_is_after_jiffies(fec_prmts->fec_curr_cnts->time + msecs_to_jiffies(window))) {
		sbl_fec_counts_zero(sbl, port_num, fec_prmts->fec_rates);
		spin_unlock(&fec_prmts->fec_cnt_lock);

		return -EINPROGRESS;
	}

	temp = fec_prmts->fec_prev_cnts;
	fec_prmts->fec_prev_cnts = fec_prmts->fec_curr_cnts;
	fec_prmts->fec_curr_cnts = temp;
	sbl_fec_counts_get(sbl, port_num, fec_prmts->fec_curr_cnts);

	spin_lock_irqsave(&link->fec_discard_lock, irq_flags);
	discard_rates = (link->fec_discard_time >= fec_prmts->fec_prev_cnts->time) &&
			(link->fec_discard_time < fec_prmts->fec_curr_cnts->time);
	reason = link->fec_discard_type;
	spin_unlock_irqrestore(&link->fec_discard_lock, irq_flags);

	if (discard_rates) {
		sbl_dev_dbg(sbl->dev, "%d: %s - ignoring FEC rates for the current window", port_num,
			    sbl_fec_discard_str(reason));
		sbl_fec_counts_zero(sbl, port_num, fec_prmts->fec_rates);
		spin_unlock(&fec_prmts->fec_cnt_lock);
		return -EINTR;
	}

	/*
	 * calc time difference  (HZ)
	 * (unsigned so rollover ok)
	 */
	tdiff = fec_prmts->fec_curr_cnts->time - fec_prmts->fec_prev_cnts->time;

	/* calculate rates */
	fec_prmts->fec_rates->ccw = sbl_fec_rate_calc(sbl, port_num,
			fec_prmts->fec_curr_cnts->ccw, fec_prmts->fec_prev_cnts->ccw, tdiff);
	fec_prmts->fec_rates->ucw = sbl_fec_rate_calc(sbl, port_num,
			fec_prmts->fec_curr_cnts->ucw, fec_prmts->fec_prev_cnts->ucw, tdiff);
	fec_prmts->fec_rates->llr_tx_replay = sbl_fec_rate_calc(sbl, port_num,
			fec_prmts->fec_curr_cnts->llr_tx_replay, fec_prmts->fec_prev_cnts->llr_tx_replay, tdiff);

	for (i = 0; i < SBL_PCS_NUM_FECL_CNTRS; ++i)
		fec_prmts->fec_rates->fecl[i] = sbl_fec_rate_calc(sbl, port_num,
				fec_prmts->fec_curr_cnts->fecl[i], fec_prmts->fec_prev_cnts->fecl[i], tdiff);

	fec_prmts->fec_rates->time = jiffies_to_msecs(tdiff);

	spin_unlock(&fec_prmts->fec_cnt_lock);

	return 0;

}

/*
 * returns true if the uncorrected code word rate is bad
 *
 *	 threshold adjustment is a percentage
 */
static bool sbl_fec_ucw_rate_bad(struct sbl_inst *sbl, int port_num,
				u32 thresh_adj)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	bool ignore_err;
	u64 ucw_bad;
	u64 ucw_hwm;

	spin_lock(&fec_prmts->fec_cw_lock);
	ucw_hwm = fec_prmts->fec_ucw_hwm;
#ifdef CONFIG_SBL_PLATFORM_ROS_HW
	ucw_bad = fec_prmts->fec_ucw_thresh;
#else
	ucw_bad = 21;
#endif
	spin_unlock(&fec_prmts->fec_cw_lock);

	/*
	 * update high water mark
	 *
	 *	 we record this even if no test is performed so we can use it for
	 *	 debug/calibration
	 */
	if (fec_prmts->fec_rates->ucw > ucw_hwm) {
		spin_lock(&fec_prmts->fec_cw_lock);
		fec_prmts->fec_ucw_hwm = fec_prmts->fec_rates->ucw;
		spin_unlock(&fec_prmts->fec_cw_lock);
	}

	/* apply percentage adjustment */
	ucw_bad = ucw_bad * thresh_adj / 100;

	if (ucw_bad == 0) {
		sbl_dev_dbg(sbl->dev, "%d: fec ucw test ignored, threshold is zero", port_num);
		return false;
	}

	/*
	 * check uncorrected code words
	 */
	if (fec_prmts->fec_rates->ucw > ucw_bad) {
		sbl_link_counters_incr(sbl, port_num, fec_ucw_err);
		ignore_err = sbl_debug_option(sbl, port_num, SBL_DEBUG_IGNORE_HIGH_FEC_UCW);

		sbl_dev_err(sbl->dev, "%d: bad ucw, ccw %lld, ucw %lld (>%lld), (%lld %lld %lld %lld %lld %lld %lld %lld), window %ldms%s\n",
				    port_num, fec_prmts->fec_rates->ccw,
				    fec_prmts->fec_rates->ucw, ucw_bad,
				    fec_prmts->fec_rates->fecl[0], fec_prmts->fec_rates->fecl[1],
				    fec_prmts->fec_rates->fecl[2], fec_prmts->fec_rates->fecl[3],
				    fec_prmts->fec_rates->fecl[4], fec_prmts->fec_rates->fecl[5],
				    fec_prmts->fec_rates->fecl[6], fec_prmts->fec_rates->fecl[7],
				    fec_prmts->fec_rates->time,
				    ignore_err ? " -ignored" : "");
		if (ignore_err)
			return false;
		else
			return true;
	}
	return false;

}

/*
 * returns true if the corrected code word rate is bad
 */
static bool sbl_fec_ccw_rate_bad(struct sbl_inst *sbl, int port_num,
				u32 thresh_adj, bool use_stp_thresh)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	u64 ccw_bad;
	bool ignore_err;
	u64 ccw_hwm;

	spin_lock(&fec_prmts->fec_cw_lock);
	ccw_hwm = fec_prmts->fec_ccw_hwm;
#ifdef CONFIG_SBL_PLATFORM_ROS_HW
	if (use_stp_thresh)
		ccw_bad = fec_prmts->fec_stp_ccw_thresh;
	else
		ccw_bad = fec_prmts->fec_ccw_thresh;
#else
	ccw_bad = 21250000;
#endif
	spin_unlock(&fec_prmts->fec_cw_lock);

	/*
	 * update high water mark
	 *
	 *	 we record this even if no test is performed so we can use it for
	 *	 debug/calibration
	 */
	if (fec_prmts->fec_rates->ccw > ccw_hwm) {
		spin_lock(&fec_prmts->fec_cw_lock);
		fec_prmts->fec_ccw_hwm = fec_prmts->fec_rates->ccw;
		spin_unlock(&fec_prmts->fec_cw_lock);
	}

	/*
	 * apply percentage adjustment
	 */
	ccw_bad = ccw_bad * thresh_adj / 100;

	if (ccw_bad == 0) {
		sbl_dev_dbg(sbl->dev, "%d: fec ccw test ignored, threshold is zero", port_num);

		return false;
	}

	/*
	 * check corrected code words
	 */
	if (fec_prmts->fec_rates->ccw > ccw_bad) {
		sbl_link_counters_incr(sbl, port_num, fec_ccw_err);
		ignore_err = sbl_debug_option(sbl, port_num, SBL_DEBUG_IGNORE_HIGH_FEC_CCW);

		sbl_dev_err(sbl->dev, "%d: bad ccw, ccw %lld (>%lld), ucw %lld, (%lld %lld %lld %lld %lld %lld %lld %lld), window %ldms%s\n",
				port_num, fec_prmts->fec_rates->ccw, ccw_bad,
				fec_prmts->fec_rates->ucw,
				fec_prmts->fec_rates->fecl[0], fec_prmts->fec_rates->fecl[1],
				fec_prmts->fec_rates->fecl[2], fec_prmts->fec_rates->fecl[3],
				fec_prmts->fec_rates->fecl[4], fec_prmts->fec_rates->fecl[5],
				fec_prmts->fec_rates->fecl[6], fec_prmts->fec_rates->fecl[7],
				fec_prmts->fec_rates->time,
				ignore_err ? " -ignored" : "");
		if (ignore_err)
			return false;
		else
			return true;
	}
	return false;
}

static void sbl_fec_counts_zero(struct sbl_inst *sbl, int port_num,
				struct sbl_pcs_fec_cntrs *cntrs)
{
	memset(cntrs, 0, sizeof(struct sbl_pcs_fec_cntrs));
}

static u64 sbl_fec_rate_calc(struct sbl_inst *sbl, int port_num,
		u64 curr, u64 prev, unsigned long tdiff)
{
	if (curr < prev) {
		sbl_dev_err(sbl->dev, "%d: fec counter went backwards", port_num);

		return 0;
	} else
		return (curr - prev) * HZ / tdiff;
}

void sbl_fec_timer_work(struct work_struct *work)
{
	u32 ucw_thresh_adj;
	u32 ccw_thresh_adj;
	int port_num;
	unsigned long irq_flags;
	struct sbl_link *link;
	u32 down_origin = 0;
	struct sbl_fec *fec_prmts;
	int warning_count = SBL_MAX_FEC_WARNINGS;
	int err;
	struct fec_data *fec_data = container_of(work, struct fec_data, fec_timer_work);
	struct sbl_inst *sbl = fec_data->sbl;

	port_num = fec_data->port_num;
	link = sbl->link + port_num;
	fec_prmts = fec_data->fec_prmts;

	if ((link->blstate == SBL_BASE_LINK_STATUS_UP) && (sbl_pml_pcs_aligned(sbl, port_num))) {
		err = sbl_fec_rates_update(sbl, port_num, SBL_FEC_MON_PERIOD);

		if (!err) {
			spin_lock_irqsave(&fec_prmts->fec_cw_lock, irq_flags);
			ucw_thresh_adj = fec_prmts->fec_ucw_down_thresh_adj;
			ccw_thresh_adj = fec_prmts->fec_ccw_down_thresh_adj;
			spin_unlock_irqrestore(&fec_prmts->fec_cw_lock, irq_flags);

			if (sbl_fec_ucw_rate_bad(sbl, port_num, ucw_thresh_adj)) {
				/* take the link down */
				down_origin = SBL_LINK_DOWN_ORIGIN_UCW;
				sbl_pml_link_down_async_alert(sbl, port_num, down_origin);
				return;
			}

			if (sbl_fec_ccw_rate_bad(sbl, port_num, ccw_thresh_adj, false)) {
				/* take the link down */
				down_origin = SBL_LINK_DOWN_ORIGIN_CCW;
				sbl_pml_link_down_async_alert(sbl, port_num, down_origin);
				return;
			}

			if (sbl_fec_txr_rate_bad(sbl, port_num, 0)) {
				/* take the link down */
				down_origin = SBL_LINK_DOWN_ORIGIN_LLR_TX_REPLAY;
				sbl_pml_link_down_async_alert(sbl, port_num, down_origin);
				return;
			}
			sbl_fec_rates_warnings(sbl, port_num, &warning_count);

		}

	}
	mod_timer(&fec_data->fec_timer, jiffies + msecs_to_jiffies(SBL_FEC_MON_PERIOD));
}

static bool sbl_fec_txr_rate_bad(struct sbl_inst *sbl, int port_num,
		u32 thresh_adj)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	u64 llr_tx_replay_bad;
	u64 llr_tx_replay_hwm;
	bool ignore_err;

	spin_lock(&fec_prmts->fec_cw_lock);
	llr_tx_replay_bad = fec_prmts->fec_llr_tx_replay_thresh;
	llr_tx_replay_hwm = fec_prmts->fec_llr_tx_replay_hwm;
	spin_unlock(&fec_prmts->fec_cw_lock);

	/*
	 * update high water mark
	 *
	 *	 we record this even if no test is performed so we can use it for
	 *	 debug/calibration
	 */
	if (fec_prmts->fec_rates->llr_tx_replay > llr_tx_replay_hwm) {
		spin_lock(&fec_prmts->fec_cw_lock);
		fec_prmts->fec_llr_tx_replay_hwm = fec_prmts->fec_rates->llr_tx_replay;
		spin_unlock(&fec_prmts->fec_cw_lock);
	}
	if (llr_tx_replay_bad == 0) {
		sbl_dev_dbg(sbl->dev, "%d: fec llr_tx_replay test ignored, threshold is zero", port_num);
		return false;
	}

	/*
	 * check llr_tx_replay rate
	 */
	if (fec_prmts->fec_rates->llr_tx_replay > llr_tx_replay_bad) {
		sbl_link_counters_incr(sbl, port_num, fec_txr_err);
		ignore_err = sbl_debug_option(sbl, port_num, SBL_DEBUG_IGNORE_HIGH_FEC_TXR);

		sbl_dev_err(sbl->dev, "%d: bad llr_tx_replay, llr_tx_replay %lld (>%lld), window %ldms%s\n",
				port_num,
				fec_prmts->fec_rates->llr_tx_replay, llr_tx_replay_bad,
				fec_prmts->fec_rates->time,
				ignore_err ? " -ignored" : "");

		if (ignore_err)
			return false;
		else
			return true;
	}
	return false;
}

/*
 * print a few warnings (so we can see how things are changing) about high
 * fec lane error rates
 */
static void sbl_fec_rates_warnings(struct sbl_inst *sbl, int port_num,
		int *warning_count)
{

	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;
	int fecl_warn;
	int i;

	spin_lock(&fec_prmts->fec_cw_lock);
	fecl_warn = fec_prmts->fecl_warn;
	spin_unlock(&fec_prmts->fec_cw_lock);

	if (fecl_warn == 0) {
		dev_dbg(sbl->dev, "%d: fec ccw warn ignored, threshold is zero", port_num);

		return;
	}

	for (i = 0; i < SBL_PCS_NUM_FECL_CNTRS; ++i) {
		if (fec_prmts->fec_rates->fecl[i] > fecl_warn) {
			sbl_link_counters_incr(sbl, port_num, fec_warn);

			if (*warning_count > 0) {
				dev_warn(sbl->dev, "sbl %d: warning, ccw %lld, (%lld %lld %lld %lld %lld %lld %lld %lld)%s\n",
						port_num, fec_prmts->fec_rates->ccw,
						fec_prmts->fec_rates->fecl[0], fec_prmts->fec_rates->fecl[1],
						fec_prmts->fec_rates->fecl[2], fec_prmts->fec_rates->fecl[3],
						fec_prmts->fec_rates->fecl[4], fec_prmts->fec_rates->fecl[5],
						fec_prmts->fec_rates->fecl[6], fec_prmts->fec_rates->fecl[7],
						(*warning_count == 1) ? " - last" : "");
				--*warning_count;
			} else {
				dev_dbg(sbl->dev, "sbl %d: warning, ccw %lld, (%lld %lld %lld %lld %lld %lld %lld %lld)\n",
						port_num, fec_prmts->fec_rates->ccw,
						fec_prmts->fec_rates->fecl[0], fec_prmts->fec_rates->fecl[1],
						fec_prmts->fec_rates->fecl[2], fec_prmts->fec_rates->fecl[3],
						fec_prmts->fec_rates->fecl[4], fec_prmts->fec_rates->fecl[5],
						fec_prmts->fec_rates->fecl[6], fec_prmts->fec_rates->fecl[7]);
			}
			return;
		}
	}
}

void sbl_zero_all_fec_counts(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	spin_lock(&fec_prmts->fec_cnt_lock);
	memset(fec_prmts->fec_curr_cnts, 0, sizeof(struct sbl_pcs_fec_cntrs));
	memset(fec_prmts->fec_prev_cnts, 0, sizeof(struct sbl_pcs_fec_cntrs));
	memset(fec_prmts->fec_rates, 0, sizeof(struct sbl_pcs_fec_cntrs));
	spin_unlock(&fec_prmts->fec_cnt_lock);
}

void sbl_fec_timer(struct timer_list *timer)
{
	int port_num;
	struct sbl_link *link;
	struct fec_data *fec_data = from_timer(fec_data, timer, fec_timer);
	struct sbl_inst *sbl = fec_data->sbl;

	port_num = fec_data->port_num;
	link = sbl->link + port_num;

	if (!queue_work(sbl->workq, &(link->fec_data->fec_timer_work)))
		sbl_dev_warn(sbl->dev, "fec timer work already queued");
}

#ifdef CONFIG_SYSFS
int sbl_fec_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	u64 ucw_thresh;
	u64 ucw_hwm;
	u64 ccw_thresh;
	u64 ccw_hwm;
	u64 llr_tx_replay_thresh;
	u64 llr_tx_replay_hwm;
	u64 fecl_warn;
	int s = 0;
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;


	spin_lock(&fec_prmts->fec_cw_lock);
	if (link->blstate == SBL_BASE_LINK_STATUS_STARTING) {
		ucw_thresh = fec_prmts->fec_ucw_thresh * fec_prmts->fec_ucw_up_thresh_adj / 100;
		ccw_thresh = fec_prmts->fec_ccw_thresh * fec_prmts->fec_ccw_up_thresh_adj / 100;
	} else {
		ucw_thresh = fec_prmts->fec_ucw_thresh * fec_prmts->fec_ucw_down_thresh_adj / 100;
		ccw_thresh = fec_prmts->fec_ccw_thresh * fec_prmts->fec_ccw_down_thresh_adj / 100;
	}

	llr_tx_replay_thresh = fec_prmts->fec_llr_tx_replay_thresh;
	ucw_hwm = fec_prmts->fec_ucw_hwm;
	ccw_hwm = fec_prmts->fec_ccw_hwm;
	llr_tx_replay_hwm = fec_prmts->fec_llr_tx_replay_hwm;
	fecl_warn = fec_prmts->fecl_warn;
	spin_unlock(&fec_prmts->fec_cw_lock);
	if (link->blstate == SBL_BASE_LINK_STATUS_UP) {

		s += snprintf(buf+s, size-s, "fec monitor: hwm- ccw %lld/%lld, ucw %lld/%lld, llr_tx_replay %lld/%lld, warn %lld",
				ccw_hwm, ccw_thresh, ucw_hwm, ucw_thresh, llr_tx_replay_hwm, llr_tx_replay_thresh, fecl_warn);

		if (ucw_thresh == 0)
			s += snprintf(buf+s, size-s, ", ucw-off");

		if (ccw_thresh == 0)
			s += snprintf(buf+s, size-s, ", ccw-off");

		s += snprintf(buf+s, size-s, "\n");

		if (sbl_pml_pcs_aligned(sbl, port_num)) {
			spin_lock(&fec_prmts->fec_cnt_lock);
			if (fec_prmts->fec_rates) {
				s += snprintf(buf+s, size-s, "fec monitor: rates- ccw %lld, ucw %lld/%lld",
						fec_prmts->fec_rates->ccw, fec_prmts->fec_rates->ucw, fec_prmts->fec_curr_cnts->ucw);
				s += snprintf(buf+s, size-s, ", llr_tx_replay %lld, window %ld",
						fec_prmts->fec_rates->llr_tx_replay, fec_prmts->fec_rates->time);
				s += snprintf(buf+s, size-s, ", (%lld %lld %lld %lld %lld %lld %lld %lld)\n",
						fec_prmts->fec_rates->fecl[0], fec_prmts->fec_rates->fecl[1],
						fec_prmts->fec_rates->fecl[2], fec_prmts->fec_rates->fecl[3],
						fec_prmts->fec_rates->fecl[4], fec_prmts->fec_rates->fecl[5],
						fec_prmts->fec_rates->fecl[6], fec_prmts->fec_rates->fecl[7]);
			}
			spin_unlock(&fec_prmts->fec_cnt_lock);
		}
	}
	return s;
}
EXPORT_SYMBOL(sbl_fec_sysfs_sprint);
#endif

/*
 * set the llr_tx_replay rate threshold
 *
 *	An adjustment of zero means disable the test
 */
int sbl_fec_txr_rate_set(struct sbl_inst *sbl, int port_num,
		u32 txr_rate)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	spin_lock(&fec_prmts->fec_cw_lock);
	fec_prmts->fec_llr_tx_replay_thresh = txr_rate;
	spin_unlock(&fec_prmts->fec_cw_lock);

	sbl_dev_dbg(sbl->dev, "%d: Setting txr_rate %d",
			port_num, txr_rate);

	return 0;
}
EXPORT_SYMBOL(sbl_fec_txr_rate_set);

/*
 * modify adjustments
 *
 *	 (really at test/debug/tuning interface)
 */
void sbl_fec_modify_adjustments(struct sbl_inst *sbl, int port_num,
		u32 *ucw_up_adj, u32 *ccw_up_adj, u32 *ucw_down_adj, u32 *ccw_down_adj, u32 *stp_ccw_up_adj)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	spin_lock(&fec_prmts->fec_cw_lock);
	if (ucw_up_adj)
		fec_prmts->fec_ucw_up_thresh_adj = *ucw_up_adj;
	if (ccw_up_adj)
		fec_prmts->fec_ccw_up_thresh_adj = *ccw_up_adj;
	if (ucw_down_adj)
		fec_prmts->fec_ucw_down_thresh_adj = *ucw_down_adj;
	if (ccw_down_adj)
		fec_prmts->fec_ccw_down_thresh_adj = *ccw_down_adj;
	if (stp_ccw_up_adj)
		fec_prmts->fec_stp_ccw_up_thresh_adj = *stp_ccw_up_adj;
	spin_unlock(&fec_prmts->fec_cw_lock);
}
EXPORT_SYMBOL(sbl_fec_modify_adjustments);

/*
 * dump adjusted FEC thresholds
 */
void sbl_fec_dump(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;


	u64 ucw_thresh;
	u64 ccw_thresh;
	u64 stp_ccw_thresh;
	u32 ucw_up_adj;
	u32 ccw_up_adj;
	u32 stp_ccw_up_adj;
	u32 ucw_down_adj;
	u32 ccw_down_adj;

	spin_lock(&fec_prmts->fec_cw_lock);
	ucw_thresh     = fec_prmts->fec_ucw_thresh;
	ccw_thresh     = fec_prmts->fec_ccw_thresh;
	stp_ccw_thresh = fec_prmts->fec_stp_ccw_thresh;
	ucw_up_adj     = fec_prmts->fec_ucw_up_thresh_adj;
	ccw_up_adj     = fec_prmts->fec_ccw_up_thresh_adj;
	stp_ccw_up_adj = fec_prmts->fec_stp_ccw_up_thresh_adj;
	ucw_down_adj   = fec_prmts->fec_ucw_down_thresh_adj;
	ccw_down_adj   = fec_prmts->fec_ccw_down_thresh_adj;
	spin_unlock(&fec_prmts->fec_cw_lock);

	sbl_dev_info(sbl->dev, "%d: ucw : thresh %lld", port_num, ucw_thresh);
	sbl_dev_info(sbl->dev, "%d: ucw up: x%d%%, %lld", port_num,
			ucw_up_adj, ucw_thresh * ucw_up_adj / 100);
	sbl_dev_info(sbl->dev, "%d: ucw down: x%d%%, %lld", port_num,
			ucw_down_adj, ucw_thresh * ucw_down_adj / 100);

	sbl_dev_info(sbl->dev, "%d: ccw : thresh %lld", port_num, ccw_thresh);
	sbl_dev_info(sbl->dev, "%d: stp ccw : thresh %lld", port_num, stp_ccw_thresh);
	sbl_dev_info(sbl->dev, "%d: ccw up: x%d%%, %lld", port_num,
			ccw_up_adj, ccw_thresh * ccw_up_adj / 100);
	sbl_dev_info(sbl->dev, "%d: stp ccw up: x%d%%, %lld", port_num,
			stp_ccw_up_adj, stp_ccw_thresh * stp_ccw_up_adj / 100);
	sbl_dev_info(sbl->dev, "%d: ccw down: x%d%%, %lld", port_num,
			ccw_down_adj, ccw_thresh * ccw_down_adj / 100);
}
EXPORT_SYMBOL(sbl_fec_dump);

void sbl_fec_hwms_clear(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;
	struct sbl_fec *fec_prmts = fec_data->fec_prmts;

	sbl_dev_dbg(sbl->dev, "%d: clearing fec thresholds", port_num);

	spin_lock(&fec_prmts->fec_cw_lock);
	fec_prmts->fec_ucw_hwm = 0;
	fec_prmts->fec_ccw_hwm = 0;
	fec_prmts->fec_llr_tx_replay_hwm = 0;
	spin_unlock(&fec_prmts->fec_cw_lock);
}
EXPORT_SYMBOL(sbl_fec_hwms_clear);
