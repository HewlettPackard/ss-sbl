/* SPDX-License-Identifier: GPL-2.0 */


/* Copyright 2019-2022 Hewlett Packard Enterprise Development LP */

#ifndef _SBL_LINK_H_
#define _SBL_LINK_H_


/*
 * link mode auto negotiation
 */
int sbl_link_autoneg(struct sbl_inst *sbl, int port_num);

/*
 * state info flags for trace/debug
 */
enum sbl_link_info_flags {
	SBL_LINK_INFO_LP_DET       = (1<<0),  /* trying to detect lp */
	SBL_LINK_INFO_PCS_ANEG     = (1<<1),  /* pcs configured for autoneg */
	SBL_LINK_INFO_PCS_TX_AM    = (1<<2),  /* pcs is sending AM */
	SBL_LINK_INFO_PCS_ALIGN_EN = (1<<3),  /* pcs alignment enabled */
	SBL_LINK_INFO_PCS_A_WAIT   = (1<<4),  /* pcs waiting for alignment */
	SBL_LINK_INFO_PCS_TX_RF    = (1<<5),  /* pcs is sending remote fault */
	SBL_LINK_INFO_PCS_F_WAIT   = (1<<6),  /* pcs waiting for faults to clear */
	SBL_LINK_INFO_PCS_UP       = (1<<7),  /* pcs is up and aligned */
	SBL_LINK_INFO_MAC_OP       = (1<<8),  /* MAC is operational */
	SBL_LINK_INFO_OS           = (1<<9),  /* sending ordered sets */
	SBL_LINK_INFO_LLR_LOOP     = (1<<10), /* llr can measure loop time */
	SBL_LINK_INFO_LLR_DETECT   = (1<<11), /* llr detect */
	SBL_LINK_INFO_LLR_MEASURE  = (1<<12), /* llr measuring loop time */
	SBL_LINK_INFO_LLR_WAIT     = (1<<13), /* waiting for llr to be ready */
	SBL_LINK_INFO_LLR_RUN      = (1<<14), /* llr is operational */
	SBL_LINK_INFO_LLR_DISABLED = (1<<15), /* llr is setup but disabled */
	SBL_LINK_INFO_FAULT_MON    = (1<<16), /* link fault detection is operational */
};

void sbl_link_info_set(struct sbl_inst *sbl, int port_num, u32 flag);
void sbl_link_info_clear(struct sbl_inst *sbl, int port_num, u32 flag);

/*
 * sysfs
 */
#ifdef CONFIG_SYSFS
int sbl_pml_pcs_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_pml_pcs_lane_degrade_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
#endif

/*
 * debug
 */
void sbl_link_lock_dbg(struct sbl_inst *sbl, int port_num, char *text);


#endif /* _SBL_LINK_H_ */
