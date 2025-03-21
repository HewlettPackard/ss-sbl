// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sbl/sbl_pml.h>


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

static int  sbl_base_link_lp_detect(struct sbl_inst *sbl, int port_num);
static int  sbl_base_link_check_fix_fw(struct sbl_inst *sbl, int port_num);
static int  sbl_base_link_get_mode(struct sbl_inst *sbl, int port_num);
static bool sbl_base_link_an_timed_out(struct sbl_inst *sbl, int port_num, int err);
static int  sbl_link_get_mode_electrical(struct sbl_inst *sbl, int port_num);
static int  sbl_link_get_mode_optical(struct sbl_inst *sbl, int port_num);
static int  sbl_link_fault_monitor_start(struct sbl_inst *sbl, int port_num);
static int  sbl_link_fault_monitor_stop(struct sbl_inst *sbl, int port_num);
static void sbl_base_link_report_err(struct sbl_inst *sbl, char *txt,
		int port_num, int err);


int sbl_get_lp_subtype(struct sbl_inst *sbl, int port_num, enum sbl_lp_subtype *lp_subtype)
{
	int              err;
	struct sbl_link *link;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	if (lp_subtype == NULL)
		return -EINVAL;

	sbl_dev_dbg(sbl->dev, "%d: get lp subtype", port_num);

	link = sbl->link + port_num;

	*lp_subtype = link->lp_subtype;

	return 0;
}
EXPORT_SYMBOL(sbl_get_lp_subtype);


/*
 *
 * principal base link control API
 *
 *
 */
int sbl_base_link_config(struct sbl_inst *sbl, int port_num, struct sbl_base_link_attr *blattr)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "bl %d: config\n", port_num);

	if (!blattr)
		return -EINVAL;

	if (blattr->magic != SBL_LINK_ATTR_MAGIC)
		return -EINVAL;

	link = sbl->link + port_num;

	spin_lock(&link->lock);

	if (!(link->blstate & (SBL_BASE_LINK_STATUS_DOWN|SBL_BASE_LINK_STATUS_UNCONFIGURED))) {

		sbl_dev_err(sbl->dev, "bl %d: wrong state (%s) for config", port_num,
				sbl_link_state_str(link->blstate));
		spin_unlock(&link->lock);
		return -EUCLEAN;
	}

	memcpy(&link->blattr, blattr, sizeof(struct sbl_base_link_attr));

	link->blconfigured = true;

	if (link->blattr.loopback_mode == SBL_LOOPBACK_MODE_LOCAL) {
		/* we dont need media to go to down */
		link->blstate = SBL_BASE_LINK_STATUS_DOWN;
	} else if (link->mconfigured) {
		/* we need media as well if not loopback */
		link->blstate = SBL_BASE_LINK_STATUS_DOWN;
	}

	spin_unlock(&link->lock);

	return 0;
}
EXPORT_SYMBOL(sbl_base_link_config);


int sbl_base_link_start(struct sbl_inst *sbl, int port_num)
{
	int err;
	int tmp_err;
	u32 base = SBL_PML_BASE(port_num);
	u64 cfg_pcs_reg;
	u64 sts_pcs_lane_degrade_reg;
	struct lane_degrade lanes;
	u32 ucw_adj = 100;
	u32 ccw_adj = 0;
	s32 ucw_in = SBL_LINK_FEC_HPE;
	s32 ccw_in = SBL_LINK_FEC_HPE;
	struct sbl_link *link = sbl->link + port_num;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "bl %d: start\n", port_num);

	link = sbl->link + port_num;

	err = mutex_lock_interruptible(&link->busy_mtx);
	if (err)
		return -ERESTARTSYS;

	if (link->blstate == SBL_BASE_LINK_STATUS_UP) {
		/* link came up while waiting for the mutex */
		mutex_unlock(&link->busy_mtx);
		return 0;
	}

	if (link->blstate != SBL_BASE_LINK_STATUS_DOWN) {

		sbl_dev_err(sbl->dev, "bl %d: wrong state (%s) for start", port_num,
			sbl_link_state_str(link->blstate));
		mutex_unlock(&link->busy_mtx);
		return -EUCLEAN;
	}

	/*
	 * only local loopback is possible without media
	 */
	if (link->blattr.loopback_mode != SBL_LOOPBACK_MODE_LOCAL) {
		err = sbl_media_validate_config(sbl, port_num);
		if (err) {
			sbl_dev_err(sbl->dev, "bl %d: config unsuitable for media type", port_num);
			mutex_unlock(&link->busy_mtx);
			return err;
		}
	}

	link->blstate = SBL_BASE_LINK_STATUS_STARTING;

	/*
	 * check for loopback mode change
	 */
	if (link->blattr.loopback_mode != link->loopback_mode)
		link->llr_loop_time = 0;
	link->loopback_mode = link->blattr.loopback_mode;

	/*
	 * setup the timeout for start to complete
	 * (autoneg might modify this later)
	 */
	sbl_link_init_start_timeout(sbl, port_num);

	/*
	 * clear out any residual pcs state
	 */
	sbl_pml_set_defaults(sbl, port_num);

	/* reset state */
	link->lp_detected = false;

	/*
	 * validate serdes firmwares are (still) uncorrupted, recover them if
	 *  needed
	 *
	 */
	err = sbl_base_link_check_fix_fw(sbl, port_num);
	if (err) {
		sbl_base_link_report_err(sbl, "ensure_healthly", port_num, err);
		goto out;
	}

	/*
	 * determine the link mode
	 *
	 *   this may do autoneg for electrical links
	 */
	err = sbl_base_link_get_mode(sbl, port_num);
	if (err) {
		if (sbl_base_link_an_timed_out(sbl, port_num, err))
			sbl_dev_dbg(sbl->dev, "bl %d: autoneg timeout", port_num);
		else
			sbl_base_link_report_err(sbl, "get_mode", port_num, err);
		goto out;
	}

	/* start sending alignment markers for lp to tune against */
	err = sbl_pml_pcs_am_start(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "bl %d: am_start failed [%d]\n", port_num, err);
		goto out;
	}

	/*
	 * wait until we detect the link partner
	 */
	err = sbl_base_link_lp_detect(sbl, port_num);
	if (err) {
		sbl_base_link_report_err(sbl, "lpd", port_num, err);
		goto out;
	}

	/* record when we really start to bring up the link */
	sbl_link_up_begin(sbl, port_num);

	/*
	 * start the serdes
	 */
	err = sbl_serdes_start(sbl, port_num);
	if (err) {
		sbl_base_link_report_err(sbl, "serdes_start", port_num, err);
		goto out;
	}

	/*
	 * start the pml block (pcs,mac,llr)
	 */
	err = sbl_pml_start(sbl, port_num);
	if (err) {
		sbl_base_link_report_err(sbl, "pml_start", port_num, err);
		goto out_serdes;
	}

	/* clearing fec values before start */
	sbl_zero_all_fec_counts(sbl, port_num);

	/* setup thresholds and adjustments for fec monitoring */
	err = sbl_fec_thresholds_set(sbl, port_num, ucw_in, ccw_in);
	if (err) {
		sbl_dev_err(sbl->dev, "%d: setting fec thresholds failed [%d]",
				port_num, err);
		goto out_pcs;
	}

	err = sbl_fec_adjustments_set(sbl, port_num, ucw_adj, ccw_adj);
	if (err) {
		sbl_dev_err(sbl->dev, "%d: setting fec adjustments failed [%d]",
				port_num, err);
		goto out_pcs;
	}

	/*
	 * start fec checking
	 */
	err = sbl_fec_up_check(sbl, port_num);
	if (err) {
		sbl_serdes_invalidate_tuning_params(sbl, port_num);
		sbl_dev_info(sbl->dev, "%d: failed start fec check", err);
		goto out_pcs;
	}

	/*
	 * start monitoring for link faults
	 */
	err = sbl_link_fault_monitor_start(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "bl %d: link fault detect start failed [%d]\n", port_num, err);
		goto out_pcs;
	}

	/*
	 * final check the link is still up and all lanes remain active
	 */
	if (!sbl_pml_pcs_up(sbl, port_num)) {
		sbl_dev_err(sbl->dev, "bl %d: link failed during startup\n", port_num);
		err = -ENETDOWN;
		goto out_fm;
	}

	cfg_pcs_reg = sbl_read64(sbl, base | SBL_PML_CFG_PCS_OFFSET);
	if (SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_GET(cfg_pcs_reg)) {

		sts_pcs_lane_degrade_reg = sbl_read64(sbl, base | SBL_PML_STS_PCS_LANE_DEGRADE_OFFSET);
		lanes.tx = SBL_PML_STS_PCS_LANE_DEGRADE_LP_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg);
		lanes.rx = SBL_PML_STS_PCS_LANE_DEGRADE_RX_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg);

		if (lanes.tx != SBL_LINK_ALL_LANES || lanes.rx != SBL_LINK_ALL_LANES) {
			sbl_dev_err(sbl->dev, "bl %d: lane failed during startup - TX: 0x%llx - RX: 0x%llx",
				    port_num, lanes.tx, lanes.rx);
			err = -ENETDOWN;
			goto out_fm;
		}
	}

	/* success ! */
	sbl_link_up_record_timespec(sbl, port_num);
	sbl_link_start_record_timespec(sbl, port_num);

	link->blstate = SBL_BASE_LINK_STATUS_UP;
	link->blerr = 0;

	sbl_dev_dbg(sbl->dev, "starting timer port_num:%d ", port_num);
	mod_timer(&link->fec_data->fec_timer, jiffies + msecs_to_jiffies(SBL_FEC_MON_PERIOD));
	mutex_unlock(&link->busy_mtx);

	return 0;

	/*
	 * failed
	 */
out_fm:
	sbl_link_fault_monitor_stop(sbl, port_num);
out_pcs:
	if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_CLEANUP))
		sbl_pml_link_down(sbl, port_num);
out_serdes:
	if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_CLEANUP))
		sbl_serdes_stop(sbl, port_num);
out:
	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_CLEANUP) ||
			sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_RELOAD_FW))
		link->reload_serdes_fw = false;
	else if (sbl_debug_option(sbl, port_num, SBL_DEBUG_FORCE_RELOAD_FW))
		link->reload_serdes_fw = true;

	if (link->reload_serdes_fw) {
		sbl_dev_info(sbl->dev, "bl %d: reloading serdes fw\n", port_num);
		tmp_err = sbl_serdes_firmware_flash_safe(sbl, port_num, true);
		if (tmp_err) {
			/* All we can do is report failure here */
			sbl_dev_err(sbl->dev, "bl %d: fw flash failed [%d]\n",
					port_num, tmp_err);
			link->sstate = SBL_SERDES_STATUS_ERROR;
		} else
			link->sstate = SBL_SERDES_STATUS_DOWN;
		link->serr = tmp_err;
		link->reload_serdes_fw = false;
	}
	link->blstate = SBL_BASE_LINK_STATUS_ERROR;
	link->blerr = err;
	mutex_unlock(&link->busy_mtx);

	return err;
}
EXPORT_SYMBOL(sbl_base_link_start);


static void sbl_base_link_report_err(struct sbl_inst *sbl, char *txt, int port_num, int err)
{
	switch (err) {
	case -ECANCELED:
		sbl_dev_dbg(sbl->dev, "bl %d: %s cancelled\n", port_num, txt);
		break;
	case -ETIMEDOUT:
		sbl_dev_dbg(sbl->dev, "bl %d: %s timed out\n", port_num, txt);
		break;
	default:
		sbl_dev_err(sbl->dev, "bl %d: %s failed [%d]\n", port_num, txt, err);
	}
}

void sbl_ignore_save_tuning_param(struct sbl_inst *sbl, int port_num, bool ignore)
{
	struct sbl_link *link = sbl->link + port_num;

	if (ignore)
		link->blattr.options &= ~SBL_OPT_USE_SAVED_PARAMS;
	else
		link->blattr.options |= SBL_OPT_USE_SAVED_PARAMS;
}
EXPORT_SYMBOL(sbl_ignore_save_tuning_param);

void sbl_enable_opt_lane_degrade(struct sbl_inst *sbl, int port_num, bool enable)
{
	struct sbl_link *link = sbl->link + port_num;

	if (enable)
		link->blattr.options |= SBL_OPT_LANE_DEGRADE;
	else
		link->blattr.options &= ~SBL_OPT_LANE_DEGRADE;
}
EXPORT_SYMBOL(sbl_enable_opt_lane_degrade);


void sbl_disable_pml_recovery(struct sbl_inst *sbl, int port_num, bool disable)
{
	struct sbl_link *link = sbl->link + port_num;

	if (disable)
		link->blattr.options |= SBL_DISABLE_PML_RECOVERY;
	else
		link->blattr.options &= ~SBL_DISABLE_PML_RECOVERY;
}
EXPORT_SYMBOL(sbl_disable_pml_recovery);


void sbl_set_degraded_flag(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	spin_lock(&link->is_degraded_lock);
	link->is_degraded = true;
	spin_unlock(&link->is_degraded_lock);
}
EXPORT_SYMBOL(sbl_set_degraded_flag);


void sbl_clear_degraded_flag(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	spin_lock(&link->is_degraded_lock);
	link->is_degraded = false;
	spin_unlock(&link->is_degraded_lock);
}
EXPORT_SYMBOL(sbl_clear_degraded_flag);


bool sbl_get_degraded_flag(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	bool degraded_flag;

	spin_lock(&link->is_degraded_lock);
	degraded_flag = link->is_degraded;
	spin_unlock(&link->is_degraded_lock);

	return degraded_flag;
}
EXPORT_SYMBOL(sbl_get_degraded_flag);


int sbl_base_link_enable_start(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "bl %d: enable start\n", port_num);

	spin_lock(&link->lock);
	link->start_cancelled = false;
	spin_unlock(&link->lock);

	return 0;
}
EXPORT_SYMBOL(sbl_base_link_enable_start);


int sbl_base_link_cancel_start(struct sbl_inst *sbl, int port_num)
{
	int rtn;
	struct sbl_link *link = sbl->link + port_num;
	struct fec_data *fec_data = link->fec_data;

	sbl_dev_dbg(sbl->dev, "bl %d: cancelling start\n", port_num);

	spin_lock(&link->lock);
	link->start_cancelled = true;
	spin_unlock(&link->lock);
	rtn = del_timer_sync(&fec_data->fec_timer);

	if (rtn < 0)
		sbl_dev_warn(sbl->dev, "bl %d: del_timer_sync failed [%d]", port_num, rtn);

	return 0;
}
EXPORT_SYMBOL(sbl_base_link_cancel_start);


bool sbl_base_link_start_cancelled(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	bool cancelled;

	spin_lock(&link->lock);
	cancelled = link->start_cancelled;
	spin_unlock(&link->lock);

	return cancelled;
}


int sbl_base_link_stop(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;
	int rtn;
	struct fec_data *fec_data;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "bl %d: stop", port_num);

	link = sbl->link + port_num;
	fec_data = link->fec_data;

	err = mutex_lock_interruptible(&link->busy_mtx);
	if (err)
		return -ERESTARTSYS;

	if (link->blstate == SBL_BASE_LINK_STATUS_DOWN) {
		/* link went down while waiting for the mutex */
		mutex_unlock(&link->busy_mtx);
		return 0;
	}

	if (link->blstate != SBL_BASE_LINK_STATUS_UP) {
		sbl_dev_err(sbl->dev, "bl %d: not up (%s) for stop - ignored", port_num,
				sbl_link_state_str(link->blstate));
		mutex_unlock(&link->busy_mtx);
		return -EBUSY;
	}

	/* if keeping serdes up, don't change state to stopping */
	if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_KEEP_SERDES_UP))
		link->blstate = SBL_BASE_LINK_STATUS_STOPPING;

	/*
	 * We stop the serdes before stopping the pml to avoid breaking AOC firmware,
	 * really we should stop the pml first. This should be harmless provided
	 * down detection is off.
	 */
	err = sbl_link_fault_monitor_stop(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "bl %d: link_fault_monitor_stop failed [%d]", port_num, err);
		goto out;
	}

	if (link->pml_recovery.started)
		sbl_pml_recovery_cancel(sbl, port_num);

	err = sbl_serdes_stop(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "bl %d: serdes_stop failed [%d]", port_num, err);
		goto out;
	}

	err = sbl_pml_link_down(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "bl %d: pml_link_down failed [%d]", port_num, err);
		goto out;
	}

	rtn = del_timer_sync(&fec_data->fec_timer);

	if (rtn < 0)
		sbl_dev_warn(sbl->dev, "bl %d: del_timer_sync failed [%d]", port_num, rtn);
out:
	spin_lock(&link->lock);
	if (err)
		link->blstate = SBL_BASE_LINK_STATUS_ERROR;
	/* if keep serdes up, don't change state to down */
	else if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_KEEP_SERDES_UP))
		link->blstate = SBL_BASE_LINK_STATUS_DOWN;
	link->blerr = err;
	spin_unlock(&link->lock);

	mutex_unlock(&link->busy_mtx);

	sbl_dev_dbg(sbl->dev, "bl %d: stop done", port_num);

	return link->blerr;
}
EXPORT_SYMBOL(sbl_base_link_stop);


int sbl_base_link_reset(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;
	int rtn;
	struct fec_data *fec_data;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "bl %d: reset\n", port_num);

	link = sbl->link + port_num;
	fec_data = link->fec_data;

	err = mutex_lock_interruptible(&link->busy_mtx);
	if (err)
		return -ERESTARTSYS;

	link->blstate = SBL_BASE_LINK_STATUS_RESETTING;

	/* disable and remove any pml intr handlers */
	if (link->intr_err_flgs) {
		sbl_pml_disable_intr_handler(sbl, port_num, link->intr_err_flgs);
		sbl_pml_remove_intr_handler(sbl, port_num);
		link->intr_err_flgs = 0;
	}

	if (link->pml_recovery.started)
		sbl_pml_recovery_cancel(sbl, port_num);

	err = sbl_serdes_reset(sbl, port_num);
	if (err)
		sbl_dev_err(sbl->dev, "bl %d: reset: serdes_reset failed [%d]", port_num, err);

	err = sbl_pml_link_down(sbl, port_num);
	if (err)
		sbl_dev_warn(sbl->dev, "bl %d: reset: pcs_link_down failed [%d]", port_num, err);

	rtn = del_timer_sync(&fec_data->fec_timer);

	if (rtn < 0)
		sbl_dev_warn(sbl->dev, "bl %d: del_timer_sync failed [%d]", port_num, rtn);

	link->blstate = SBL_BASE_LINK_STATUS_UNCONFIGURED;
	link->blerr = 0;
	link->blconfigured = false;
	link->pcs_config = false;
	link->llr_loop_time = 0;
	link->start_cancelled = false;
	link->link_info = 0;
	link->lp_subtype = SBL_LP_SUBTYPE_INVALID;
	/* dont reset media attribute */

	/* reset hw */
	sbl_pml_set_defaults(sbl, port_num);

	sbl_pml_err_flgs_clear_all(sbl, port_num);

	mutex_unlock(&link->busy_mtx);

	return 0;
}
EXPORT_SYMBOL(sbl_base_link_reset);


/*
 * try to clear any error states that are non-fatal so we can directly
 * attempt another start up.
 */
void sbl_base_link_try_start_fail_cleanup(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return;

	sbl_dev_dbg(sbl->dev, "bl %d: try start fail cleanup\n", port_num);

	link = sbl->link + port_num;

	err = mutex_lock_interruptible(&link->busy_mtx);
	if (err)
		return;

	/* disable and remove any pml intr handlers */
	if (link->intr_err_flgs) {
		sbl_pml_disable_intr_handler(sbl, port_num, link->intr_err_flgs);
		sbl_pml_remove_intr_handler(sbl, port_num);
		link->intr_err_flgs = 0;
	}

	/*
	 * when debugging we can leave the hw in its current state
	 */
	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_CLEANUP)) {
		sbl_dev_warn(sbl->dev, "%d: hw cleanup inhibited\n", port_num);
		mutex_unlock(&link->busy_mtx);
		return;
	}

	/*
	 * clean up the serdes state
	 * We have to do this before resetting the PML or the optical transceivers can
	 * fail.
	 */
	switch (link->sstate) {
	case SBL_SERDES_STATUS_RUNNING:
		sbl_serdes_stop(sbl, port_num);
		break;
	case SBL_SERDES_STATUS_AUTONEG:
		sbl_an_serdes_stop(sbl, port_num);
		break;
	default:
		break;
	}

	/* still not down! */
	if (link->sstate != SBL_SERDES_STATUS_DOWN)
		sbl_serdes_reset(sbl, port_num);

	/*
	 * some errors we can clean up and move to down so
	 * we can try to come up again directly
	 */
	if (link->blstate == SBL_BASE_LINK_STATUS_ERROR) {
		switch (link->blerr) {

		case -EADV:          /* pcs startup saw high serdes error rate */
		case -ENETDOWN:      /* pcs dropped out after starting */
		case -ECHRNG:        /* serdes eye heights went bad during pcs startup */
		case -ELNRNG:        /* unable to tune properly */
			sbl_serdes_invalidate_all_tuning_params(sbl);
			fallthrough;
		case -ETIME:
		case -ETIMEDOUT:
		case -EOVERFLOW:
		case -ECANCELED:
		case -ENOSR:         /* llr failed to start */

			link->blstate = SBL_BASE_LINK_STATUS_DOWN;
			link->blerr = 0;
			link->pcs_config = false;
			link->llr_loop_time = 0;
			link->start_cancelled = false;

			sbl_pml_link_down(sbl, port_num);
			sbl_pml_set_defaults(sbl, port_num);
			break;

		default:
			sbl_dev_err(sbl->dev, "%d: start fail cleanup not clearing err %d\n",
					port_num, link->blerr);
			break;
		}
	}

	mutex_unlock(&link->busy_mtx);
}
EXPORT_SYMBOL(sbl_base_link_try_start_fail_cleanup);


static int sbl_link_fault_monitor_start(struct sbl_inst *sbl, int port_num)
{
	int err;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u64 err_flags;

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET);
	if (SBL_PML_CFG_LLR_SM_REPLAY_CT_MAX_GET(val64) < SBL_LLR_REPLAY_CT_MAX_UNLIMITED)
		err_flags = SBL_PML_FAULT_ERR_FLAGS;
	else
		err_flags = SBL_PML_REC_FAULT_ERR_FLAGS;

	/* make sure we have not already had an error */
	if (sbl_pml_err_flgs_test(sbl, port_num, err_flags)) {
		sbl_dev_err(sbl->dev, "fm %d: start - errors already present\n",
				port_num);
		err = -ENETDOWN;
		goto out;
	}

	err = sbl_pml_install_intr_handler(sbl, port_num, err_flags);
	if (err) {
		sbl_dev_err(sbl->dev, "fm %d: intr install failed [%d]\n", port_num, err);
		goto out;
	}

	err = sbl_pml_enable_intr_handler(sbl, port_num, err_flags);
	if (err) {
		sbl_dev_err(sbl->dev, "fm %d: intr enable failed [%d]\n", port_num, err);
		goto out_uninstall;
		return err;
	}

	if (sbl_pml_err_flgs_test(sbl, port_num, err_flags)) {
		sbl_dev_err(sbl->dev, "fm %d: link down during start\n", port_num);
		err = -ENETDOWN;
		goto out_disable;
	} else {
		/* all up and running */
		sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_FAULT_MON);
	}
	return 0;

out_disable:
	sbl_pml_disable_intr_handler(sbl, port_num, err_flags);
out_uninstall:
	sbl_pml_remove_intr_handler(sbl, port_num);
out:
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_FAULT_MON);

	return err;
}


static int sbl_link_fault_monitor_stop(struct sbl_inst *sbl, int port_num)
{
	int err;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u64 err_flags;

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET);
	if (SBL_PML_CFG_LLR_SM_REPLAY_CT_MAX_GET(val64) < SBL_LLR_REPLAY_CT_MAX_UNLIMITED)
		err_flags = SBL_PML_FAULT_ERR_FLAGS;
	else
		err_flags = SBL_PML_REC_FAULT_ERR_FLAGS;

	err = sbl_pml_disable_intr_handler(sbl, port_num, err_flags);
	if (err) {
		sbl_dev_err(sbl->dev, "fm %d: intr disable failed [%d]\n", port_num, err);
		return err;
	}

	err = sbl_pml_remove_intr_handler(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "fm %d: intr remove failed [%d]\n", port_num, err);
		return err;
	}

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_FAULT_MON);
	return 0;
}


int sbl_base_link_get_status(struct sbl_inst *sbl, int port_num,
		int *blstate, int *blerr, int *sstate, int *serr,
		int *media_type, int *link_mode)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	if (blstate)
		*blstate = link->blstate;
	if (blerr)
		*blerr = link->blerr;
	if (sstate)
		*sstate = link->sstate;
	if (serr)
		*serr = link->serr;
	if (media_type)
		*media_type = link->mattr.media;

	if (link_mode) {
		if (link->sstate == SBL_SERDES_STATUS_RUNNING)
			*link_mode = link->link_mode;            /* actual mode */
		else
			*link_mode = link->blattr.link_mode;     /* target mode */
	}

	return 0;
}
EXPORT_SYMBOL(sbl_base_link_get_status);


/*
 * update a string with the state of the pcs and serdes
 */
char *sbl_base_link_state_str(struct sbl_inst *sbl, int port_num, char *buf, int len)
{
	char pcs_buf[SBL_PCS_STATE_STR_LEN];
	u8 not_idle_map;  // not signalling electrical idle
	u8 good_eyes_map;
	u8 active_map;

	if (!buf || !len)
		return "";

	sbl_pml_pcs_state_str(sbl, port_num, pcs_buf, SBL_PCS_STATE_STR_LEN);

	if (sbl_port_get_serdes_state_maps(sbl, port_num, &active_map,
			&not_idle_map, &good_eyes_map)) {
		/* only active map unavailable */
		snprintf(buf, len, "pcs %s, serdes -,-/%x", pcs_buf, active_map);
	} else
		snprintf(buf, len, "pcs %s, serdes %x,%x/%x", pcs_buf,
				not_idle_map, good_eyes_map, active_map);

	return buf;
}
EXPORT_SYMBOL(sbl_base_link_state_str);


/*
 * detect the presence of our link partner
 */
static int sbl_base_link_lp_detect(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int err = 0;

	/* already detected - probably from autoneg */
	if (link->lp_detected)
		return 0;

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LP_DET);

	/* currently we have only one way to do this */
	if (link->blattr.options & SBL_OPT_SERDES_LPD) {
		err =  sbl_serdes_lp_detect(sbl, port_num);
		if (!err)
			link->lp_detected = true;
	}

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LP_DET);

	return err;
}

/*
 * Check/recover SBL FW
 *
 */
static int sbl_base_link_check_fix_fw(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int serdes;
	int err;

	for (serdes = 0; serdes < sbl->switch_info->num_serdes; ++serdes) {
		err = sbl_validate_serdes_fw_crc(sbl, port_num, serdes);
		if (err) {
			/* Any lane with corrputed FW will cause all lanes for
			 * the port to be reloaded.
			 */
			goto reload_fw;
		}
	}
	return 0;

 reload_fw:
	err = sbl_serdes_firmware_flash_safe(sbl, port_num, false);
	if (err) {
		/* All we can do is report failure here */
		sbl_dev_err(sbl->dev, "%d: check/fix: fw flash failed [%d]\n",
			port_num, err);
		link->sstate = SBL_SERDES_STATUS_ERROR;
	}
	return err;
}

/*
 * determine the link mode (speed)
 *
 */
static int sbl_base_link_get_mode(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int err = 0;

	/* determine required mode */
	if (link->loopback_mode == SBL_LOOPBACK_MODE_LOCAL) {
		/* directly use the mode specified - no media check */
		link->link_mode = link->blattr.link_mode;
		return 0;
	}

	switch (link->mattr.media) {

	case SBL_LINK_MEDIA_ELECTRICAL:
		err = sbl_link_get_mode_electrical(sbl, port_num);
		break;

	case SBL_LINK_MEDIA_OPTICAL:
		err = sbl_link_get_mode_optical(sbl, port_num);
		break;

	default:
		sbl_dev_err(sbl->dev, "bl %d: bad media to determine mode", port_num);
		err = -ENOMEDIUM;
		break;
	}
	if (err)
		return err;

	/* ensure the media supports the required mode */
	if (!sbl_media_check_mode_supported(sbl, port_num, link->link_mode)) {
		sbl_dev_err(sbl->dev, "bl %d: config mode (%s) not supported by media",
			port_num, sbl_link_mode_str(link->blattr.link_mode));
		return -EMEDIUMTYPE;
	} else
		return 0;
}


static int sbl_link_get_mode_electrical(struct sbl_inst *sbl, int port_num)
{
	sbl_dev_dbg(sbl->dev, "bl %d: elec get mode", port_num);

	return sbl_link_autoneg(sbl, port_num);
}


/*
 * For now we will just use the configured speed
 *
 */
static int sbl_link_get_mode_optical(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "bl %d: optical link - fixing speed to config value (%s)\n",
			port_num, sbl_link_mode_str(link->blattr.link_mode));
	link->link_mode = link->blattr.link_mode;

	return 0;
}


/*
 * return true if get_mode failed because autoneg timed out
 */
static bool sbl_base_link_an_timed_out(struct sbl_inst *sbl, int port_num, int err)
{
	struct sbl_link *link = sbl->link + port_num;

	if (err != -ETIME)
		return false;

	if (link->mattr.media != SBL_LINK_MEDIA_ELECTRICAL)
		return false;

	if (link->blattr.pec.an_mode == SBL_AN_MODE_OFF)
		return false;

	return true;
}


/*
 * Return the failure and warning thresholds for fec metrics
 * monitoring
 *
 * The ccw values used here corresponds to a BER of 1.e-4
 * The ucw values correspond to a little less than 1e-10
 * The single lane fec warning threshold is the ccw rate divided by
 * the number of fec lanes in use for the mode.
 *
 *
 * 200G LINK (BS) (CD)
 *   212500000000 bits/second over a 4-lane link @ 200G
 *   39062500 cw/s (5440 bits/codeword)
 *
 *   (Post)Pre-FEC BER
 *  21250000.000000 (un)corrected cw/s => 1e-04 BER (1 bad bit per codeword)
 *   2125000.000000 (un)corrected cw/s => 1e-05 BER
 *    212500.000000 (un)corrected cw/s => 1e-06 BER
 *     21250.000000 (un)corrected cw/s => 1e-07 BER
 *      2125.000000 (un)corrected cw/s => 1e-08 BER
 *       212.500000 (un)corrected cw/s => 1e-09 BER
 *        21.250000 (un)corrected cw/s => 1e-10 BER
 *         2.125000 (un)corrected cw/s => 1e-11 BER
 *         0.212500 (un)corrected cw/s => 1e-12 BER
 *         0.021250 (un)corrected cw/s => 1e-13 BER
 *         0.002125 (un)corrected cw/s => 1e-13 BER
 *
 * 100G LINK (BJ)
 *   103125000000 bits/second over a 4-lane link @ 100G
 *   19531250 cw/s (5280 bits/codeword)
 *
 *   (Post) Pre-FEC BER
 *   1031250.000000 (un)corrected cw/s => 1e-05 BER (1 bad bit per codeword)
 *    103125.000000 (un)corrected cw/s => 1e-06 BER
 *     10312.500000 (un)corrected cw/s => 1e-07 BER
 *      1031.250000 (un)corrected cw/s => 1e-08 BER
 *       103.125000 (un)corrected cw/s => 1e-09 BER
 *        10.312500 (un)corrected cw/s => 1e-10 BER
 *         1.031250 (un)corrected cw/s => 1e-11 BER
 *         0.103125 (un)corrected cw/s => 1e-12 BER
 *
 * Cassini uses the old
 *    sbl_link_get_fec_thresholds()
 * Rosetta uses the newer
 *    sbl_link_get_ucw_thresh_ieee()
 *    sbl_link_get_ucw_thresh_hpe()
 *    sbl_link_get_ccw_thresh_ieee()
 *    sbl_link_get_ccw_thresh_hpe()
 */
#ifndef CONFIG_SBL_PLATFORM_ROS_HW
int sbl_link_get_fec_thresholds(struct sbl_inst *sbl, int port_num,
		int *ucw_bad, int *ccw_bad, int *fecl_warn)
{
	struct sbl_link *link = sbl->link + port_num;
	int ucw;
	int ccw;
	int warn;

	if (link->sstate != SBL_SERDES_STATUS_RUNNING)
		return -ENAVAIL;

	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		ucw = 21;
		ccw = 21250000;
		warn = ccw/8;
		break;
	case SBL_LINK_MODE_BJ_100G:
		ucw = 11;
		ccw = 10312500;
		warn = ccw/8;
		break;
	case SBL_LINK_MODE_CD_100G:
		ucw = 11;
		ccw = 10625000;
		warn = ccw/4;
		break;
	case SBL_LINK_MODE_CD_50G:
		ucw = 5;
		ccw = 5312500;
		warn = ccw/2;
		break;
	default:
		sbl_dev_err(sbl->dev, "%d: bad mode for fec thresh", port_num);
		return -ENAVAIL;
	}

	if (ucw_bad)
		*ucw_bad = ucw;
	if (ccw_bad)
		*ccw_bad = ccw;
	if (fecl_warn)
		*fecl_warn = warn;

	return 0;
}
EXPORT_SYMBOL(sbl_link_get_fec_thresholds);
#endif

/*
 * uncorrected codeword thresholds
 */
u64  sbl_link_get_ucw_thresh_hpe(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	/* return a little under 1e-10 for all modes and cable types */
	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		return 21;
	case SBL_LINK_MODE_BJ_100G:
	case SBL_LINK_MODE_CD_100G:
		return 11;
	case SBL_LINK_MODE_CD_50G:
		return 5;
	default:
		sbl_dev_err(sbl->dev, "%d: cannot determine ucw rate", port_num);
		return 0;
	}
}
EXPORT_SYMBOL(sbl_link_get_ucw_thresh_hpe);


u64  sbl_link_get_ucw_thresh_ieee(struct sbl_inst *sbl, int port_num)
{
	/* currently these are the same as the hpe values */
	return sbl_link_get_ucw_thresh_hpe(sbl, port_num);
}
EXPORT_SYMBOL(sbl_link_get_ucw_thresh_ieee);


/*
 * corrected code words
 */
u64  sbl_link_get_ccw_thresh_hpe(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	/*
	 * for electrical links use a threshold of 1.e-5
	 * (we consider loopback links to be electrical)
	 */
	if ((link->mattr.media == SBL_LINK_MEDIA_ELECTRICAL) ||
			(link->loopback_mode == SBL_LOOPBACK_MODE_LOCAL)) {
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			return 21250000;
		case SBL_LINK_MODE_BJ_100G:
			return 10312500;
		case SBL_LINK_MODE_CD_100G:
			return 10625000;
		case SBL_LINK_MODE_CD_50G:
			return 5312500;
		default:
			sbl_dev_err(sbl->dev, "%d: cannot determine PEC ccw rate", port_num);
			return 0;
		}

	}

	/*
	 * optical links seem to require a threshold of about 4e-5
	 * (otherwise pcal can sometimes take us over the threshold)
	 */
	if (link->mattr.media == SBL_LINK_MEDIA_OPTICAL) {
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			return 85000000;
		case SBL_LINK_MODE_BJ_100G:
			return 41250000;
		case SBL_LINK_MODE_CD_100G:
			return 42500000;
		case SBL_LINK_MODE_CD_50G:
			return 21250000;
		default:
			sbl_dev_err(sbl->dev, "%d: cannot determine AOC ccw rate", port_num);
			return 0;
		}
	}

	/* dont recognise media */
	sbl_dev_err(sbl->dev, "%d: cannot determine ccw rate - unrecognised media", port_num);
	return 0;
}
EXPORT_SYMBOL(sbl_link_get_ccw_thresh_hpe);


u64  sbl_link_get_ccw_thresh_ieee(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	/*
	 * ieee thresholds are about 2e-4
	 * We dont care about media type
	 */
	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		return 42500000;
	case SBL_LINK_MODE_BJ_100G:
	case SBL_LINK_MODE_CD_100G:
		return 21250000;
	case SBL_LINK_MODE_CD_50G:
		return 10625000;
	default:
		sbl_dev_err(sbl->dev, "%d: cannot determine ccw rate", port_num);
		return 0;
	}
}
EXPORT_SYMBOL(sbl_link_get_ccw_thresh_ieee);

/*
 * corrected code words when STP is used
 * It is meant to be aggressive and is of the order 1e-08
 */
u64  sbl_link_get_stp_ccw_thresh_hpe(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	/*
	 * for electrical links use a threshold of 1.e-8
	 * (we consider loopback links to be electrical)
	 */
	if ((link->mattr.media == SBL_LINK_MEDIA_ELECTRICAL) ||
			(link->loopback_mode == SBL_LOOPBACK_MODE_LOCAL)) {
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			return 2125;
		case SBL_LINK_MODE_BJ_100G:
			return 1031;
		case SBL_LINK_MODE_CD_100G:
			return 1062;
		case SBL_LINK_MODE_CD_50G:
			return 531;
		default:
			sbl_dev_err(sbl->dev, "%d: cannot determine PEC stp ccw rate", port_num);
			return 0;
		}

	}

	/*
	 * optical links seem to require a threshold of about 4e-8
	 * (otherwise pcal can sometimes take us over the threshold)
	 */
	if (link->mattr.media == SBL_LINK_MEDIA_OPTICAL) {
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			return 8500;
		case SBL_LINK_MODE_BJ_100G:
			return 4125;
		case SBL_LINK_MODE_CD_100G:
			return 4250;
		case SBL_LINK_MODE_CD_50G:
			return 2125;
		default:
			sbl_dev_err(sbl->dev, "%d: cannot determine AOC stp ccw rate", port_num);
			return 0;
		}
	}

	/* dont recognise media */
	sbl_dev_err(sbl->dev, "%d: cannot determine stp ccw rate - unrecognised media", port_num);
	return 0;
}
EXPORT_SYMBOL(sbl_link_get_stp_ccw_thresh_hpe);


u64  sbl_link_get_stp_ccw_thresh_ieee(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	/*
	 * ieee thresholds are about 2e-8
	 * We dont care about media type
	 */
	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		return 4250;
	case SBL_LINK_MODE_BJ_100G:
	case SBL_LINK_MODE_CD_100G:
		return 2125;
	case SBL_LINK_MODE_CD_50G:
		return 1062;
	default:
		sbl_dev_err(sbl->dev, "%d: cannot determine stp ccw rate", port_num);
		return 0;
	}
}
EXPORT_SYMBOL(sbl_link_get_stp_ccw_thresh_ieee);

/*
 * Link info
 *    lets you see the current state of the link
 *
 * No locking here
 */
void sbl_link_info_set(struct sbl_inst *sbl, int port_num, u32 flag)
{
	switch (flag) {

	case SBL_LINK_INFO_LP_DET:
	case SBL_LINK_INFO_PCS_ANEG:
	case SBL_LINK_INFO_PCS_TX_AM:
	case SBL_LINK_INFO_PCS_TX_RF:
	case SBL_LINK_INFO_PCS_ALIGN_EN:
	case SBL_LINK_INFO_PCS_A_WAIT:
	case SBL_LINK_INFO_PCS_F_WAIT:
	case SBL_LINK_INFO_PCS_UP:
	case SBL_LINK_INFO_MAC_OP:
	case SBL_LINK_INFO_OS:
	case SBL_LINK_INFO_LLR_LOOP:
	case SBL_LINK_INFO_LLR_MEASURE:
	case SBL_LINK_INFO_LLR_WAIT:
	case SBL_LINK_INFO_LLR_RUN:
	case SBL_LINK_INFO_LLR_DISABLED:
	case SBL_LINK_INFO_FAULT_MON:
	case SBL_LINK_INFO_LLR_DETECT:
		sbl->link[port_num].link_info |= flag;
		break;

	default:
		sbl_dev_dbg(sbl->dev, "bl %d: unrecognised set info flag (%d)\n",
				port_num, flag);
		break;
	}
}


void sbl_link_info_clear(struct sbl_inst *sbl, int port_num, u32 flag)
{
	switch (flag) {

	case SBL_LINK_INFO_LP_DET:
	case SBL_LINK_INFO_PCS_ANEG:
	case SBL_LINK_INFO_PCS_TX_AM:
	case SBL_LINK_INFO_PCS_TX_RF:
	case SBL_LINK_INFO_PCS_ALIGN_EN:
	case SBL_LINK_INFO_PCS_A_WAIT:
	case SBL_LINK_INFO_PCS_F_WAIT:
	case SBL_LINK_INFO_PCS_UP:
	case SBL_LINK_INFO_MAC_OP:
	case SBL_LINK_INFO_OS:
	case SBL_LINK_INFO_LLR_LOOP:
	case SBL_LINK_INFO_LLR_MEASURE:
	case SBL_LINK_INFO_LLR_WAIT:
	case SBL_LINK_INFO_LLR_RUN:
	case SBL_LINK_INFO_LLR_DISABLED:
	case SBL_LINK_INFO_FAULT_MON:
	case SBL_LINK_INFO_LLR_DETECT:
		sbl->link[port_num].link_info &= ~flag;
		break;

	default:
		sbl_dev_dbg(sbl->dev, "bl %d: unrecognised clear info flag (%d)\n",
				port_num, flag);
		break;
	}
}


/*
 * Print out link state, info etc for sysfs diags
 *
 *   No locking here
 */
#ifdef CONFIG_SYSFS
int sbl_base_link_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	bool mac_tx_op;
	bool mac_rx_op;
	u32 mac_ifg_mode;
	u64 llr_cfg;
	u32 llr_mode;
	u32 llr_down_behavior;
	u64 llr_sts;
	int s = 0;

	spin_lock(&link->lock);

	s += snprintf(buf+s, size-s, "base link state: %s",
			sbl_link_state_str(link->blstate));
	if (link->blstate == SBL_BASE_LINK_STATUS_ERROR)
		s += snprintf(buf+s, size-s, " [%d]", link->blerr);
	if (link->blstate == SBL_BASE_LINK_STATUS_STARTING)
		s += snprintf(buf+s, size-s, " (%d/%d)",
				sbl_link_start_elapsed(sbl, port_num),
				sbl_get_start_timeout(sbl, port_num));
	if (link->blstate == SBL_BASE_LINK_STATUS_UP)
		s += snprintf(buf+s, size-s, " (%lld.%.3ld, %lld.%.3ld)",
				(long long)link->start_time.tv_sec,
				link->start_time.tv_nsec/1000000,
				(long long)link->up_time.tv_sec,
				link->up_time.tv_nsec/1000000);
	s += snprintf(buf+s, size-s, "\n");

	if (link->link_info) {
		s += snprintf(buf+s, size-s, "base link info: ");
		if (link->link_info & SBL_LINK_INFO_PCS_ANEG)
			s += snprintf(buf+s, size-s, "pcs-aneg ");
		if (link->link_info & SBL_LINK_INFO_PCS_TX_AM)
			s += snprintf(buf+s, size-s, "pcs-tx-am ");
		if (link->link_info & SBL_LINK_INFO_PCS_ALIGN_EN)
			s += snprintf(buf+s, size-s, "pcs-a-en ");
		if (link->link_info & SBL_LINK_INFO_PCS_A_WAIT)
			s += snprintf(buf+s, size-s, "pcs-a-wait ");
		if (link->link_info & SBL_LINK_INFO_PCS_TX_RF)
			s += snprintf(buf+s, size-s, "pcs-tx-rf ");
		if (link->link_info & SBL_LINK_INFO_PCS_F_WAIT)
			s += snprintf(buf+s, size-s, "pcs-f-wait ");
		if (link->link_info & SBL_LINK_INFO_PCS_UP)
			s += snprintf(buf+s, size-s, "pcs-up ");
		if (link->link_info & SBL_LINK_INFO_LP_DET)
			s += snprintf(buf+s, size-s, "lp-det ");
		if (link->link_info & SBL_LINK_INFO_MAC_OP)
			s += snprintf(buf+s, size-s, "mac ");
		if (link->link_info & SBL_LINK_INFO_OS)
			s += snprintf(buf+s, size-s, "os ");
		if (link->link_info & SBL_LINK_INFO_LLR_LOOP)
			s += snprintf(buf+s, size-s, "llr-loop-en ");
		if (link->link_info & SBL_LINK_INFO_LLR_DETECT)
			s += snprintf(buf+s, size-s, "llr-detect ");
		if (link->link_info & SBL_LINK_INFO_LLR_MEASURE)
			s += snprintf(buf+s, size-s, "llr-measure ");
		if (link->link_info & SBL_LINK_INFO_LLR_WAIT)
			s += snprintf(buf+s, size-s, "llr-wait ");
		if (link->link_info & SBL_LINK_INFO_LLR_RUN)
			s += snprintf(buf+s, size-s, "llr ");
		if (link->link_info & SBL_LINK_INFO_LLR_DISABLED)
			s += snprintf(buf+s, size-s, "llr-dis ");
		if (link->link_info & SBL_LINK_INFO_FAULT_MON)
			s += snprintf(buf+s, size-s, "fm ");
		s += snprintf(buf+s, size-s, "\n");
	}

	if (link->blconfigured) {

		if (link->blattr.config_target == SBL_BASE_LINK_CONFIG_PEC) {
			s += snprintf(buf+s, size-s, "base link: an: mode %s",
					sbl_an_mode_str(link->blattr.pec.an_mode));
			if (link->link_info & SBL_LINK_INFO_PCS_ANEG) {
				if (link->blattr.pec.an_mode == SBL_AN_MODE_FIXED)
					s += snprintf(buf+s, size-s, "(%s)",
							sbl_link_mode_str(link->blattr.link_mode));
				s += snprintf(buf+s, size-s, ", tries %d, nonce %x, state %s",
						link->an_try_count, link->an_nonce,
						sbl_an_get_sm_state(sbl, port_num));
			}
			if ((link->blattr.pec.an_mode != SBL_AN_MODE_OFF) && (link->an_rx_count))
				s += snprintf(buf+s, size-s, ", received %d", link->an_rx_count);
			if (link->an_100cr4_fixup_applied)
				s += snprintf(buf+s, size-s, ", 100cr4-fixup");
			if (link->an_options & AN_OPT_LLR)
				s += snprintf(buf+s, size-s, ", llr");

			s += snprintf(buf+s, size-s, "\n");
		}
		sbl_pml_mac_hw_status(sbl, port_num, &mac_tx_op, &mac_rx_op, &mac_ifg_mode, NULL);
		if (mac_tx_op || mac_rx_op) {
			s += snprintf(buf+s, size-s, "base link: mac: tx %d, rx %d, ifg %s\n",
					mac_tx_op, mac_rx_op, sbl_ifg_config_str(link->ifg_config));
		}

		llr_cfg = sbl_read64(sbl, SBL_PML_BASE(port_num)|SBL_PML_CFG_LLR_OFFSET);
		switch (SBL_PML_CFG_LLR_LLR_MODE_GET(llr_cfg)) {
		case 0:
			llr_mode = SBL_LLR_MODE_OFF;
			break;
		case 1:
			llr_mode = SBL_LLR_MODE_MONITOR;
			break;
		case 2:
			llr_mode = SBL_LLR_MODE_ON;
			break;
		default:
			llr_mode = SBL_LLR_MODE_INVALID;
		}
		switch (SBL_PML_CFG_LLR_LINK_DOWN_BEHAVIOR_GET(llr_cfg)) {
		case 0:
			llr_down_behavior = SBL_LLR_LINK_DOWN_DISCARD;
			break;
		case 1:
			llr_down_behavior = SBL_LLR_LINK_DOWN_BLOCK;
			break;
		case 2:
			llr_down_behavior = SBL_LLR_LINK_DOWN_BEST_EFFORT;
			break;
		default:
			llr_down_behavior = SBL_LLR_LINK_DOWN_INVALID;
		}
		llr_sts = sbl_read64(sbl, SBL_PML_BASE(port_num)|SBL_PML_STS_LLR_MAX_USAGE_OFFSET);
		s += snprintf(buf+s, size-s, "base link: llr: mode %s, down %s, loop %lld %s %s\n",
				sbl_llr_mode_str(llr_mode),
				sbl_llr_down_behaviour_str(llr_down_behavior),
				link->llr_loop_time,
				SBL_PML_STS_LLR_MAX_USAGE_BUFF_SPC_STARVED_GET(llr_sts) ? "buf starved" : "",
				SBL_PML_STS_LLR_MAX_USAGE_SEQ_STARVED_GET(llr_sts) ? "seq starved" : "");

		if (link->blattr.options & SBL_DISABLE_PML_RECOVERY)
			s += snprintf(buf+s, size-s, "base link: pml rec: disabled\n");
		else
			s += snprintf(buf+s, size-s, "base link: pml rec: enabled, to %u, rl %u/%u\n",
				link->blattr.pml_recovery.timeout,
				link->blattr.pml_recovery.rl_max_duration,
				link->blattr.pml_recovery.rl_window_size);
	}

	spin_unlock(&link->lock);

	s += sbl_debug_sysfs_sprint(sbl, port_num, buf+s, size-s);

	return s;
}
EXPORT_SYMBOL(sbl_base_link_sysfs_sprint);

#ifndef CONFIG_SBL_PLATFORM_ROS_HW
int sbl_base_link_llr_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);

	if (link->link_info & SBL_LINK_INFO_LLR_RUN)
		s += snprintf(buf+s, size-s, "on");
	else
		s += snprintf(buf+s, size-s, "off");

	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_base_link_llr_sysfs_sprint);

int sbl_base_link_loopback_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);

	s += snprintf(buf+s, size-s, sbl_loopback_mode_str(link->loopback_mode));

	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_base_link_loopback_sysfs_sprint);
#endif
#endif

int sbl_base_link_dump_attr(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	struct sbl_base_link_attr *attr = &link->blattr;
	int s = 0;

	s += snprintf(buf+s, size-s, "%d: base link attributes:\n", port_num);

	if (!link->blconfigured) {
		s += snprintf(buf+s, size-s, "not configured\n");
		return s;
	}

	s += snprintf(buf+s, size-s, "options 0x%x", attr->options);
	if (attr->options) {
		s += snprintf(buf+s, size-s, " -");
		if (attr->options & SBL_OPT_FABRIC_LINK)
			s += snprintf(buf+s, size-s, " fabric");
		if (attr->options & SBL_OPT_SERDES_LPD)
			s += snprintf(buf+s, size-s, " serdes-lpd");
		if (attr->options & SBL_OPT_DFE_SAVE_PARAMS)
			s += snprintf(buf+s, size-s, " save-params");
		if (attr->options & SBL_OPT_USE_SAVED_PARAMS)
			s += snprintf(buf+s, size-s, " use-params");
		if (attr->options & SBL_OPT_RESET_CLEAR_PARAMS)
			s += snprintf(buf+s, size-s, " reset-clear-params");
		if (attr->options & SBL_OPT_ENABLE_PCAL)
			s += snprintf(buf+s, size-s, " pcal");
		if (attr->options & SBL_OPT_DFE_ALWAYS_MAX_EFFORT)
			s += snprintf(buf+s, size-s, " max-effort");
		if (attr->options & SBL_OPT_DFE_ALWAYS_MED_EFFORT)
			s += snprintf(buf+s, size-s, " med-effort");
		if (attr->options & SBL_OPT_DFE_ALWAYS_MIN_EFFORT)
			s += snprintf(buf+s, size-s, " min-effort");
		if (attr->options & SBL_OPT_AUTONEG_TIMEOUT_IEEE)
			s += snprintf(buf+s, size-s, " an-timeout-ieee");
		if (attr->options & SBL_OPT_AUTONEG_TIMEOUT_SSHOT)
			s += snprintf(buf+s, size-s, " an-timeout-sshot");
		if (attr->options & SBL_OPT_AUTONEG_100CR4_FIXUP)
			s += snprintf(buf+s, size-s, " an-100cr4-fixup");
		if (attr->options & SBL_OPT_RELOAD_FW_ON_TIMEOUT)
			s += snprintf(buf+s, size-s, " timeout-reload-fw");
		if (attr->options & SBL_OPT_ALLOW_MEDIA_BAD_MODE)
			s += snprintf(buf+s, size-s, " allow-media-bad-mode");
		if (attr->options & SBL_OPT_ALLOW_MEDIA_BAD_LEN)
			s += snprintf(buf+s, size-s, " allow-media-bad-len");
		if (attr->options & SBL_OPT_ENABLE_ETHER_LLR)
			s += snprintf(buf+s, size-s, " enable-ether-llr");
		if (attr->options & SBL_OPT_ENABLE_IFG_HPC_WITH_LLR)
			s += snprintf(buf+s, size-s, " enable-ifg-hpc-with-llr");
		if (attr->options & SBL_OPT_ENABLE_IFG_CONFIG)
			s += snprintf(buf+s, size-s, " enable-ifg-config");
		if (attr->options & SBL_OPT_DISABLE_AN_LLR)
			s += snprintf(buf+s, size-s, " disable-an-llr");
		if (attr->options & SBL_OPT_LANE_DEGRADE)
			s += snprintf(buf+s, size-s, " enable-lane-degrade");
		if (attr->options & SBL_DISABLE_PML_RECOVERY)
			s += snprintf(buf+s, size-s, " disable-pml-recovery");
	}
	s += snprintf(buf+s, size-s, "\n");
	s += snprintf(buf+s, size-s, "start_timeout %d\n", attr->start_timeout);
	if (attr->config_target == SBL_BASE_LINK_CONFIG_PEC) {
		s += snprintf(buf+s, size-s, "config_target PEC\n");
		s += snprintf(buf+s, size-s, "an_mode %d (%s)\n", attr->pec.an_mode, sbl_an_mode_str(attr->pec.an_mode));
		s += snprintf(buf+s, size-s, "an_retry_timeout %d\n", attr->pec.an_retry_timeout);
		s += snprintf(buf+s, size-s, "an_max_retry %d\n", attr->pec.an_max_retry);
	}
	if (attr->config_target == SBL_BASE_LINK_CONFIG_AOC) {
		s += snprintf(buf+s, size-s, "config_target AOC\n");
		s += snprintf(buf+s, size-s, "optical_lock_delay %d\n", attr->aoc.optical_lock_delay);
		s += snprintf(buf+s, size-s, "optical_lock_interval %d\n", attr->aoc.optical_lock_interval);
	}
	s += snprintf(buf+s, size-s, "lpd_timeout %d\n", attr->lpd_timeout);
	s += snprintf(buf+s, size-s, "lpd_poll_interval %d\n", attr->lpd_poll_interval);
	s += snprintf(buf+s, size-s, "link_mode %d (%s)\n", attr->link_mode, sbl_link_mode_str(attr->link_mode));
	s += snprintf(buf+s, size-s, "loopback_mode %d (%s)\n", attr->loopback_mode, sbl_loopback_mode_str(attr->loopback_mode));
	s += snprintf(buf+s, size-s, "link_partner %d\n", attr->link_partner);
	s += snprintf(buf+s, size-s, "tuning_pattern %d\n", attr->tuning_pattern);
	s += snprintf(buf+s, size-s, "precoding %d\n", attr->precoding);
	s += snprintf(buf+s, size-s, "dfe_pre_delay %d\n", attr->dfe_pre_delay);
	s += snprintf(buf+s, size-s, "dfe_timeout %d\n", attr->dfe_timeout);
	s += snprintf(buf+s, size-s, "dfe_poll_interval %d\n", attr->dfe_poll_interval);
	s += snprintf(buf+s, size-s, "pcal_eyecheck_holdoff %d\n", attr->pcal_eyecheck_holdoff);
	s += snprintf(buf+s, size-s, "nrz_min_eye_height 0x%x\n", attr->nrz_min_eye_height);
	s += snprintf(buf+s, size-s, "nrz_max_eye_height 0x%x\n", attr->nrz_max_eye_height);
	s += snprintf(buf+s, size-s, "pam4_min_eye_height 0x%x\n", attr->pam4_min_eye_height);
	s += snprintf(buf+s, size-s, "pam4_max_eye_height 0x%x\n", attr->pam4_max_eye_height);
	s += snprintf(buf+s, size-s, "fec_mode %d\n", attr->fec_mode);
	s += snprintf(buf+s, size-s, "enable_autodegrade %d\n", attr->enable_autodegrade);
	s += snprintf(buf+s, size-s, "llr_mode %d (%s)\n", attr->llr_mode, sbl_llr_mode_str(attr->llr_mode));
	s += snprintf(buf+s, size-s, "ifg_config %d (%s)\n", attr->ifg_config, sbl_ifg_config_str(attr->ifg_config));
	s += snprintf(buf+s, size-s, "pml_recovery.timeout %u\n", attr->pml_recovery.timeout);
	s += snprintf(buf+s, size-s, "pml_recovery.rl_max_duration %u\n", attr->pml_recovery.rl_max_duration);
	s += snprintf(buf+s, size-s, "pml_recovery.rl_window_size %u\n", attr->pml_recovery.rl_window_size);

	return s;
}
EXPORT_SYMBOL(sbl_base_link_dump_attr);



