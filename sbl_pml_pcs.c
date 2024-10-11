// SPDX-License-Identifier: GPL-2.0

/*
 * sbl_pml_pcs.c
 *
 * Copyright 2019,2022-2024 Hewlett Packard Enterprise Development LP
 *
 * core PML block functions
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <sbl/sbl_pml.h>

#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_serdes_map.h"
#include "sbl_internal.h"
#include "sbl_serdes_fn.h"

static int  sbl_pml_pcs_determine_active_lanes(struct sbl_inst *sbl, int port_num);
static int  sbl_pml_pcs_alignment_wait(struct sbl_inst *sbl, int port_num);
static int  sbl_pml_pcs_fault_clear_wait(struct sbl_inst *sbl, int port_num);
static bool sbl_pml_pcs_locked(struct sbl_inst *sbl, int port_num);
static bool sbl_pml_pcs_no_faults(struct sbl_inst *sbl, int port_num);
static void sbl_pml_pcs_start_lock(struct sbl_inst *sbl, int port_num);
static void sbl_pml_pcs_stop_lock(struct sbl_inst *sbl, int port_num);

/*
 * PCS configuration
 */
static int sbl_pml_pcs_config(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	int err;
	u64 en_lane_degrade_reg;

	sbl_dev_dbg(sbl->dev, "%d: pcs config", port_num);

	/* work out all the lane config and store in link db */
	err = sbl_pml_pcs_determine_active_lanes(sbl, port_num);
	if (err) {
		sbl_dev_dbg(sbl->dev, "%d: pcs config lanes failed [%d]", port_num, err);
		return err;
	}

	/* pcs must be off for setting  */
	sbl_pml_pcs_stop(sbl, port_num);

	/* clear out the previous active lanes and disable lock */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	val64 = SBL_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);

	/* rapid alignment is broken so disable */
	val64 = sbl_read64(sbl, base|SBL_PML_DBG_PCS_OFFSET);
	val64 = SBL_PML_DBG_PCS_ENABLE_RAPID_ALIGNMENT_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_DBG_PCS_OFFSET, val64);

	/* pcs config */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		val64 = SBL_PML_CFG_PCS_PCS_MODE_UPDATE(val64, 0ULL);
		break;
	case SBL_LINK_MODE_BJ_100G:
		val64 = SBL_PML_CFG_PCS_PCS_MODE_UPDATE(val64, 1ULL);
		break;
	case SBL_LINK_MODE_CD_100G:
		val64 = SBL_PML_CFG_PCS_PCS_MODE_UPDATE(val64, 2ULL);
		break;
	case SBL_LINK_MODE_CD_50G:
		val64 = SBL_PML_CFG_PCS_PCS_MODE_UPDATE(val64, 3ULL);
		break;
	default:
		sbl_dev_dbg(sbl->dev, "%d: pcs link mode invalid (%d)", port_num, link->link_mode);
		return -EBADRQC;
	}
	val64 = SBL_PML_CFG_PCS_ENABLE_AUTO_NEG_UPDATE(val64, 0ULL);
	if ((link->blattr.options & SBL_OPT_LANE_DEGRADE) &&
			(link->link_mode == SBL_LINK_MODE_BS_200G))
		val64 = SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(val64, 1ULL);
	else
		val64 = SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(val64, 0ULL);

	sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET, val64);

	/* pcs tx  */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET);
	val64 = SBL_PML_CFG_TX_PCS_ENABLE_CTL_OS_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_TX_PCS_CDC_READY_LEVEL_UPDATE(val64,
			SBL_PML_CFG_TX_PCS_CDC_READY_LEVEL_DFLT);
	val64 = SBL_PML_CFG_TX_PCS_GEARBOX_CREDITS_UPDATE(val64,
			SBL_PML_CFG_TX_PCS_GEARBOX_CREDITS_DFLT);
	val64 = SBL_PML_CFG_TX_PCS_LANE_0_SOURCE_UPDATE(val64,
			sbl->switch_info->ports[port_num].serdes[0].tx_lane_source);
	val64 = SBL_PML_CFG_TX_PCS_LANE_1_SOURCE_UPDATE(val64,
			sbl->switch_info->ports[port_num].serdes[1].tx_lane_source);
	val64 = SBL_PML_CFG_TX_PCS_LANE_2_SOURCE_UPDATE(val64,
			sbl->switch_info->ports[port_num].serdes[2].tx_lane_source);
	val64 = SBL_PML_CFG_TX_PCS_LANE_3_SOURCE_UPDATE(val64,
			sbl->switch_info->ports[port_num].serdes[3].tx_lane_source);
	sbl_write64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET, val64);

	/* pcs rx */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	switch (link->blattr.fec_mode) {
	case SBL_RS_MODE_OFF:
		val64 = SBL_PML_CFG_RX_PCS_RS_MODE_UPDATE(val64, 0ULL);
		break;
	case SBL_RS_MODE_OFF_SYN:
		val64 = SBL_PML_CFG_RX_PCS_RS_MODE_UPDATE(val64, 1ULL);
		break;
	case SBL_RS_MODE_OFF_CHK:
		val64 = SBL_PML_CFG_RX_PCS_RS_MODE_UPDATE(val64, 2ULL);
		break;
	case SBL_RS_MODE_ON_SYN_MRK:
		val64 = SBL_PML_CFG_RX_PCS_RS_MODE_UPDATE(val64, 3ULL);
		break;
	case SBL_RS_MODE_ON_CHK_SYN_MRK:
		val64 = SBL_PML_CFG_RX_PCS_RS_MODE_UPDATE(val64, 4ULL);
		break;
	case SBL_RS_MODE_ON:
		val64 = SBL_PML_CFG_RX_PCS_RS_MODE_UPDATE(val64, 5ULL);
		break;
	default:
		sbl_dev_dbg(sbl->dev, "%d: pcs fec mode invalid (%d)", port_num, link->blattr.fec_mode);
		return -EBADRQC;
       }
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_CTL_OS_UPDATE(val64, 0ULL);                   /* ordered sets off */
	val64 = SBL_PML_CFG_RX_PCS_HEALTH_BAD_SENSITIVITY_UPDATE(val64, 4ULL);          /* less sensitive */
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_RX_SM_UPDATE(val64,                           /* state machine */
			(link->blattr.options & SBL_OPT_FABRIC_LINK) ? 0ULL : 1ULL);
	en_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	/*
	 * If we disable the following CSR, Auto Lane Degrade is enabled and
	 * there is a concern regarding handling of short burst errors
	 */
	if (SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_GET(en_lane_degrade_reg))
		val64 = SBL_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(val64, 0ULL);     /* restart on 3 bad RS codewords - 0 with autodegrade enabled */
	else
		val64 = SBL_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(val64, 1ULL);     /* restart on 3 bad RS codewords - 1 with autodegrade disabled */
	val64 = SBL_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_AMS_UPDATE(val64,                /* restart on 5 bad AMs */
			(link->link_mode == SBL_LINK_MODE_BS_200G) ? 1ULL : 0ULL);
	val64 = SBL_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(val64, link->active_rx_lanes);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(val64, 0ULL);                     /* pcs disable lock  (really enable alignment process) */
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);

	link->pcs_config = true;

	return 0;
}

/*
 * Enables auto lane degrading in the hardware.
 *
 * This function can also be called if the AM_LOCK is observed on some lanes
 * but not all and the timeout has expired. This will allow auto lane degrade
 * feature to be used when not all lanes are available at the initial link
 * bringup.
 */
void sbl_pml_pcs_enable_auto_lane_degrade(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64, en_lane_degrade_reg;
	unsigned long timeout;
	int err = 0;

	en_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	if (SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_GET(en_lane_degrade_reg)) {
		timeout = jiffies + usecs_to_jiffies(HM_TIMEOUT);
		while (!sbl_pml_rx_pls_available(sbl, port_num)) {
			if (time_after(jiffies, timeout)) {
				sbl_dev_dbg(sbl->dev, "%d: timeout for rx pls available\n", port_num);
				err = -ETIMEDOUT;
				goto out;
			}
		}
		timeout = jiffies + usecs_to_jiffies(HM_TIMEOUT);
		while (!sbl_pml_lp_pls_available(sbl, port_num)) {
			if (time_after(jiffies, timeout)) {
				sbl_dev_dbg(sbl->dev, "%d: timeout for lp pls available\n", port_num);
				err = -ETIMEDOUT;
				goto out;
			}
		}

		val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
		val64 = SBL_PML_CFG_RX_PCS_ALLOW_AUTO_DEGRADE_UPDATE(val64, 1ULL);
		sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);

		val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET);
		val64 = SBL_PML_CFG_TX_PCS_ALLOW_AUTO_DEGRADE_UPDATE(val64, 1ULL);
		sbl_write64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET, val64);
		sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET); /* flush */
		sbl_dev_dbg(sbl->dev, "auto lane degrade is enabled");
	}
out:
	if (err) {
		sbl_dev_err(sbl->dev, "%d: auto lane degrade is not enabled", port_num);
		en_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
		en_lane_degrade_reg = SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(en_lane_degrade_reg, 0ULL);
		sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET, en_lane_degrade_reg);

		val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
		val64 = SBL_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(val64, 1ULL);
		sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);
		sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET); /* flush */
		sbl_dev_dbg(sbl->dev, "Restart lock on bad CWS set back to 1 since couldn't enable ALD\n");
	}
}


bool sbl_pml_rx_pls_available(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u64 sts_pcs_lane_degrade_reg;

	sts_pcs_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_LANE_DEGRADE_OFFSET);
	val64 = SBL_PML_STS_PCS_LANE_DEGRADE_RX_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg);

	return (val64 == MAX_PLS_AVAILABLE);
}

bool sbl_pml_lp_pls_available(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u64 sts_pcs_lane_degrade_reg;

	sts_pcs_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_LANE_DEGRADE_OFFSET);
	val64 = SBL_PML_STS_PCS_LANE_DEGRADE_LP_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg);

	return (val64 == MAX_PLS_AVAILABLE);
}


/*
 * Function to find the lanes that data will be arriving on and
 * setup the correct active lanes to align and active fec lanes.
 */
static int sbl_pml_pcs_determine_active_lanes(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct serdes_info *serdes =  sbl->switch_info->ports[port_num].serdes;
	int i;

	sbl_dev_dbg(sbl->dev, "%d: pcs config lanes", port_num);

	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		/*
		 * 4 lanes of 50Gbps - 8 internal 25Gbps fec lanes
		 * setup directly
		 */
		link->active_rx_lanes  = 0xf;    /* all */
		link->active_fec_lanes = 0xff;   /* all */
		break;
	case SBL_LINK_MODE_BJ_100G:
		/*
		 * 4 lanes of 25Gbps - 4 internal 25Gbps lanes
		 * setup directly
		 */
		link->active_rx_lanes  = 0xf;    /* all */
		link->active_fec_lanes = 0xf;    /* special case */
		break;
	case SBL_LINK_MODE_CD_100G:
		/*
		 * 2 lanes of 50Gbps - 4 internal 25Gbps lanes
		 * locate incoming lanes 0 and 1
		 */
		link->active_rx_lanes  = 0;
		link->active_fec_lanes = 0;
		if (link->loopback_mode == SBL_LOOPBACK_MODE_LOCAL) {
			/* we will receive on the tx lanes we went out on */
			for (i = 0; i < SBL_SERDES_LANES_PER_PORT; ++i) {
				if (serdes[i].tx_lane_source < 2) {
					link->active_rx_lanes  |= 0x1 << i;
					link->active_fec_lanes |= 0x3 << 2*i;
				}
			}
		} else {
			/* we will receive on the rx lanes */
			for (i = 0; i < SBL_SERDES_LANES_PER_PORT; ++i) {
				if (serdes[i].rx_lane_source < 2) {
					link->active_rx_lanes  |= 0x1 << i;
					link->active_fec_lanes |= 0x3 << 2*i;
				}
			}
		}
		break;
	case SBL_LINK_MODE_CD_50G:
		/*
		 * 1 lanes of 50Gbps - 2 internal 25Gbps lanes
		 * locate incoming lane 0
		 */
		if (link->loopback_mode == SBL_LOOPBACK_MODE_LOCAL) {
			/* we will receive on the lane 0 we went out on */
			for (i = 0; i < SBL_SERDES_LANES_PER_PORT; ++i) {
				if (serdes[i].tx_lane_source == 0) {
					link->active_rx_lanes  = 0x1 << i;
					link->active_fec_lanes = 0x3 << 2*i;
					break;
				}
			}
		} else {
			/* we will receive on the rx lane */
			for (i = 0; i < SBL_SERDES_LANES_PER_PORT; ++i) {
				if (serdes[i].rx_lane_source == 0) {
					link->active_rx_lanes  = 0x1 << i;
					link->active_fec_lanes = 0x3 << 2*i;
				}
			}
		}
		break;
	default:
		sbl_dev_err(sbl->dev, "%d: pcs bad link_mode (%d)", port_num, link->link_mode);
		return -EBADRQC;
	}

	return 0;
}


/*
 * PCS start-up
 */
void sbl_pml_pcs_start(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u32 val32;

	sbl_dev_dbg(sbl->dev, "%d: pcs start", port_num);

	/* we need updated AMs for 200GHz mode */
	val32 = SBL_PML_CFG_PCS_AMS_DFLT;
	if (link->link_mode == SBL_LINK_MODE_BS_200G)
		val32 = SBL_PML_CFG_PCS_AMS_USE_PROGRAMMABLE_AMS_UPDATE(
				val32, 1ULL);
	else
		val32 = SBL_PML_CFG_PCS_AMS_USE_PROGRAMMABLE_AMS_UPDATE(
				val32, 0ULL);
	sbl_write32(sbl, base|SBL_PML_CFG_PCS_AMS_OFFSET, val32);

	/*
	 * start pcs
	 */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	val64 = SBL_PML_CFG_PCS_PCS_ENABLE_UPDATE(val64, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET); /* flush */

	/* record if we are trying to align */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	if (SBL_PML_CFG_RX_PCS_ENABLE_LOCK_GET(val64))
		sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_ALIGN_EN);
	else
		sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_ALIGN_EN);
}


void sbl_pml_pcs_enable_alignment(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: pcs enable alignment", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(val64, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);

	/* alignment process is underway */
	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_ALIGN_EN);
}


void sbl_pml_pcs_disable_alignment(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: pcs disable alignment", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);

	/* alignment process is stopped */
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_ALIGN_EN);
}


/*
 * remote fault.
 *
 * there is a debug reg we can use to send remote fault to our link partner
 */
void sbl_pml_pcs_set_tx_rf(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: pcs set tx rf", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_DBG_PCS_OFFSET);
	val64 = SBL_PML_DBG_PCS_FORCE_TX_DATA_UPDATE(val64, 1ULL);
	val64 = SBL_PML_DBG_PCS_FORCE_TX_DATA_RF_UPDATE(val64, 1ULL);
	sbl_write64(sbl, base|SBL_PML_DBG_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_DBG_PCS_OFFSET);

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_TX_RF);
}


void sbl_pml_pcs_clear_tx_rf(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: pcs clear tx rf", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_DBG_PCS_OFFSET);
	val64 = SBL_PML_DBG_PCS_FORCE_TX_DATA_UPDATE(val64, 0ULL);
	val64 = SBL_PML_DBG_PCS_FORCE_TX_DATA_RF_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_DBG_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_DBG_PCS_OFFSET);

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_TX_RF);
}


/*
 * wait for the pcs to come up
 *
 *   i.e for it to be locked and aligned and faults to have cleared
 *
 *
 */
int sbl_pml_pcs_wait(struct sbl_inst *sbl, int port_num)
{
	char pcs_state_str[SBL_PCS_STATE_STR_LEN];
	int err;

	sbl_dev_dbg(sbl->dev, "%d: pcs wait", port_num);

	/*
	 * start locking
	 */
	sbl_pml_pcs_start_lock(sbl, port_num);

	/*
	 * clear forcing remote fault.
	 * (it will continue to be set until PCS is actually ready)
	 */
	sbl_pml_pcs_clear_tx_rf(sbl, port_num);

	/*
	 * keep trying to bring the pcs up until we timeout
	 */
	while (true) {

		/*
		 * wait for alignment
		 */
		err = sbl_pml_pcs_alignment_wait(sbl, port_num);
		switch (err) {
		case 0:
			break;

		case -ETIMEDOUT:
			/* out of time, dump state and give up  */
			sbl_dev_err(sbl->dev, "%d: pcs_wait alignment timeout (%s)\n", port_num,
					sbl_pml_pcs_state_str(sbl, port_num, pcs_state_str,
							SBL_PCS_STATE_STR_LEN));
			goto out;

		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "%d: pcs_wait alignment failed [%d]\n", port_num, err);
			goto out;

		default:
			/* something unexpected */
			sbl_dev_err(sbl->dev, "%d: pcs_wait alignment failed [%d]\n", port_num, err);
			goto out;
		}

		/*
		 * wait for faults to clear
		 */
		err = sbl_pml_pcs_fault_clear_wait(sbl, port_num);
		switch (err) {

		case 0:
			goto out;

		case -ENOLCK:
			/* alignment lost - try to align again  */
			sbl_dev_warn(sbl->dev, "%d: pcs_wait alignment lost - restart (%s)\n", port_num,
					sbl_pml_pcs_state_str(sbl, port_num, pcs_state_str,
							SBL_PCS_STATE_STR_LEN));
			continue;

		case -ETIMEDOUT:
			sbl_dev_err(sbl->dev, "%d: pcs_wait fault clear timeout (%s)\n", port_num,
					sbl_pml_pcs_state_str(sbl, port_num, pcs_state_str,
							SBL_PCS_STATE_STR_LEN));
			goto out;

		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "%d: pcs_wait fault clear cancelled\n", port_num);
			goto out;

		default:
			/* something unexpected */
			sbl_dev_err(sbl->dev, "%d: pcs_wait fault clear failed [%d]\n", port_num, err);
			goto out;
		}
	}

out:
	/* drop all stage info */
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_ALIGN_EN);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_A_WAIT);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_F_WAIT);

	if (err)
		return err;

	/* clear all the PCS related error flags */
	sbl_pml_err_flgs_clear(sbl, port_num, SBL_PML_ALL_PCS_ERR_FLGS);

	/* note pcs is up */
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_TX_AM);
	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_UP);

	return 0;
}


char *sbl_pml_pcs_state_str(struct sbl_inst *sbl, int port_num, char *buf, int len)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	if (!buf || !len)
		return "";

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);
	snprintf(buf, len, "lk %llx/%.2x, a%lld f%lld lf%lld hs%lld",
			SBL_PML_STS_RX_PCS_AM_LOCK_GET(val64),
			link->active_fec_lanes,
			SBL_PML_STS_RX_PCS_ALIGN_STATUS_GET(val64),
			SBL_PML_STS_RX_PCS_FAULT_GET(val64),
			SBL_PML_STS_RX_PCS_LOCAL_FAULT_GET(val64),
			SBL_PML_STS_RX_PCS_HI_SER_GET(val64));
	return buf;
}


/*
 * wait for the pcs to become aligned
 */
static int sbl_pml_pcs_alignment_wait(struct sbl_inst *sbl, int port_num)
{
	unsigned long start_jiffy;
	int elapsed;
	int err;

	sbl_dev_dbg(sbl->dev, "%d: pml pcs alignment wait\n", port_num);

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_A_WAIT);

	 /*
	  * poll for all lanes to get bitlock
	  *
	  *   This might be a very long time as the other end obviously needs to
	  *   be coming up at the same time. So start eagerly checking and then back off.
	  *
	  *   If we are getting a high serdes error rate or bad eyes then we can never
	  *   meaningfully lock so bail with an error and the serdes will be retuned.
	  *
	  *   Occasionally the pcs seems to get stuck and some lanes never come up.
	  *   Restarting locking seems to clear this.
	  */
restart:
	start_jiffy = jiffies;
	while (!sbl_pml_pcs_locked(sbl, port_num)) {

		if (sbl_start_timeout(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pcs lock wait timeout\n",
					port_num);
			err = -ETIMEDOUT;
			goto out;
		}

		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pcs lock wait cancelled\n",
					port_num);
			err = -ECANCELED;
			goto out;
		}

		elapsed = jiffies_to_msecs(jiffies - start_jiffy);
		if (elapsed > 1000) {

			/* if we have a high serdes error, we will never align  */
			if (sbl_pml_pcs_high_serdes_error(sbl, port_num)) {
				sbl_dev_warn(sbl->dev, "%d: pcs lock high serdes error detected\n", port_num);
				err = -EADV;
				goto out;
			}

			/* Ensure the serdes is still good (eyes stay open)  */
			err = sbl_port_check_eyes(sbl, port_num);
			if (err) {
				sbl_dev_warn(sbl->dev, "%d: pcs lock some eyes have gone bad", port_num);
				goto out;
			}

			/* restart locking in case it's locked up */
			sbl_pml_pcs_stop_lock(sbl, port_num);
			sbl_pml_pcs_start_lock(sbl, port_num);

			msleep(SBL_PML_PCS_ALIGN_SLOW_POLL_DELAY);
		} else if (elapsed > 50)
			msleep(100);
		else
			msleep(10);
	}

	/*
	 * poll for lane alignment
	 */
	start_jiffy = jiffies;
	while (!sbl_pml_pcs_aligned(sbl, port_num)) {

		if (sbl_start_timeout(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pcs align wait timeout\n",
					port_num);
			err = -ETIMEDOUT;
			goto out;
		}

		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pcs align wait cancelled\n",
					port_num);
			err = -ECANCELED;
			goto out;
		}

		/* if we lose lock restart trying to lock again  */
		if (!sbl_pml_pcs_locked(sbl, port_num)) {
			sbl_dev_warn(sbl->dev, "%d: pcs align - lost lock\n", port_num);
			goto restart;
		}

		elapsed = jiffies_to_msecs(jiffies - start_jiffy);
		if (elapsed > SBL_PML_PCS_ALIGN_TIMEOUT) {

			/*
			 * we should have got alignment by now
			 * give up, restart locking and try again
			 */
			sbl_pml_pcs_stop_lock(sbl, port_num);
			sbl_pml_pcs_start_lock(sbl, port_num);
			goto restart;
		} else if (elapsed > 50)
			msleep(100);
		else
			msleep(10);
	}

out:
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_A_WAIT);

	return err;
}


/*
 * enable pcs recovery
 */
void sbl_pml_pcs_recovery_enable(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;

	link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: PCS recovery enable", port_num);

	spin_lock(&link->pcs_recovery_lock);
	link->pcs_recovery_flag = true;
	spin_unlock(&link->pcs_recovery_lock);
}
EXPORT_SYMBOL(sbl_pml_pcs_recovery_enable);


/*
 * disable pcs recovery
 */
void sbl_pml_pcs_recovery_disable(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;

	link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: PCS recovery disable", port_num);

	spin_lock(&link->pcs_recovery_lock);
	link->pcs_recovery_flag = false;
	spin_unlock(&link->pcs_recovery_lock);
}
EXPORT_SYMBOL(sbl_pml_pcs_recovery_disable);


static void sbl_pml_pcs_start_lock(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: pml bring-up start am lock\n", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(val64, link->active_rx_lanes);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
}


static void sbl_pml_pcs_stop_lock(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: pml bring-up stop am lock\n", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_LOCK_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
}


/*
 * wait for pcs faults to clear
 *
 * we wait for fault to stay clear as it is momentarily set sometimes
 */
static int sbl_pml_pcs_fault_clear_wait(struct sbl_inst *sbl, int port_num)
{
	unsigned long start_jiffy = jiffies;
	int no_fault_count = 0;
	int elapsed;
	int err;

	sbl_dev_dbg(sbl->dev, "%d: pml pcs fault clear wait\n", port_num);

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_F_WAIT);

	while (true) {

		/* check for timeout */
		if (sbl_start_timeout(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pml fault clear wait timeout\n",
					port_num);
			err = -ETIMEDOUT;
			goto out;
		}

		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pml fault clear wait cancelled\n",
					port_num);
			err = -ECANCELED;
			goto out;
		}

		/* check we are still aligned */
		if (!sbl_pml_pcs_aligned(sbl, port_num)) {
			sbl_dev_dbg(sbl->dev, "%d: pml fault clear wait lost alignment\n",
					port_num);
			err = -ENOLCK;
			goto out;
		}

		/*
		 * check for no fault
		 * we need multiple good tests to be sure it is up
		 */
		if (sbl_pml_pcs_no_faults(sbl, port_num)) {
			++no_fault_count;
			if (no_fault_count == SBL_PML_REQUIRED_NO_FAULT_COUNT) {
				/* done */
				err = 0;
				goto out;
			} else {
				/* poll fast and check again */
				start_jiffy = jiffies;
			}
		} else
			no_fault_count = 0;

		/* wait with backoff */
		elapsed = jiffies_to_msecs(jiffies - start_jiffy);
		if (elapsed > 5000)
			msleep(1000);
		else if (elapsed > 100)
			msleep(100);
		else
			msleep(10);
	}

out:
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_F_WAIT);

	return err;
}


/*
 * helper state functions
 */
#ifdef CONFIG_SBL_PLATFORM_CAS_SIM
static bool sbl_pml_pcs_locked(struct sbl_inst *sbl, int port_num)
{
	return true;
}
#else
static bool sbl_pml_pcs_locked(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	struct sbl_link *link = sbl->link + port_num;
	u64 locked_lanes;
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);
	locked_lanes = SBL_PML_STS_RX_PCS_AM_LOCK_GET(val64);

	return (locked_lanes == link->active_fec_lanes);
}
#endif /* CONFIG_SBL_PLATFORM_CAS_SIM */


#ifdef CONFIG_SBL_PLATFORM_CAS_SIM
bool sbl_pml_pcs_aligned(struct sbl_inst *sbl, int port_num)
{
	return true;
}
#else
bool sbl_pml_pcs_aligned(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);

	return (SBL_PML_STS_RX_PCS_ALIGN_STATUS_GET(val64) == 1);
}
#endif /* CONFIG_SBL_PLATFORM_CAS_SIM */
EXPORT_SYMBOL(sbl_pml_pcs_aligned);


static bool sbl_pml_pcs_no_faults(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 faults;
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);
	faults = SBL_PML_STS_RX_PCS_FAULT_GET(val64) | SBL_PML_STS_RX_PCS_LOCAL_FAULT_GET(val64);

	return (faults == 0ULL);
}


bool sbl_pml_pcs_high_serdes_error(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);
	return 	SBL_PML_STS_RX_PCS_HI_SER_GET(val64);
}


#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM)
bool sbl_pml_pcs_up(struct sbl_inst *sbl, int port_num)
{
	return true;
}
#else
bool sbl_pml_pcs_up(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);

	return 	SBL_PML_STS_RX_PCS_ALIGN_STATUS_GET(val64) &&
			!SBL_PML_STS_RX_PCS_FAULT_GET(val64) &&
			!SBL_PML_STS_RX_PCS_LOCAL_FAULT_GET(val64) &&
			!SBL_PML_STS_RX_PCS_HI_SER_GET(val64);
}
#endif

bool sbl_pml_recovery_no_faults(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	struct sbl_link *link = sbl->link + port_num;
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_OFFSET);

	sbl_dev_dbg(sbl->dev, "%d:sbl_pml_recovery fault info val=0x%llx aml=0x%llx fecl:0x%x",
		     port_num, val64, SBL_PML_STS_RX_PCS_AM_LOCK_GET(val64), link->active_fec_lanes);

	return 	SBL_PML_STS_RX_PCS_ALIGN_STATUS_GET(val64) &&
			!SBL_PML_STS_RX_PCS_FAULT_GET(val64) &&
			!SBL_PML_STS_RX_PCS_LOCAL_FAULT_GET(val64) &&
			!SBL_PML_STS_RX_PCS_HI_SER_GET(val64) &&
			(SBL_PML_STS_RX_PCS_AM_LOCK_GET(val64) == link->active_fec_lanes);
}

void sbl_pml_recovery_log_pcs_status(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_DESKEW_DEPTHS_OFFSET);
	sbl_dev_info(sbl->dev, "%d: SBL_PML_STS_RX_PCS_DESKEW_DEPTHS_OFFSET = 0x%llx",
			port_num, val64);

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_AM_MATCH_OFFSET);
	sbl_dev_info(sbl->dev, "%d: SBL_PML_STS_RX_PCS_AM_MATCH_OFFSET = 0x%llx",
			port_num, val64);

	val64 = sbl_read64(sbl, base|SBL_PML_STS_RX_PCS_FECL_SOURCES_OFFSET);
	sbl_dev_info(sbl->dev, "%d: SBL_PML_STS_RX_PCS_FECL_SOURCES_OFFSET = 0x%llx",
			port_num, val64);
}

/*
 * stop the pcs
 */
void sbl_pml_pcs_stop(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	struct sbl_link *link = sbl->link + port_num;
	u64 val64;

	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_KEEP_SERDES_UP)) {
		sbl_dev_err(sbl->dev, "%d: KEEP SERDES UP", port_num);
		return;
	}

	sbl_dev_dbg(sbl->dev, "%d: pml pcs stop\n", port_num);

	if ((link->sstate != SBL_SERDES_STATUS_DOWN) &&
		(link->mattr.media == SBL_LINK_MEDIA_OPTICAL)) {
		sbl_dev_err(sbl->dev, "%d: not stopping pcs because serdes is running",
			port_num);
		return;
	}

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);
	val64 = SBL_PML_CFG_PCS_PCS_ENABLE_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_PCS_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET); /* flush */

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_TX_AM);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_ALIGN_EN);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_A_WAIT);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_F_WAIT);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_UP);
}


/*
 * enable/disable pcs sending ordered sets
 */
void sbl_pml_pcs_ordered_sets(struct sbl_inst *sbl,
		int port_num, int enable)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	u64 state = (enable) ? 1ULL : 0ULL;

	sbl_dev_dbg(sbl->dev, "%d: pml pcs ordered sets - %s\n", port_num,
			(enable)?"start":"stop");

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET);
	val64 = SBL_PML_CFG_RX_PCS_ENABLE_CTL_OS_UPDATE(val64, state);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_PCS_OFFSET, val64);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET);
	val64 = SBL_PML_CFG_TX_PCS_ENABLE_CTL_OS_UPDATE(val64, state);
	sbl_write64(sbl, base|SBL_PML_CFG_TX_PCS_OFFSET, val64);

	/* pci flush */
	sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);

	if (enable)
		sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_OS);
	else
		sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_OS);
}


/*
 * Start/Stop sending am.
 *
 *   Start: configures and starts the PCS and then immediately disables the alignment
 *   locking process
 *
 *   Stop: just switched off the PCS
 */
int  sbl_pml_pcs_am_start(struct sbl_inst *sbl, int port_num)
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

	if (!(link->blstate & (SBL_BASE_LINK_STATUS_STARTING|SBL_BASE_LINK_STATUS_STARTING))) {
		sbl_dev_err(sbl->dev, "pcs_am_start %d: wrong state (%s)\n", port_num,
				sbl_link_state_str(link->blstate));
		return -ENAVAIL;
	}

	/*
	 * configuring will stop the pcs
	 * this might perturb link partner when tuning so report we do this
	 */
	if (link->link_info & SBL_LINK_INFO_PCS_TX_AM)
		sbl_dev_dbg(sbl->dev, "%d pcs_am_start: tx am restart\n", port_num);

	err = sbl_pml_pcs_config(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "pcs_config %d: failed [%d]\n", port_num, err);
		return err;
	}

	/* send remote fault to the link partner */
	sbl_pml_pcs_set_tx_rf(sbl, port_num);

	/* start the pcs - which will start it sending AM */
	sbl_pml_pcs_start(sbl, port_num);

	/* if pcs is running it will be sending AMs */
	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_PCS_TX_AM);

	/* but definitely not doing any of this yet */
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_ANEG);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_A_WAIT);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_F_WAIT);
	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_PCS_UP);

	return 0;
}


/*
 * print out pcs state
 *
 */
#ifdef CONFIG_SYSFS
int sbl_pml_pcs_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	char state_str[SBL_PCS_STATE_STR_LEN];
	int s = 0;

	spin_lock(&link->lock);
	if (link->pcs_config) {
		s += snprintf(buf+s, size-s, "pcs:  %s (%d %d %d %d) rx %x %s\n",
				sbl_link_mode_str(link->link_mode),
				sbl->switch_info->ports[port_num].serdes[0].tx_lane_source,
				sbl->switch_info->ports[port_num].serdes[1].tx_lane_source,
				sbl->switch_info->ports[port_num].serdes[2].tx_lane_source,
				sbl->switch_info->ports[port_num].serdes[3].tx_lane_source,
				link->active_rx_lanes,
				sbl_pml_pcs_state_str(sbl, port_num, state_str, SBL_PCS_STATE_STR_LEN));
	}
	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_pml_pcs_sysfs_sprint);

int sbl_pml_pcs_lane_degrade_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;
	u32 base = SBL_PML_BASE(port_num);
	u64 sts_pcs_lane_degrade_reg = sbl_read64(sbl, base|SBL_PML_STS_PCS_LANE_DEGRADE_OFFSET);
	u64 cfg_pcs_reg = sbl_read64(sbl, base|SBL_PML_CFG_PCS_OFFSET);

	spin_lock(&link->lock);
	if (link->pcs_config) {
		s += snprintf(buf+s, size-s, "ALD: %lld - TX: 0x%llx - RX: 0x%llx\n",
				SBL_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_GET(cfg_pcs_reg),
				SBL_PML_STS_PCS_LANE_DEGRADE_LP_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg),
				SBL_PML_STS_PCS_LANE_DEGRADE_RX_PLS_AVAILABLE_GET(sts_pcs_lane_degrade_reg));
	}
	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_pml_pcs_lane_degrade_sysfs_sprint);

#ifndef CONFIG_SBL_PLATFORM_ROS_HW
int sbl_pml_pcs_speed_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);
	if (link->pcs_config)
		s += snprintf(buf+s, size-s, "%s", sbl_link_mode_str(link->link_mode));
	else
		s += snprintf(buf+s, size-s, "NA");
	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_pml_pcs_speed_sysfs_sprint);
#endif
#endif
