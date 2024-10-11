/* SPDX-License-Identifier: GPL-2.0 */

/*
 *
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 *
 *	Fec related functionality
 */

/* FEC up check */
#define SBL_FEC_UP_SETTLE_PERIOD	 250	   /* ms */
#define SBL_FEC_UP_WINDOW		 250	   /* ms */
#define SBL_FEC_UP_COUNT_FABRIC		 4
#define SBL_FEC_UP_COUNT_EDGE		 1
#define SBL_FEC_MON_PERIOD		 1000	   /* 1sec*/
#define SBL_FEC_LLR_TX_REPLAY_THRESH	 100000	   /* llr_tx_replays/s */
#define SBL_PCS_NUM_FECL_CNTRS		 8
#define SBL_MAX_FEC_WARNINGS		 3	   /* number of warnings issued */

struct sbl_pcs_fec_cntrs {
	u64 ccw;				  /* corrected code words */
	u64 ucw;				  /* uncorrected code words */
	u64 llr_tx_replay;			  /* llr_tx_replay_event count */
	u64 fecl[SBL_PCS_NUM_FECL_CNTRS];	  /* lane errors */
	unsigned long time;			  /* either abs or interval */
};



struct sbl_inst;

struct sbl_fec {
	struct sbl_pcs_fec_cntrs *fec_curr_cnts;   /* current fec counters */
	struct sbl_pcs_fec_cntrs *fec_prev_cnts;   /* previous fec counts */
	struct sbl_pcs_fec_cntrs *fec_rates;	   /* current fec rates */
	struct sbl_pcs_fec_cntrs fec_cntrs[2];
	spinlock_t fec_cnt_lock;		   /* locks above pointers */

	u64 fec_ucw_thresh;			   /* uncorrected codewords link up threshold */
	u32 fec_ucw_up_thresh_adj;		   /* debug: percentage adjustment for link up threshold */
	u32 fec_ucw_down_thresh_adj;		   /* percentage adjustment for link down threshold */
	u32 fec_ucw_hwm;			   /* highest value measured */

	u64 fec_ccw_thresh;			   /* corrected codewords link up threshold */
	u32 fec_ccw_up_thresh_adj;		   /* debug: percentage adjustment for link up threshold */
	u32 fec_ccw_down_thresh_adj;		   /* percentage adjustment for link down threshold */
	u32 fec_ccw_hwm;			   /* highest value measured */

	u64 fec_llr_tx_replay_thresh;		   /* LLR TX Replay threshold */
	u32 fec_llr_tx_replay_hwm;		   /* highest value measured */
	u32 fecl_warn;				   /* corrected codewords per fec lane warning threshold */
	spinlock_t fec_cw_lock;			   /* lock above thresholds */
};


/**
 * @brief link monitor flags for setting fec thresholds
 *
 *	  Can be used in place of explicit threshold values
 *
 */

enum sbl_link_fec_flags {
	SBL_LINK_FEC_INVALID	=  0,	/**< invalid */
	SBL_LINK_FEC_OFF	= -1,	/**< disable monitoring */
	SBL_LINK_FEC_IEEE	= -2,	/**< Use IEEE value for link mode and media */
	SBL_LINK_FEC_HPE	= -3,	/**< Use HPE value for link mode and media */
	SBL_LINK_FEC_DEFAULT	= SBL_LINK_FEC_HPE,
};

void sbl_fec_thresholds_clear(struct sbl_inst *sbl, int port_num);
int sbl_fec_adjustments_set(struct sbl_inst *sbl, int port_num,
		u32 ucw_adj, u32 ccw_adj);

int sbl_fec_thresholds_set(struct sbl_inst *sbl, int port_num,
		s32 ucw_in, s32 ccw_in);

void sbl_fec_counts_get(struct sbl_inst *sbl, int port_num, struct sbl_pcs_fec_cntrs *cntrs);
int sbl_fec_up_check(struct sbl_inst *sbl, int port_num);
void sbl_zero_all_fec_counts(struct sbl_inst *sbl, int port_num);
void sbl_fec_timer(struct timer_list *timer);
void sbl_fec_hwms_clear(struct sbl_inst *sbl, int port_num);
void sbl_fec_dump(struct sbl_inst *sbl, int port_num);
void sbl_fec_modify_adjustments(struct sbl_inst *sbl, int port_num,
		u32 *ucw_up_adj, u32 *ccw_up_adj, u32 *ucw_down_adj, u32 *ccw_down_adj);
int sbl_fec_txr_rate_set(struct sbl_inst *sbl, int port_num,
		u32 txr_rate);
void sbl_fec_timer_work(struct work_struct *work);
