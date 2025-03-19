// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <sbl/sbl_pml.h>

#include "uapi/sbl_kconfig.h"
#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_internal.h"
#include "sbl_an.h"


static int  sbl_pml_llr_ready_wait(struct sbl_inst *sbl, int port_num);
static int  sbl_pml_llr_measure_loop_time_ns(struct sbl_inst *sbl,
		int port_num, u64 *llr_loop_time);
static void sbl_pml_llr_calculate_capacity(struct sbl_inst *sbl,
		int port_num, u64 *max_data, u64 *max_seq);
static void sbl_pml_llr_enable_loop_timing(struct sbl_inst *sbl, int port_num);
static void sbl_pml_llr_disable_loop_timing(struct sbl_inst *sbl, int port_num);
static void sbl_pml_llr_mode_set(struct sbl_inst *sbl, int port_num, u32 llr_mode);
static int  sbl_pml_llr_mode_get(struct sbl_inst *sbl, int port_num, u32 *llr_mode);
static int  sbl_pml_llr_detect(struct sbl_inst *sbl, int port_num, u32 *llr_mode);


/*
 *  configure
 */
void sbl_pml_llr_config(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: LLR config", port_num);

	/* init to original requested */
	link->llr_mode = link->blattr.llr_mode;

	/* config */
	sbl_write64(sbl, base|SBL_PML_STS_LLR_MAX_USAGE_OFFSET, 0ULL);
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
	val64 = SBL_PML_CFG_LLR_LLR_MODE_UPDATE(val64, 0ULL);  /* LLR OFF */
	val64 = SBL_PML_CFG_LLR_PREAMBLE_SEQ_CHECK_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_LLR_ACK_NACK_ERR_CHECK_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_LLR_FILTER_LOSSLESS_WHEN_OFF_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_LLR_FILTER_CTL_FRAMES_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_LLR_SIZE_UPDATE(val64, 3ULL);
	val64 = SBL_PML_CFG_LLR_ENABLE_LOOP_TIMING_UPDATE(val64, 0ULL);  /* loop timing off */
	val64 = SBL_PML_CFG_LLR_LINK_DOWN_BEHAVIOR_UPDATE(val64,
			sbl_pml_llr_link_down_behaviour(sbl, port_num));
	val64 = SBL_PML_CFG_LLR_MAC_IF_CREDITS_UPDATE(val64,
			SBL_PML_CFG_LLR_MAC_IF_CREDITS_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);

	sbl_write64(sbl, base|SBL_PML_CFG_LLR_CF_SMAC_OFFSET,
			SBL_PML_CFG_LLR_CF_SMAC_DFLT);

	sbl_write64(sbl, base|SBL_PML_CFG_LLR_CF_ETYPE_OFFSET,
			SBL_PML_CFG_LLR_CF_ETYPE_DFLT);

	sbl_write64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET,
			SBL_PML_CFG_LLR_SM_DFLT);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_CF_RATES_OFFSET);
	val64 = SBL_PML_CFG_LLR_CF_RATES_LOOP_TIMING_PERIOD_UPDATE(val64, SBL_PML_LLR_TIMING_PERIOD);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_CF_RATES_OFFSET, val64);

	sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);  /* flush */
}


/*
 *  start
 */
int sbl_pml_llr_start(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 llr_max_data;
	u64 llr_max_seq;
	u64 val64;
	int err = -1;

	sbl_dev_dbg(sbl->dev, "%d: LLR start", port_num);

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_DISABLED);

	sbl_pml_llr_enable_loop_timing(sbl, port_num);

	/* get mode here */
	err = sbl_pml_llr_mode_get(sbl, port_num, &(link->llr_mode));
	if (err) {
		sbl_dev_err(sbl->dev, "%d: LLR get mode failed [%d]", port_num, err);
		goto out_err;
	}

	/* if LLR is OFF then nothing to do here */
	if (link->llr_mode == SBL_LLR_MODE_OFF)
		goto out;

	/* set HPC if needed */
	if (link->llr_options & SBL_PML_LLR_OPTION_HPC_WIH_LLR)
		sbl_pml_mac_hpc_set(sbl, port_num);

	/* start ordered sets */
	sbl_pml_pcs_ordered_sets(sbl, port_num, true);

	/* measure llr loop time if we don't have it already */
	if (!link->llr_loop_time) {
		err = sbl_pml_llr_measure_loop_time_ns(sbl, port_num, &link->llr_loop_time);
		if (err) {
			sbl_dev_err(sbl->dev, "%d: LLR loop measurement failed [%d]", port_num, err);
			goto out_err;
		}
	}

	/* set max replay time from loop-back time */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET);
	if (link->blattr.options & SBL_DISABLE_PML_RECOVERY)
		val64 = SBL_PML_CFG_LLR_SM_REPLAY_CT_MAX_UPDATE(val64, SBL_DFLT_REPLAY_CT_MAX);
	else
		val64 = SBL_PML_CFG_LLR_SM_REPLAY_CT_MAX_UPDATE(val64, SBL_LLR_REPLAY_CT_MAX_UNLIMITED);
	val64 = SBL_PML_CFG_LLR_SM_REPLAY_TIMER_MAX_UPDATE(val64,
			3 * link->llr_loop_time + 500);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_SM_OFFSET);  /* flush */

	/* capacity configuration */
	if (link->blattr.options & SBL_OPT_FABRIC_LINK) {
		/* these can be set to their defaults for fabric links */
		sbl_write64(sbl, base|SBL_PML_CFG_LLR_CAPACITY_OFFSET,
				SBL_PML_CFG_LLR_CAPACITY_DFLT);
		sbl_read64(sbl, base|SBL_PML_CFG_LLR_CAPACITY_OFFSET);  /* flush */
	} else {
		sbl_pml_llr_calculate_capacity(sbl, port_num, &llr_max_data, &llr_max_seq);
		val64 = SBL_PML_CFG_LLR_CAPACITY_MAX_DATA_SET(llr_max_data) |
			SBL_PML_CFG_LLR_CAPACITY_MAX_SEQ_SET(llr_max_seq);
		sbl_write64(sbl, base|SBL_PML_CFG_LLR_CAPACITY_OFFSET, val64);
		sbl_read64(sbl, base|SBL_PML_CFG_LLR_CAPACITY_OFFSET);  /* flush */
	}

	/* set max data age timer & link down timer */
	if (link->blattr.options & SBL_DISABLE_PML_RECOVERY)
		sbl_write64(sbl, base|SBL_PML_CFG_LLR_TIMEOUTS_OFFSET, SBL_PML_CFG_LLR_TIMEOUTS_DFLT);
	else {
		val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_TIMEOUTS_OFFSET);
		val64 = SBL_PML_CFG_LLR_TIMEOUTS_DATA_AGE_TIMER_MAX_UPDATE(val64,
				(link->blattr.pml_recovery.timeout +
				 SBL_PML_REC_LLR_TIMEOUT_OFFSET) * 1000000);
		val64 = SBL_PML_CFG_LLR_TIMEOUTS_PCS_LINK_DN_TIMER_MAX_UPDATE(val64,
				(link->blattr.pml_recovery.timeout +
				 SBL_PML_REC_LLR_TIMEOUT_OFFSET) * 1000000);
		sbl_write64(sbl, base|SBL_PML_CFG_LLR_TIMEOUTS_OFFSET, val64);
	}
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_TIMEOUTS_OFFSET);  /* flush */

	/* clear all the llr related error flags */
	sbl_pml_err_flgs_clear(sbl, port_num, SBL_PML_ALL_LLR_ERR_FLGS);

	/* set required mode */
	sbl_pml_llr_mode_set(sbl, port_num, link->llr_mode);

	/* wait for llr to start */
	err = sbl_pml_llr_ready_wait(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "%d: LLR ready wait failed [%d]", port_num, err);
		goto out_err;
	}

	/* success */
	sbl_dev_dbg(sbl->dev, "%d: LLR running", port_num);
	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LLR_RUN);

out:
	sbl_pml_llr_disable_loop_timing(sbl, port_num);
	return 0;

out_err:
	sbl_pml_llr_disable_loop_timing(sbl, port_num);
	sbl_pml_llr_stop(sbl, port_num);
	return err;
}


static int sbl_pml_llr_ready_wait(struct sbl_inst *sbl, int port_num)
{
	int err = -1;

	sbl_dev_dbg(sbl->dev, "%d: LLR ready wait", port_num);

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LLR_WAIT);

	while (sbl_pml_llr_get_state(sbl, port_num) != SBL_PML_LLR_STATE_ADVANCE) {

		if (sbl_start_timeout(sbl, port_num)) {
			sbl_dev_err(sbl->dev, "%d: LLR ready wait timeout", port_num);
			err = -ETIMEDOUT;
			goto out;
		}

		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			sbl_dev_err(sbl->dev, "%d: LLR ready wait cancelled", port_num);
			err = -ECANCELED;
			goto out;
		}

		/* should be relatively well synchronised by this point */
		usleep_range(5000, 6000);
	}

	err = 0;

out:
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_WAIT);
	return err;
}


u32 sbl_pml_llr_get_state(struct sbl_inst *sbl, int port_num)
{
	u32 base  = SBL_PML_BASE(port_num);
	u64 val64 = sbl_read64(sbl, base|SBL_PML_STS_LLR_OFFSET);

	switch (SBL_PML_STS_LLR_LLR_STATE_GET(val64)) {
	case 0: /* OFF */
		return SBL_PML_LLR_STATE_OFF;
	case 1: /* INIT */
		return SBL_PML_LLR_STATE_INIT;
	case 2: /* ADVANCE */
		return SBL_PML_LLR_STATE_ADVANCE;
	case 3: /* HALT */
		return SBL_PML_LLR_STATE_HALT;
	case 4: /* REPLAY */
		return SBL_PML_LLR_STATE_REPLAY;
	case 5: /* DISCARD */
		return SBL_PML_LLR_STATE_DISCARD;
	case 6: /* MONITOR */
		return SBL_PML_LLR_STATE_MONITOR;
	default:
		return SBL_PML_LLR_STATE_UNKNOWN;
	}
}
EXPORT_SYMBOL(sbl_pml_llr_get_state);


/*
 * calculate the loop time
 *
 *  try to make a number of measurements or timeout/cancelled
 */
static int sbl_pml_llr_measure_loop_time_ns(struct sbl_inst *sbl, int port_num,
		u64 *llr_loop_time)
{
	struct sbl_link *link;
	u32              base;
	u64              min_loop_time;
	u64              max_loop_time;
	int              x;
	u64              val64;
	u64              time64;
	int              err;

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LLR_MEASURE);

	link = sbl->link + port_num;
	base = SBL_PML_BASE(port_num);

	min_loop_time  = SBL_PML_LLR_MIN_LOOP_TIME;
	max_loop_time  = SBL_PML_LLR_MAX_LOOP_TIME;
	*llr_loop_time = SBL_PML_LLR_MAX_LOOP_TIME;
	time64         = 0ULL;

	sbl_dev_dbg(sbl->dev, "%d: LLR measure loop time (len = %s)(media = %s)",
		port_num, sbl_link_len_str(link->mattr.len), sbl_link_media_str(link->mattr.media));

	/* calc loop time bounds */
	if (link->mattr.len != SBL_LINK_LEN_INVALID) {
		err = sbl_media_calc_loop_time(sbl, port_num, &min_loop_time);
		if (err) {
			sbl_dev_err(sbl->dev, "%d: LLR measure calc loop time failed [%d]", port_num, err);
			return err;
		}
		/* arbitrary range for max */
		max_loop_time = min_loop_time + 100 /* ns */;
	}
	sbl_dev_dbg(sbl->dev, "%d: LLR measure loop time bounds: min = %lldns, max = %lldns",
		port_num, min_loop_time, max_loop_time);

	/* measure loop time */
	for (x = 0; x < SBL_PML_LLR_TIMING_LOOPS; ++x) {

		if (sbl_start_timeout(sbl, port_num)) {
			sbl_dev_err(sbl->dev, "%d: LLR measure loop time timeout", port_num);
			err = -ETIMEDOUT;
			goto out;
		}

		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			sbl_dev_err(sbl->dev, "%d: LLR measure loop time cancelled", port_num);
			err = -ECANCELED;
			goto out;
		}

		/* start a measurement */
		sbl_write64(sbl, base|SBL_PML_STS_LLR_LOOP_TIME_OFFSET, 0ULL);
		sbl_read64(sbl, base|SBL_PML_STS_LLR_LOOP_TIME_OFFSET);  /* flush */
		/* give the measurement time to complete */
		udelay(SBL_PML_LLR_TIMING_MEASURE_DELAY);
		/* read the measurement */
		val64 = sbl_read64(sbl, base|SBL_PML_STS_LLR_LOOP_TIME_OFFSET);

		time64 = SBL_PML_STS_LLR_LOOP_TIME_LOOP_TIME_GET(val64); /* ns */
		sbl_dev_dbg(sbl->dev, "%d: LLR measure time = %lldns (%d)", port_num, time64, x);

		/* if we didn't get a valid time, then wait to retry */
		if (!time64)
			msleep(SBL_PML_LLR_TIMING_RETRY_DELAY);

		/* if we did get a valid time, then check it here */
		else if ((time64 >= min_loop_time) && (time64 <= max_loop_time)) {
			/* save the fastest time here */
			if (time64 < *llr_loop_time)
				*llr_loop_time = time64;
		}
	}

	/* check result here */
	if (*llr_loop_time == SBL_PML_LLR_MAX_LOOP_TIME) {
		if (sbl_debug_option(sbl, port_num, SBL_DEBUG_ALLOW_LOOP_TIME_FAIL)) {
			sbl_dev_err(sbl->dev, "%d: LLR measure loop time failed (min = %lld, max = %lld, last = %lld)",
				port_num, min_loop_time, max_loop_time, time64);
			err = -ENODATA;
			goto out;
		}
		/* set to max calculated loop time */
		*llr_loop_time = max_loop_time;
	}

	sbl_dev_dbg(sbl->dev, "%d: LLR measure loop time = %lldns", port_num, *llr_loop_time);
	err = 0;

out:
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_MEASURE);
	return err;
}


/*
 * calculate the required size of the llr buffer
 *
 *   we will use the measured loop time not cable length as we dont
 *   always have that.
 *
 *   TODO: link degrade & mfs
 */
static void sbl_pml_llr_calculate_capacity(struct sbl_inst *sbl, int port_num,
		u64 *max_data, u64 *max_seq)
{
	struct sbl_link *link = sbl->link + port_num;

	u64 bytes_per_ns = 25;
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	u64 bytes_per_frame = sbl_get_max_frame_size(sbl, port_num);
#else
	u64 bytes_per_frame = 9216;  /* cassini jumbo frame */
#endif

	u64 cap_data_max;
	u64 cap_seq_max;

	u64 calc;

	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		bytes_per_ns = 25;  /* 25000us */
		break;
	case SBL_LINK_MODE_BJ_100G:
	case SBL_LINK_MODE_CD_100G:
		bytes_per_ns = 13;  /* 12500us */
		break;
	case SBL_LINK_MODE_CD_50G:
		bytes_per_ns =  7;  /*  6250us */
		break;
	}

#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	switch (link->blattr.link_partner) {
	case SBL_LINK_PARTNER_SWITCH:
		sbl_dev_dbg(sbl->dev, "%d: LLR fabric link detected", port_num);
		cap_data_max = sbl_llr_fabric_cap_data_max_get();
		cap_seq_max  = sbl_llr_fabric_cap_seq_max_get();
		break;
	case SBL_LINK_PARTNER_NIC:
	case SBL_LINK_PARTNER_NIC_C2:
		sbl_dev_dbg(sbl->dev, "%d: LLR edge link detected", port_num);
		cap_data_max = sbl_llr_edge_cap_data_max_get();
		cap_seq_max  = sbl_llr_edge_cap_seq_max_get();
		break;
	default:
		sbl_dev_dbg(sbl->dev, "%d: LLR unknown link partner", port_num);
		cap_data_max = sbl_llr_edge_cap_data_max_get();
		cap_seq_max  = sbl_llr_edge_cap_seq_max_get();
	}
#else
	cap_data_max = sbl_llr_edge_cap_data_max_get();
	cap_seq_max  = sbl_llr_edge_cap_seq_max_get();
#endif

	calc = (link->llr_loop_time * bytes_per_ns) + (bytes_per_frame * SBL_PML_LLR_NUM_FRAMES);

	/* units 48 byte quanta */
	*max_data = DIV_ROUND_UP(calc, 48);
	if (*max_data > cap_data_max) {
		sbl_dev_dbg(sbl->dev,
			"%d: LLR max data cap 0x%llx out of bounds, setting to 0x%llx",
			port_num, *max_data, cap_data_max);
		*max_data = cap_data_max;
	}

	/* units 32 byte frame size */
	*max_seq  = DIV_ROUND_UP(calc, 32);
	if (*max_seq > cap_seq_max) {
		sbl_dev_dbg(sbl->dev,
			"%d: LLR max seq cap 0x%llx out of bounds, setting to 0x%llx",
			port_num, *max_seq, cap_seq_max);
		*max_seq = cap_seq_max;
	}

	sbl_dev_dbg(sbl->dev,
		"%d: LLR cap: data = 0x%llx, seq = 0x%llx",
		port_num, *max_data, *max_seq);
}


/*
 * enable llr timing measurements
 */
static void sbl_pml_llr_enable_loop_timing(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: LLR enable loop timing", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
	val64 = SBL_PML_CFG_LLR_ENABLE_LOOP_TIMING_UPDATE(val64, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);  /* flush */

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LLR_LOOP);
}


/*
 * disable llr timing measurements
 */
static void sbl_pml_llr_disable_loop_timing(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: LLR disable loop timing", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
	val64 = SBL_PML_CFG_LLR_ENABLE_LOOP_TIMING_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);  /* flush */

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_LOOP);
}


/*
 * stop
 */
void sbl_pml_llr_stop(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: LLR stop", port_num);

	link->llr_mode = SBL_LLR_MODE_OFF;

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
	val64 = SBL_PML_CFG_LLR_LLR_MODE_UPDATE(val64, 0LL);  /* LLR OFF */
	val64 = SBL_PML_CFG_LLR_FILTER_LOSSLESS_WHEN_OFF_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_LLR_FILTER_CTL_FRAMES_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_LLR_ENABLE_LOOP_TIMING_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_LLR_LINK_DOWN_BEHAVIOR_UPDATE(val64,
			sbl_pml_llr_link_down_behaviour(sbl, port_num));
	val64 = SBL_PML_CFG_LLR_MAC_IF_CREDITS_UPDATE(val64,
			SBL_PML_CFG_LLR_MAC_IF_CREDITS_DFLT);
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);  /* flush */

	sbl_pml_pcs_ordered_sets(sbl, port_num, false);

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_RUN);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_DISABLED);
}
EXPORT_SYMBOL(sbl_pml_llr_stop);


/*
 * disable
 *
 *  called by the fabric LSM when the link goes into draining
 *  the llr is stopped
 *
 */
void sbl_pml_llr_disable(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: LLR disable", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
	val64 = SBL_PML_CFG_LLR_LLR_MODE_UPDATE(val64, 0ULL); /* LLR OFF */
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);  /* flush */

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LLR_DISABLED);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_RUN);
}
EXPORT_SYMBOL(sbl_pml_llr_disable);


/*
 * enable
 *
 *  called by the fabric LSM when the link goes into starting
 *  the llr is restored to its previous state
 *
 */
void sbl_pml_llr_enable(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: LLR enable", port_num);

	sbl_pml_llr_mode_set(sbl, port_num, link->llr_mode);
}
EXPORT_SYMBOL(sbl_pml_llr_enable);

/*
 * check llr is ready with timeout
 */
bool sbl_pml_llr_check_is_ready(struct sbl_inst *sbl, int port_num, unsigned int timeout_ms)
{
	unsigned long to_jiffy = jiffies + msecs_to_jiffies(timeout_ms);

	sbl_dev_dbg(sbl->dev, "%d: LLR check is ready", port_num);

	do {
		if (sbl_pml_llr_get_state(sbl, port_num) == SBL_PML_LLR_STATE_ADVANCE)
			return true;
		usleep_range(5000, 6000);
	} while (time_before(jiffies, to_jiffy));

	return false;
}
EXPORT_SYMBOL(sbl_pml_llr_check_is_ready);


/*
 * behaviour when the link is down
 *   TODO - revisit with cassini and/or faster restart
 */
u64 sbl_pml_llr_link_down_behaviour(struct sbl_inst *sbl,
		int port_num)
{
	if (sbl_is_fabric_link(sbl, port_num)) {
		/*
		 *   fabric link should really be BLOCK, but only if the pcs
		 *   can recover quickly. Currently it can't - pcs failure means
		 *   going right back to tuning the serdes.
		 */
		return 0ULL;
	}

	return 0ULL;
}


/*
 * set LLR mode
 */
static void sbl_pml_llr_mode_set(struct sbl_inst *sbl, int port_num, u32 llr_mode)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: LLR mode set (%d)", port_num, llr_mode);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);
	switch (llr_mode) {
	case SBL_LLR_MODE_OFF:
		val64 = SBL_PML_CFG_LLR_LLR_MODE_UPDATE(val64, 0ULL);
		break;
	case SBL_LLR_MODE_MONITOR:
		val64 = SBL_PML_CFG_LLR_LLR_MODE_UPDATE(val64, 1ULL);
		break;
	case SBL_LLR_MODE_ON:
		val64 = SBL_PML_CFG_LLR_LLR_MODE_UPDATE(val64, 2ULL);
		break;
	default:
		sbl_dev_dbg(sbl->dev, "%d: LLR mode invalid (%d)", port_num, llr_mode);
		return;
	}
	sbl_write64(sbl, base|SBL_PML_CFG_LLR_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_LLR_OFFSET);  /* flush */
}


/*
 *  get LLR mode
 */
static int sbl_pml_llr_mode_get(struct sbl_inst *sbl, int port_num, u32 *llr_mode)
{
	struct sbl_link *link = sbl->link + port_num;
	int               err = -1;

	sbl_dev_dbg(sbl->dev, "%d: LLR mode get (%d)", port_num, link->blattr.llr_mode);

	switch (link->blattr.llr_mode) {

	case SBL_LLR_MODE_MONITOR:
		sbl_dev_dbg(sbl->dev, "%d: LLR mode monitor", port_num);
		*llr_mode = SBL_LLR_MODE_MONITOR;
		return 0;

	case SBL_LLR_MODE_ON:
		sbl_dev_dbg(sbl->dev, "%d: LLR mode on", port_num);
		goto out_on;

	case SBL_LLR_MODE_AUTO:
		/* check fabric link here */
		if (link->blattr.options & SBL_OPT_FABRIC_LINK) {
			sbl_dev_dbg(sbl->dev, "%d: LLR mode on fabric", port_num);
			goto out_on;
		}
		/* check ether link here */
		if (!(link->blattr.options & SBL_OPT_ENABLE_ETHER_LLR))
			goto out_off;
		/* check AN options here */
		if (link->an_options & AN_OPT_LLR) {
			if (link->an_options & AN_OPT_ETHER_LLR) {
				if (link->an_options & AN_OPT_HPC_WITH_LLR) {
					link->llr_options |= SBL_PML_LLR_OPTION_HPC_WIH_LLR;
					link->ifg_config = SBL_IFG_CONFIG_HPC;
					sbl_dev_dbg(sbl->dev, "%d: LLR mode on with HPC from AN", port_num);
					goto out_on;
				}
				sbl_dev_dbg(sbl->dev, "%d: LLR mode on without HPC from AN", port_num);
				goto out_on;
			}
			goto out_off;
		}
		/* do loop detect here */
		err = sbl_pml_llr_detect(sbl, port_num, &(link->llr_mode));
		switch (err) {
		case 0:
			if (link->llr_mode == SBL_LLR_MODE_OFF)
				goto out_off;
			if (link->blattr.options & SBL_OPT_ENABLE_IFG_HPC_WITH_LLR) {
				link->llr_options |= SBL_PML_LLR_OPTION_HPC_WIH_LLR;
				link->ifg_config = SBL_IFG_CONFIG_HPC;
				sbl_dev_dbg(sbl->dev, "%d: LLR mode on with HPC from LOOP", port_num);
				goto out_on;
			}
			sbl_dev_dbg(sbl->dev, "%d: LLR mode on without HPC from LOOP", port_num);
			goto out_on;
		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "%d: LLR detect cancelled", port_num);
			goto out_err;
		case -ETIMEDOUT:
			sbl_dev_dbg(sbl->dev, "%d: LLR detect timed out", port_num);
			goto out_err;
		default:
			goto out_off;
		}
		goto out_off;

	case SBL_LLR_MODE_OFF:
		goto out_off;

	default:
		sbl_dev_err(sbl->dev, "%d: LLR invalid mode [%d]", port_num, link->blattr.llr_mode);
		err = -EBADRQC;
		goto out_err;
	}

out_off:
	sbl_dev_dbg(sbl->dev, "%d: LLR mode off", port_num);
	*llr_mode = SBL_LLR_MODE_OFF;
	return 0;

out_on:
	*llr_mode = SBL_LLR_MODE_ON;
	return 0;

out_err:
	*llr_mode = SBL_LLR_MODE_OFF;
	return err;
}


/*
 *  LLR detect
 */
static int sbl_pml_llr_detect(struct sbl_inst *sbl, int port_num, u32 *llr_mode)
{
	u32           base       = SBL_PML_BASE(port_num);
	u64           val64      = 0;
	u64           time64     = 0;
	unsigned long last_jiffy = jiffies + msecs_to_jiffies(SBL_PML_LLR_DETECT_TIMEOUT);
	int           err        = -1;

	sbl_dev_dbg(sbl->dev, "%d: LLR detect", port_num);

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_LLR_DETECT);

	/* OFF unless we detect a loop */
	*llr_mode = SBL_LLR_MODE_OFF;

	sbl_write64(sbl, (base | SBL_PML_STS_LLR_LOOP_TIME_OFFSET), 0ULL);
	sbl_read64(sbl, (base | SBL_PML_STS_LLR_LOOP_TIME_OFFSET));  /* flush */
	do {

		if (sbl_start_timeout(sbl, port_num)) {
			err = -ETIMEDOUT;
			goto out;
		}

		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			err = -ECANCELED;
			goto out;
		}

		msleep(SBL_PML_LLR_DETECT_DELAY);

		val64 = sbl_read64(sbl, (base | SBL_PML_STS_LLR_LOOP_TIME_OFFSET));
		time64 = SBL_PML_STS_LLR_LOOP_TIME_LOOP_TIME_GET(val64);                 /* ns */
		sbl_dev_dbg(sbl->dev, "%d: LLR time = %lldns", port_num, time64);

		if (time64) {
			/* loop detected */
			if ((time64 < SBL_PML_LLR_MIN_LOOP_TIME) || (time64 > SBL_PML_LLR_MAX_LOOP_TIME)) {
				sbl_dev_warn(sbl->dev, "%d: LLR time out of bounds (%lldns)", port_num, time64);
				continue;
			}
			*llr_mode = SBL_LLR_MODE_ON;
			err = 0;
			goto out;
		}

	} while (time_before(jiffies, last_jiffy));

out:
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_LLR_DETECT);
	return err;
}
