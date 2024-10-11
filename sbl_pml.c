// SPDX-License-Identifier: GPL-2.0

/*
 * sbl_pml.c
 *
 * Copyright 2019-2023 Hewlett Packard Enterprise Development LP
 *
 * core PML block functions
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <sbl/sbl_pml.h>

#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_internal.h"
#include "sbl_serdes_fn.h"

/*
 * Bring-up PML block of a link
 */
int sbl_pml_start(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int err;

	sbl_dev_dbg(sbl->dev, "%d: pml bring-up starting", port_num);

	/* serdes must be up */
	if (link->sstate != SBL_SERDES_STATUS_RUNNING) {
		sbl_dev_err(sbl->dev, "%d: pml serdes not running", port_num);
		err = -ENOLINK;
		goto out_err;
	}

	/*
	 * pcs must be sending am so link partner can tune
	 */
	if (!(link->link_info & SBL_LINK_INFO_PCS_TX_AM)) {
		sbl_dev_err(sbl->dev, "%d: pml pcs not tx am", port_num);
		err = -ENODATA;
		goto out_err;
	}

	/* make sure we have time left */
	 sbl_start_timeout_ensure_remaining(sbl, port_num,
			 SBL_PML_MIN_START_TIME);

	/* make sure am locking is enabled */
	if (!(link->link_info & SBL_LINK_INFO_PCS_ALIGN_EN))
		sbl_pml_pcs_enable_alignment(sbl, port_num);

	err = sbl_pml_mac_config(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "%d: pml mac config failed [%d]", port_num, err);
		goto out_err;
	}

	sbl_pml_llr_config(sbl, port_num);

	/*
	 * keep trying to get the PML block up until
	 * we succeed, timeout or have a fatal error
	 */
	while (true) {

		/* wait for the pcs to come up  */
		err = sbl_pml_pcs_wait(sbl, port_num);
		switch (err) {
		case 0:
			/* good - carry on with mac */
			break;
		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "%d: pml pcs wait cancelled", port_num);
			goto out_err;
		case -ETIMEDOUT:
			sbl_dev_dbg(sbl->dev, "%d: pml pcs wait timeout", port_num);
			goto out_err_invalidate;
		default:
			sbl_dev_dbg(sbl->dev, "%d: pml pcs wait failed [%d]", port_num, err);
			goto out_err_invalidate;
		}

		sbl_pml_pcs_enable_auto_lane_degrade(sbl, port_num);

		/* start mac */
		sbl_pml_mac_start(sbl, port_num);

		/* start llr */
		err = sbl_pml_llr_start(sbl, port_num);
		switch (err) {
		case 0:
			/* all good */
			goto out_done;
		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "%d: pml llr start cancelled", port_num);
			goto out_err;
		case -ETIMEDOUT:
			sbl_dev_dbg(sbl->dev, "%d: pml llr start timeout", port_num);
			goto out_err;
		default:
			sbl_dev_dbg(sbl->dev, "%d: pml llr start failed [%d]", port_num, err);
			goto out_err;
		}
	};

out_done:
	/* clear any PML errors which might have been set during startup */
	sbl_pml_err_flgs_clear_all(sbl, port_num);

	/* pml block is up */
	sbl_dev_dbg(sbl->dev, "%d: pml bring-up done (up)", port_num);
	return 0;

out_err_invalidate:
	if (link->dfe_tune_count == SBL_DFE_USED_SAVED_PARAMS) {
		/*
		 * either the link partner is not starting or the saved params could be bad -
		 * we cannot (currently) determine which so destroy the saved params
		 * just in case
		 */
		sbl_dev_dbg(sbl->dev, "%d: pml bring-up - invalidating tuning params", port_num);
		sbl_serdes_invalidate_tuning_params(sbl, port_num);
	}
out_err:
	sbl_pml_mac_stop(sbl, port_num);
	if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_CLEANUP))
		sbl_pml_pcs_disable_alignment(sbl, port_num);
	sbl_dev_dbg(sbl->dev, "%d: pml bring-up failed [%d]", port_num, err);
	return err;
}


/*
 * Take down the PML block of a link
 */
int sbl_pml_link_down(struct sbl_inst *sbl, int port_num)
{
	sbl_dev_dbg(sbl->dev, "%d: pml bring-down starting\n", port_num);

	/* stop everything in order */
	sbl_pml_llr_stop(sbl, port_num);
	sbl_pml_mac_stop(sbl, port_num);
	sbl_pml_pcs_stop(sbl, port_num);

	/* clear pml errors */
	sbl_pml_err_flgs_clear_all(sbl, port_num);

	sbl_dev_dbg(sbl->dev, "%d: pml bring-down done (down)\n", port_num);
	return 0;
}

/*
 * function to reset the major PML cfg regs to their default values
 *
 *   It would seem that the default for the llr down behaviour is
 *   SBL_LLR_LINK_DOWN_BLOCK this should be SBL_LLR_LINK_DOWN_DISCARD;
 */
void sbl_pml_set_defaults(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	/* pcs */
	if (link->sstate != SBL_SERDES_STATUS_DOWN) {
		/*
		 * default is to not tx am.
		 * we should not do this because it can break optical headshells
		 */
		sbl_dev_warn(sbl->dev, "%d: pcs reset while serdes is running\n", port_num);
	}

	sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET,
			SBL_PML_CFG_PCS_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_OFFSET,
			SBL_PML_CFG_PCS_AUTONEG_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_TIMERS_OFFSET,
			SBL_PML_CFG_PCS_AUTONEG_TIMERS_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_BASE_PAGE_OFFSET,
			SBL_PML_CFG_PCS_AUTONEG_BASE_PAGE_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_OFFSET,
			SBL_PML_CFG_PCS_AUTONEG_NEXT_PAGE_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_AMS_OFFSET,
			SBL_PML_CFG_PCS_AMS_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET,
			SBL_PML_CFG_TX_PCS_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET,
			SBL_PML_CFG_RX_PCS_DFLT);

	sbl_write64(sbl, base|SBL_PML_DBG_PCS_OFFSET, SBL_PML_DBG_PCS_DFLT);

	/* pcs: we need special AMs for 200GHz mode */
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_AMS_OFFSET,
			SBL_PML_CFG_RX_PCS_AMS_SET(SBL_PCS_200_UM_MATCH_MSK,
					SBL_PCS_200_CM_MATCH_MSK));
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_CM_OFFSET,
			SBL_PCS_200_CM);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(0), SBL_PCS_200_UM0);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(1), SBL_PCS_200_UM1);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(2), SBL_PCS_200_UM2);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(3), SBL_PCS_200_UM3);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(4), SBL_PCS_200_UM4);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(5), SBL_PCS_200_UM5);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(6), SBL_PCS_200_UM6);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_UM_OFFSET(7), SBL_PCS_200_UM7);

	/* assert we have destroyed any previous pcs config */
	link->pcs_config = false;

	/* mac */
	sbl_write64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET,
			SBL_PML_CFG_TX_MAC_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET,
			SBL_PML_CFG_RX_MAC_DFLT);

	/* llr */
	val64 = SBL_PML_CFG_LLR_DFLT;
	val64 = SBL_PML_CFG_LLR_LINK_DOWN_BEHAVIOR_UPDATE(val64,
			sbl_pml_llr_link_down_behaviour(sbl, port_num));
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET,
			SBL_PML_CFG_LLR_SM_DFLT);

	/* read to flush everything */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
}


/*
 * interrupt support
 *
 *   we only have one handler
 */
int sbl_pml_install_intr_handler(struct sbl_inst *sbl, int port_num, u64 err_flags)
{
	struct sbl_link *link = sbl->link + port_num;

	if (link->intr_err_flgs) {
		sbl_dev_err(sbl->dev, "intr %d: handler already registered\n", port_num);
		return -EALREADY;
	}
	link->intr_err_flgs = err_flags;

	return (*sbl->ops.sbl_pml_install_intr_handler)(sbl->accessor, port_num, err_flags);
}


int sbl_pml_enable_intr_handler(struct sbl_inst *sbl, int port_num, u64 err_flags)
{
	struct sbl_link *link = sbl->link + port_num;

	if (!link->intr_err_flgs) {
		sbl_dev_warn(sbl->dev, "intr %d: no handler registered for enable\n", port_num);
		return 0;
	}

	if ((err_flags & ~link->intr_err_flgs) || !(err_flags & link->intr_err_flgs)) {
		sbl_dev_err(sbl->dev, "intr %d: cannot enable flags 0x%llx, 0x%llx\n", port_num,
				err_flags, link->intr_err_flgs);
		return -EINVAL;
	}

	return (*sbl->ops.sbl_pml_enable_intr_handler)(sbl->accessor, port_num, err_flags);
}


int sbl_pml_disable_intr_handler(struct sbl_inst *sbl, int port_num, u64 err_flags)
{
	struct sbl_link *link = sbl->link + port_num;

	if (!link->intr_err_flgs) {
		sbl_dev_warn(sbl->dev, "intr %d: no handler registered for disable\n", port_num);
		return 0;
	}

	if ((err_flags & ~link->intr_err_flgs) || !(err_flags & link->intr_err_flgs)) {
		sbl_dev_err(sbl->dev, "intr %d: cannot disable flags 0x%llx, 0x%llx\n", port_num,
				err_flags, link->intr_err_flgs);
		return -EINVAL;
	}

	return (*sbl->ops.sbl_pml_disable_intr_handler)(sbl->accessor, port_num, err_flags);
}


int sbl_pml_remove_intr_handler(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int err;

	if (!link->intr_err_flgs) {
		sbl_dev_warn(sbl->dev, "intr %d: no handler registered to remove\n", port_num);
		return 0;
	}

	err =  (*sbl->ops.sbl_pml_remove_intr_handler)(sbl->accessor, port_num,
			link->intr_err_flgs);
	link->intr_err_flgs = 0;

	return err;
}


/*
 * Error flags
 *
 *   TODO: inline?
 */
bool sbl_pml_err_flgs_test(struct sbl_inst *sbl, int port_num, u64 err_flgs)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_ERR_FLG_OFFSET);

	return val64 & err_flgs;
}


void sbl_pml_err_flgs_clear(struct sbl_inst *sbl, int port_num, u64 err_flgs)
{
	u32 base = SBL_PML_BASE(port_num);

	sbl_write64(sbl, base|SBL_PML_ERR_CLR_OFFSET, err_flgs);
}


void sbl_pml_err_flgs_clear_all(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 err_flgs;

	err_flgs = sbl_read64(sbl, base|SBL_PML_ERR_FLG_OFFSET);
	sbl_write64(sbl, base|SBL_PML_ERR_CLR_OFFSET, err_flgs);
}

bool sbl_pml_recovery_ignore_down_origin_fault(u32 down_origin)
{
	switch (down_origin) {
	case SBL_LINK_DOWN_ORIGIN_LINK_DOWN:
	case SBL_LINK_DOWN_ORIGIN_LOCAL_FAULT:
	case SBL_LINK_DOWN_ORIGIN_REMOTE_FAULT:
	case SBL_LINK_DOWN_ORIGIN_ALIGN:
	case SBL_LINK_DOWN_ORIGIN_HISER:
	case SBL_LINK_DOWN_ORIGIN_LLR_MAX:
		return true;
	default:
		return false;
	}
}

void sbl_pml_link_down_async_alert(struct sbl_inst *sbl, int port_num, u32 down_origin)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u64 err_flags;

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET);
	if (SBL_PML_CFG_LLR_SM_REPLAY_CT_MAX_GET(val64) < SBL_LLR_REPLAY_CT_MAX_UNLIMITED)
		err_flags = SBL_PML_FAULT_ERR_FLAGS;
	else
		err_flags = SBL_PML_REC_FAULT_ERR_FLAGS;

	/*
	 * going down or in recovery state, so don't need more intrs
	*/
	sbl_pml_disable_intr_handler(sbl, port_num, err_flags);

	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_CLEANUP)) {
		/*
		 * set state to error and signal no cleanup with the error number
		 */
		link->blstate = SBL_BASE_LINK_STATUS_ERROR;
		link->blerr = -ECONNABORTED;
	}

	sbl_async_alert(sbl, port_num, SBL_ASYNC_ALERT_LINK_DOWN,
			(void *)(uintptr_t)down_origin, 0);
}

static void sbl_pml_recovery_origin_counter_update(struct sbl_inst *sbl, int port_num,
						   enum sbl_link_down_origin origin)
{
	switch (origin) {
	case SBL_LINK_DOWN_ORIGIN_LINK_DOWN:
		sbl_link_counters_incr(sbl, port_num, pml_recovery_origin_bl_ldown);
		break;
	case SBL_LINK_DOWN_ORIGIN_LOCAL_FAULT:
		sbl_link_counters_incr(sbl, port_num, pml_recovery_origin_bl_lfault);
		break;
	case SBL_LINK_DOWN_ORIGIN_REMOTE_FAULT:
		sbl_link_counters_incr(sbl, port_num, pml_recovery_origin_bl_rfault);
		break;
	case SBL_LINK_DOWN_ORIGIN_ALIGN:
		sbl_link_counters_incr(sbl, port_num, pml_recovery_origin_bl_align);
		break;
	case SBL_LINK_DOWN_ORIGIN_HISER:
		sbl_link_counters_incr(sbl, port_num, pml_recovery_origin_bl_hiser);
		break;
	case SBL_LINK_DOWN_ORIGIN_LLR_MAX:
		sbl_link_counters_incr(sbl, port_num, pml_recovery_origin_bl_llr);
		break;
	default:
		sbl_dev_warn(sbl->dev, "%d: pml recovery origin (%d) with no counter",
			     port_num, origin);
		break;
	}
}

/*
 * PML recovery is limited by the combined amount of time spent in one or more
 * recovery attempts over a window, rather than by a count. This approximates
 * bandwidth loss. For example, 60 ms per second in PML recovery corresponds
 * to a roughly %6 loss of bandwidth.
 *
 * After each successful recovery, the duration is subtracted from the
 * remaining time budgeted for the window. The rate test fails if the remaining
 * time is insufficient for another attempt.
 */
static bool sbl_pml_recovery_rate_test(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u64 window_end = link->pml_recovery.rl_window_start + msecs_to_jiffies(link->blattr.pml_recovery.rl_window_size);

	/* reset if not started or window has elapsed */
	if (!link->pml_recovery.rl_window_start ||
	     link->pml_recovery.init_jiffies > window_end) {
		link->pml_recovery.rl_window_start = link->pml_recovery.init_jiffies;
		link->pml_recovery.rl_time_remaining = link->blattr.pml_recovery.rl_max_duration;
		return true;
	}

	if (link->pml_recovery.rl_time_remaining < SBL_PML_REC_POLL_INTERVAL)
		return false;

	return true;
}

static void sbl_pml_recovery_monitor_fallback_timer(struct timer_list *t)
{
	struct sbl_pml_recovery *pml_recovery = container_of(t, struct sbl_pml_recovery, timer);
	struct sbl_inst *sbl;
	struct sbl_link *link;
	int port_num;
	u32 down_origin;
	u32 elapsed;
	u32 rl_total_time;
	unsigned long timeout;
	unsigned long irq_flags;

	if (!pml_recovery || !pml_recovery->sbl || !pml_recovery->started)
		return;

	sbl = pml_recovery->sbl;
	port_num = pml_recovery->port_num;
	down_origin = pml_recovery->down_origin;
	link = sbl->link + port_num;
	timeout = pml_recovery->init_jiffies + msecs_to_jiffies(pml_recovery->timeout);
	elapsed = jiffies_to_msecs(jiffies - link->pml_recovery.init_jiffies);
	link->pml_recovery.rl_time_remaining -= jiffies_to_msecs(jiffies - link->pml_recovery.last_poll_jiffies);
	link->pml_recovery.last_poll_jiffies  = jiffies;

	if (sbl_pml_recovery_no_faults(sbl, port_num)) {
		sbl_dev_info(sbl->dev, "%d: PML recovered successfully in %ums", port_num, elapsed);
		sbl_link_counters_incr(sbl, port_num, pml_recovery_successes);
		sbl_pml_recovery_origin_counter_update(sbl, port_num, down_origin);
		sbl_link_counters_incr(sbl, port_num, (elapsed < SBL_PML_REC_HISTOGRAM_MAX) ?
				      (pml_recovery_histogram_0_9ms + elapsed / 10) :
				       pml_recovery_histogram_high);
		goto out;
	} else if (time_is_before_eq_jiffies(timeout)) {
		sbl_dev_info(sbl->dev, "%d: PML recovery monitor timed out (%ums)", port_num, elapsed);
		goto out_fail;
	} else if (!sbl_pml_recovery_rate_test(sbl, port_num)) {
		rl_total_time = link->blattr.pml_recovery.rl_max_duration - link->pml_recovery.rl_time_remaining;
		sbl_dev_err(sbl->dev, "%d: PML recovery rate exceeded (%ums/%ums) after %ums", port_num, rl_total_time,
			    link->blattr.pml_recovery.rl_window_size, elapsed);
		sbl_link_counters_incr(sbl, port_num, pml_recovery_rate_exceeded);
		goto out_fail;
	}

	/* start timer for next poll */
	mod_timer(&pml_recovery->timer, jiffies + msecs_to_jiffies(SBL_PML_REC_POLL_INTERVAL));
	return;

out_fail:
	sbl_pml_link_down_async_alert(sbl, port_num, down_origin);

out:
	spin_lock_irqsave(&link->fec_discard_lock, irq_flags);
	link->fec_discard_time = jiffies;
	link->fec_discard_type = SBL_FEC_DISCARD_TYPE_PML_REC_END;
	spin_unlock_irqrestore(&link->fec_discard_lock, irq_flags);
	link->pml_recovery.started = false;
}

static void sbl_pml_recovery_monitor(struct sbl_inst *sbl, int port_num, u32 down_origin)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 rl_total_time;
	unsigned long irq_flags;

	if (!link->pml_recovery.started) {

		link->pml_recovery.started = true;
		link->pml_recovery.init_jiffies = jiffies;
		link->pml_recovery.last_poll_jiffies = link->pml_recovery.init_jiffies;
		link->pml_recovery.sbl = sbl;
		link->pml_recovery.port_num = port_num;
		link->pml_recovery.down_origin = down_origin;
		link->pml_recovery.timeout = link->blattr.pml_recovery.timeout;

		if (!sbl_pml_recovery_rate_test(sbl, port_num)) {
			rl_total_time = link->blattr.pml_recovery.rl_max_duration - link->pml_recovery.rl_time_remaining;
			sbl_dev_err(sbl->dev, "%d: PML recovery rate exceeded (%ums/%ums)", port_num, rl_total_time,
				    link->blattr.pml_recovery.rl_window_size);
			sbl_pml_link_down_async_alert(sbl, port_num, down_origin);
			sbl_link_counters_incr(sbl, port_num, pml_recovery_rate_exceeded);
			link->pml_recovery.started = false;
			return;
		}
		sbl_link_counters_incr(sbl, port_num, pml_recovery_attempts);

		spin_lock_irqsave(&link->fec_discard_lock, irq_flags);
		link->fec_discard_time = link->pml_recovery.init_jiffies;
		link->fec_discard_type = SBL_FEC_DISCARD_TYPE_PML_REC_START;
		spin_unlock_irqrestore(&link->fec_discard_lock, irq_flags);

		sbl_pml_pcs_disable_alignment(sbl, port_num);
		sbl_pml_pcs_enable_alignment(sbl, port_num);

		timer_setup(&link->pml_recovery.timer, sbl_pml_recovery_monitor_fallback_timer, TIMER_IRQSAFE);
		link->pml_recovery.timer.expires = jiffies + msecs_to_jiffies(SBL_PML_REC_POLL_INTERVAL);
		add_timer(&link->pml_recovery.timer);

		sbl_dev_info(sbl->dev, "%d: PML recovery started - %s", port_num,
			     sbl_down_origin_str(down_origin));
	}
}

void sbl_pml_recovery_cancel(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 elapsed = jiffies_to_msecs(jiffies - link->pml_recovery.init_jiffies);
	unsigned long irq_flags;

	del_timer_sync(&link->pml_recovery.timer);
	spin_lock_irqsave(&link->fec_discard_lock, irq_flags);
	link->fec_discard_time = jiffies;
	link->fec_discard_type = SBL_FEC_DISCARD_TYPE_PML_REC_END;
	spin_unlock_irqrestore(&link->fec_discard_lock, irq_flags);
	link->pml_recovery.started = false;

	sbl_dev_info(sbl->dev, "%d: PML recovery canceled (%ums)", port_num, elapsed);
}

/*
 * local intr handler called by surrounding framework
 *
 *    We could support registering sub-handlers here etc but actually we
 *    only use intrs for autoneg or detecting link-down so, for now, we
 *    will directly code these here.
 *
 *    Some autoneg flags can't be cleared here because they will just get
 *    immediately raised again, so instead we will disable them (=mask
 *    then out as intr sources)
 *
 *    We will do the same with link down because, again for now, we don't support
 *    its spontaneous clearing (i.e. regaining lock)
 *
 */
int sbl_pml_hdlr(struct sbl_inst *sbl, int port_num, void *data)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 raised_flgs;
	u64 sts_pcs_lane_degrade_reg;
	u32 down_origin = 0;
	char pcs_state_str[SBL_PCS_STATE_STR_LEN];
	u64 val64;
	struct lane_degrade degrade_data;
	int alert = SBL_ASYNC_ALERT_INVALID;
	unsigned long irq_flags;

	raised_flgs = sbl_read64(sbl, base|SBL_PML_ERR_FLG_OFFSET) & link->intr_err_flgs;

	if (!raised_flgs)
		return 0;

	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_TRACE_PML_INT)) {
		sbl_dev_info(sbl->dev,
			"%d: pml hdlr (%lld %lld hs%lld mr%lld ld%lld) in 0x%llx",
			port_num,
			SBL_PML_ERR_FLG_AUTONEG_PAGE_RECEIVED_GET(raised_flgs),
			SBL_PML_ERR_FLG_AUTONEG_COMPLETE_GET(raised_flgs),
			SBL_PML_ERR_FLG_PCS_HI_SER_GET(raised_flgs),
			SBL_PML_ERR_FLG_LLR_REPLAY_AT_MAX_GET(raised_flgs),
			SBL_PML_ERR_FLG_PCS_LINK_DOWN_GET(raised_flgs),
			link->intr_err_flgs);
	}

	sts_pcs_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_LANE_DEGRADE_OFFSET);
	degrade_data.tx = SBL_PML_STS_PCS_LANE_DEGRADE_LP_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg);
	degrade_data.rx = SBL_PML_STS_PCS_LANE_DEGRADE_RX_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg);
	if (SBL_PML_ERR_FLG_PCS_RX_DEGRADE_GET(raised_flgs) && degrade_data.tx && degrade_data.rx) {
		spin_lock_irqsave(&link->fec_discard_lock, irq_flags);
		link->fec_discard_time = jiffies;
		link->fec_discard_type = SBL_FEC_DISCARD_TYPE_RX_DEGRADE;
		spin_unlock_irqrestore(&link->fec_discard_lock, irq_flags);
		sbl_dev_warn(sbl->dev, "%d: RX side Degraded -> TX Lanes Available: 0x%llx - RX Lanes Available: 0x%llx",
				port_num, degrade_data.tx, degrade_data.rx);
		sbl_async_alert(sbl, port_num, SBL_ASYNC_ALERT_RX_DEGRADE,
				&degrade_data, sizeof(degrade_data));
	}

	if (SBL_PML_ERR_FLG_PCS_TX_DEGRADE_GET(raised_flgs) && degrade_data.tx && degrade_data.rx) {
		sbl_dev_warn(sbl->dev, "%d: TX side Degraded -> TX Lanes Available: 0x%llx - RX Lanes Available: 0x%llx",
				port_num, degrade_data.tx, degrade_data.rx);
		sbl_async_alert(sbl, port_num, SBL_ASYNC_ALERT_TX_DEGRADE,
				&degrade_data, sizeof(degrade_data));
	}

	if (SBL_PML_ERR_FLG_PCS_TX_DEGRADE_FAILURE_GET(raised_flgs))
	    alert = SBL_ASYNC_ALERT_TX_DEGRADE_FAILURE;

	if (SBL_PML_ERR_FLG_PCS_RX_DEGRADE_FAILURE_GET(raised_flgs))
	    alert = SBL_ASYNC_ALERT_RX_DEGRADE_FAILURE;

	if (alert) {
		/*
		 * if Auto Lane Degrade is enabled, we will print this message regardless of whether
		 * the link went down because of lane degrade failure or not
		 */
		sbl_dev_err(sbl->dev, "%d: pml hdlr - link going down - all lanes down [%d]", port_num, alert);
		down_origin = SBL_LINK_DOWN_ORIGIN_DEGRADE_FAILURE;
		sbl_async_alert(sbl, port_num, alert, NULL, 0);
	}

	/*
	 * autoneg err flags
	 */
	if (raised_flgs & SBL_AUTONEG_ERR_FLGS) {
		sbl_pml_disable_intr_handler(sbl, port_num, SBL_AUTONEG_ERR_FLGS);
		complete(&link->an_hw_change);
	}

	/*
	 * link faults
	 */
	if (SBL_PML_ERR_FLG_PCS_HI_SER_GET(raised_flgs)) {
		if (sbl_debug_option(sbl, port_num, SBL_DEBUG_IGNORE_HISER)) {
			sbl_dev_warn(sbl->dev, "%d: pml hdlr - hiser - ignored", port_num);
		} else {
			sbl_dev_dbg(sbl->dev, "%d: pml hdlr - hiser", port_num);
			down_origin = SBL_LINK_DOWN_ORIGIN_HISER;
		}
	}

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET);
	if (SBL_PML_ERR_FLG_LLR_REPLAY_AT_MAX_GET(raised_flgs) &&
	    SBL_PML_CFG_LLR_SM_REPLAY_CT_MAX_GET(val64) < SBL_LLR_REPLAY_CT_MAX_UNLIMITED) {
		sbl_dev_dbg(sbl->dev, "%d: pml hdlr - max llr replay", port_num);
		down_origin = SBL_LINK_DOWN_ORIGIN_LLR_MAX;
	}

	if (SBL_PML_ERR_FLG_PCS_LINK_DOWN_GET(raised_flgs)) {
		val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);

		if (sbl_debug_option(sbl, port_num, SBL_DEBUG_TRACE_PML_INT)) {
			sbl_dev_info(sbl->dev, "%d: pml hdlr - link down (%s)", port_num,
				sbl_pml_pcs_state_str(sbl, port_num, pcs_state_str,
				SBL_PCS_STATE_STR_LEN));
		}

		if (!SBL_PML_STS_RX_PCS_ALIGN_STATUS_GET(val64)) {
			if (sbl_debug_option(sbl, port_num, SBL_DEBUG_IGNORE_ALIGN)) {
				sbl_dev_warn(sbl->dev, "%d: pml hdlr - align - ignored", port_num);
			} else {
				sbl_dev_dbg(sbl->dev, "%d: pml hdlr - align", port_num);
				down_origin = SBL_LINK_DOWN_ORIGIN_ALIGN;
			}
		} else if (SBL_PML_STS_RX_PCS_LOCAL_FAULT_GET(val64)) {
			sbl_dev_dbg(sbl->dev, "%d: pml hdlr - local fault", port_num);
			down_origin = SBL_LINK_DOWN_ORIGIN_LOCAL_FAULT;
		} else if (SBL_PML_STS_RX_PCS_FAULT_GET(val64)) {
			if (link->blattr.options & SBL_DISABLE_PML_RECOVERY ||
			    sbl_debug_option(sbl, port_num, SBL_DEBUG_REMOTE_FAULT_RECOVERY)) {
				sbl_dev_dbg(sbl->dev, "%d: pml hdlr - remote fault", port_num);
				down_origin = SBL_LINK_DOWN_ORIGIN_REMOTE_FAULT;
			} else {
				sbl_dev_dbg(sbl->dev, "%d: pml hdlr - remote fault - ignored", port_num);
			}
		} else {
			sbl_dev_dbg(sbl->dev, "%d: pml hdlr - link down", port_num);
			down_origin = SBL_LINK_DOWN_ORIGIN_LINK_DOWN;
		}
	}

	if (down_origin) {
		if (!(link->blattr.options & SBL_DISABLE_PML_RECOVERY) && !link->is_degraded) {
			if (sbl_pml_recovery_ignore_down_origin_fault(down_origin)) {
				sbl_dev_dbg(sbl->dev, "%d: PML recovery, pml hdlr fault(%d) is ignored", port_num, down_origin);
				sbl_pml_recovery_monitor(sbl, port_num, down_origin);

			} else
				sbl_pml_link_down_async_alert(sbl, port_num, down_origin);
		} else {
			sbl_dev_info(sbl->dev, "%d: PML recovery is disabled", port_num);
			sbl_pml_link_down_async_alert(sbl, port_num, down_origin);
		}
	}

	sbl_write64(sbl, base|SBL_PML_ERR_CLR_OFFSET, raised_flgs);
	sbl_read64(sbl, base|SBL_PML_ERR_CLR_OFFSET);  /* flush */

	return 0;
}
EXPORT_SYMBOL(sbl_pml_hdlr);

void sbl_pml_recovery_log_link_down(struct sbl_inst *sbl, int port_num)
{
	if (!sbl_pml_recovery_no_faults(sbl, port_num)) {
		sbl_pml_recovery_log_pcs_status(sbl, port_num);
		sbl_log_port_eye_heights(sbl, port_num);
	}
}
EXPORT_SYMBOL(sbl_pml_recovery_log_link_down);
