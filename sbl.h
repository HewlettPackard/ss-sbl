/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2019-2025 Hewlett Packard Enterprise Development LP */

#ifndef _SBL_H_
#define _SBL_H_

#include "linux/types.h"
#include <linux/mutex.h>
#include <linux/firmware.h>

#include "uapi/sbl.h"
#include "uapi/sbl_counters.h"

#define SBL_VERSION_MAJOR		      3
#define SBL_VERSION_MINOR		     22
#define SBL_VERSION_INC			      8

#define SBL_MAGIC		     0x6273696c  /* sbli */
#define SBL_INIT_ATTR_MAGIC	     0x62736965  /* sbla */

#define SBL_AN_MAX_RX_PAGES		     20
#define SBL_DFE_USED_SAVED_PARAMS	     -2

/* PML clock */
#ifdef CONFIG_SBL_PLATFORM_ROS
#define SBL_CLOCK_FREQ_MHZ		   850
#else
#define SBL_CLOCK_FREQ_MHZ		  1000
#endif

/* min size of buffer for pcs state string */
#define SBL_PCS_STATE_STR_LEN		     64

/* min size of buffer for base link state string */
#define SBL_BASE_LINK_STATE_STR_LEN	     (SBL_PCS_STATE_STR_LEN + 16)


struct sbl_tuning_params;
struct sbl_sc_values;
struct sbl_serdes_config;


/*
 * Reasons for taking the link down asynchronously
 */
enum sbl_link_down_origin {
	SBL_LINK_DOWN_ORIGIN_UNKNOWN = 0,
	SBL_LINK_DOWN_ORIGIN_LINK_DOWN,
	SBL_LINK_DOWN_ORIGIN_LOCAL_FAULT,
	SBL_LINK_DOWN_ORIGIN_REMOTE_FAULT,
	SBL_LINK_DOWN_ORIGIN_ALIGN,
	SBL_LINK_DOWN_ORIGIN_HISER,
	SBL_LINK_DOWN_ORIGIN_LLR_MAX,
	SBL_LINK_DOWN_ORIGIN_DEGRADE_FAILURE,
	SBL_LINK_DOWN_ORIGIN_UCW,		/* FEC - high uncorrected fec error rate */
	SBL_LINK_DOWN_ORIGIN_CCW,		/* FEC - high corrected fec error rate */
	SBL_LINK_DOWN_ORIGIN_LLR_TX_REPLAY,	/* FEC - high llr_tx_replay fec error rate */
};


/*
 * Internal llr state machine states
 * (missing from pml header)
 */
enum sbl_pml_llr_state_e {
	SBL_PML_LLR_STATE_UNKNOWN = 0,
	SBL_PML_LLR_STATE_OFF,
	SBL_PML_LLR_STATE_INIT,
	SBL_PML_LLR_STATE_ADVANCE,
	SBL_PML_LLR_STATE_HALT,
	SBL_PML_LLR_STATE_REPLAY,
	SBL_PML_LLR_STATE_DISCARD,
	SBL_PML_LLR_STATE_MONITOR,
};


/**
 * @brief Inter-frame gap config
 */
enum sbl_ifg_mode {
	SBL_IFG_MODE_INVALID = 0,
	SBL_IFG_MODE_UNKNOWN,
	SBL_IFG_MODE_HPC,	  /**< Fabric link  */
	SBL_IFG_MODE_IEEE,	  /**< Use IEEE standard */
};


/**
 * @brief IEEE inter-frame gap adjustment
 */
enum sbl_ifg_ieee_adjustment {
	SBL_IFG_IEEE_ADJUSTMENT_INVALID = 0,
	SBL_IFG_IEEE_ADJUSTMENT_UNKNOWN,
	SBL_IFG_IEEE_ADJUSTMENT_200G,	      /**< IEEE 200G adjustment */
	SBL_IFG_IEEE_ADJUSTMENT_100G,	      /**< IEEE 100G adjustment */
	SBL_IFG_IEEE_ADJUSTMENT_50G,	      /**< IEEE 50G adjustment */
	SBL_IFG_IEEE_ADJUSTMENT_NONE,	      /**< No adjustment */
};


/**
 * @brief Async alert types
 */
enum sbl_async_alert_type {
	SBL_ASYNC_ALERT_INVALID		     = 0, /**< Invalid alert */
	SBL_ASYNC_ALERT_LINK_DOWN	     = 1, /**< Link down alert */
	SBL_ASYNC_ALERT_SERDES_FW_CORRUPTION = 2, /**< SerDes firmware corruption alert */
	SBL_ASYNC_ALERT_TX_DEGRADE	     = 3, /**< TX lane degrade alert */
	SBL_ASYNC_ALERT_RX_DEGRADE	     = 4, /**< RX lane degrade alert */
	SBL_ASYNC_ALERT_TX_DEGRADE_FAILURE   = 5, /**< TX lane degrade failure alert */
	SBL_ASYNC_ALERT_RX_DEGRADE_FAILURE   = 6, /**< RX lane degrade failure alert */
	SBL_ASYNC_ALERT_SBM_FW_LOAD_FAILURE  = 7, /**< SBus master fw load failure alert */
};


/**
 * @brief event triggers to discard fec window
 */
enum sbl_fec_discard_type {
	SBL_FEC_DISCARD_TYPE_INVALID = 0,	/**< Invalid discard reason */
	SBL_FEC_DISCARD_TYPE_RX_DEGRADE,	/**< RX lane degraded */
	SBL_FEC_DISCARD_TYPE_PML_REC_START,	/**< PML recovery started */
	SBL_FEC_DISCARD_TYPE_PML_REC_END,	/**< PML recovery ended */
};


/**
 * @brief Link Partner Type
 */
enum sbl_lp_subtype {
	SBL_LP_SUBTYPE_INVALID	  = -1,    /**< Invalid - uninitialized		*/
	SBL_LP_SUBTYPE_UNKNOWN	  = 0,	   /**< Link is up, but type is Unknown */
	SBL_LP_SUBTYPE_CASSINI_V1 = 1,	   /**< Type is Cassini V1		*/
	SBL_LP_SUBTYPE_CASSINI_V2 = 2	   /**< Type is Cassini V2		*/
};


/*
 * Operations provided by the calling framework
 */
struct sbl_ops {
	/* register access */
	u32  (*sbl_read32)(void *pci_accessor, long offset);
	u64  (*sbl_read64)(void *pci_accessor, long offset);
	void (*sbl_write32)(void *pci_accessor, long offset, u32 val);
	void (*sbl_write64)(void *pci_accessor, long offset, u64 val);

	/* sbus access */
	int (*sbl_sbus_op)(void *accessor, int ring, u32 req_data,
			u8 data_addr, u8 rx_addr, u8 command,
			u32 *rsp_data, u8 *result_code, u8 *overrun,
			int timeout, unsigned int flags);
	int (*sbl_sbus_op_reset)(void *accessor, int ring);

	/* external state */
	bool (*sbl_is_fabric_link)(void *accessor, int port_num);
	int  (*sbl_get_max_frame_size)(void *accessor, int port_num);

	/* pml block intr support */
	int  (*sbl_pml_install_intr_handler)(void *accessor, int port_num, u64 err_flags);
	int  (*sbl_pml_enable_intr_handler)(void *accessor, int port_num, u64 err_flags);
	int  (*sbl_pml_disable_intr_handler)(void *accessor, int port_num, u64 err_flags);
	int  (*sbl_pml_remove_intr_handler)(void *accessor, int port_num, u64 err_flags);

	/* async alert */
	void (*sbl_async_alert)(void *accessor, int port_num, int alert_type, void *alert_data, int size);
};

/*
 * Configuration passing into SBL init
 */
struct sbl_init_attr {
	u32 magic;
#ifdef CONFIG_SBL_PLATFORM_ROS_HW
#else /* CASSINI */
	u32 uc_nic;
	u32 uc_platform;
#endif /* CASSINI */
};

struct lane_degrade {
	u64 tx;
	u64 rx;
};

struct fec_data {
	struct sbl_fec *fec_prmts;	/* fec parameters */
	struct timer_list fec_timer;	/* Fec timer */
	struct sbl_inst *sbl;
	int port_num;
	struct work_struct fec_timer_work;
};

/*
 * A slingshot base link device instance
 */
struct sbl_inst {
	int magic;
	int id;
	struct device *dev;			 /* shared linux device */

	void *accessor;				 /* accessor for calling framework */
	void *pci_accessor;			 /* accessor for pci io */
	struct sbl_ops ops;			 /* table of external operations provided by caller */
	struct sbl_instance_attr iattr;		 /* instance attributes */
	struct sbl_switch_info *switch_info;

	u32 sbus_op_flags;			 /* active sbus op flags */

	struct list_head serdes_config_list;	 /* list of serdes configurations */
	spinlock_t serdes_config_lock;		 /* lock serdes configurations list */

	struct sbl_link *link;			 /* link database */

	struct mutex *sbus_ring_mtx;		 /* locks for sbus critical section management */

	#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
		#define SBUS_RING_MTX(sbl, ring) ((sbl)->sbus_ring_mtx + (ring))
	#else
		#define SBUS_RING_MTX(sbl, ring) ((sbl)->sbus_ring_mtx)
	#endif

	struct mutex *sbm_fw_mtx;		 /* locks for sbus master firmware load */
	#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
		#define SBM_FW_MTX(sbl, ring) ((sbl)->sbm_fw_mtx + (ring))
	#else
		#define SBM_FW_MTX(sbl, ring) ((sbl)->sbm_fw_mtx)
	#endif

	bool *reload_sbm_fw;			 /* do we need to reload the sbm fw for each ring */

	atomic_t *sbm_fw_reload_count;		 /* counter to track sbus master fw reload */

	struct workqueue_struct *workq;
};

static inline int sbl_validate_instance(struct sbl_inst *sbl)
{
	if (!sbl)
		return -EINVAL;
	else if (sbl->magic != SBL_MAGIC)
		return -EINVAL;
	else
		return 0;
}

void sbl_get_version(int *major, int *minor, int *inc);

int sbl_get_num_sbus_rings(struct sbl_inst *sbl);

/*
 * Instance management
 */
struct sbl_inst *sbl_new_instance(void *accessor, void *pci_accessor,
				  const struct sbl_ops *ops,
				  struct sbl_init_attr *init_attr);
#ifndef CONFIG_SBL_PLATFORM_ROS_HW
void   sbl_set_eth_name(struct sbl_inst *sbl, const char *name);
#endif
int    sbl_delete_instance(struct sbl_inst *sbl);
int    sbl_initialise_instance(struct sbl_inst *sbl, struct sbl_instance_attr *attr);
int    sbl_restore_instance(struct sbl_inst *sbl);

/*
 * media management
 */
int  sbl_media_config(struct sbl_inst *sbl, int port_num,
		struct sbl_media_attr *attr);
int  sbl_media_unconfig(struct sbl_inst *sbl, int port_num);
bool sbl_media_check_mode_supported(struct sbl_inst *sbl, int port_num,
		u32 mode);
bool sbl_media_check_headshell_reset_supported(struct sbl_inst *sbl, int port_num);
int  sbl_media_validate_config(struct sbl_inst *sbl, int port_num);
int  sbl_media_calc_loop_time(struct sbl_inst *sbl, int port_num, u64 *min_loop_time);
u64  sbl_media_get_loop_time_ns(struct sbl_inst *sbl, int port_num,
		bool require_measurement);
u32 sbl_media_get_len_cm(struct sbl_inst *sbl, int port_num, u64 len);

/*
 * Base link control
 */
int  sbl_get_lp_subtype(struct sbl_inst *sbl, int port_num, enum sbl_lp_subtype *lp_subtype);
int  sbl_base_link_config(struct sbl_inst *sbl, int port_num,
		struct sbl_base_link_attr *blattr);
int  sbl_base_link_start(struct sbl_inst *sbl, int port_num);
int  sbl_base_link_enable_start(struct sbl_inst *sbl, int port_num);
int  sbl_base_link_cancel_start(struct sbl_inst *sbl, int port_num);
bool sbl_base_link_start_cancelled(struct sbl_inst *sbl, int port_num);
int  sbl_base_link_stop(struct sbl_inst *sbl, int port_num);
int  sbl_base_link_reset(struct sbl_inst *sbl, int port_num);
int  sbl_base_link_get_status(struct sbl_inst *sbl, int port_num,
		int *blstate, int *blerr, int *sstate, int *serr,
		int *media_type, int *link_mode);
void sbl_base_link_try_start_fail_cleanup(struct sbl_inst *sbl,
		int port_num);
char *sbl_base_link_state_str(struct sbl_inst *sbl, int port_num, char *buf, int len);
int  sbl_base_link_dump_attr(struct sbl_inst *sbl, int port_num,
		char *buf, size_t size);

/*
 * fec thresholds
 */
#ifndef CONFIG_SBL_PLATFORM_ROS_HW
int  sbl_link_get_fec_thresholds(struct sbl_inst *sbl, int port_num,
		int *ucw_bad, int *ccw_bad, int *fecl_warn);
#endif
u64  sbl_link_get_ucw_thresh_ieee(struct sbl_inst *sbl, int port_num);
u64  sbl_link_get_ucw_thresh_hpe(struct sbl_inst *sbl, int port_num);
u64  sbl_link_get_ccw_thresh_ieee(struct sbl_inst *sbl, int port_num);
u64  sbl_link_get_ccw_thresh_hpe(struct sbl_inst *sbl, int port_num);
u64  sbl_link_get_stp_ccw_thresh_ieee(struct sbl_inst *sbl, int port_num);
u64  sbl_link_get_stp_ccw_thresh_hpe(struct sbl_inst *sbl, int port_num);

/*
 * serdes tuning params
 */
int sbl_serdes_get_tuning_params(struct sbl_inst *sbl, int port_num,
		struct sbl_tuning_params *tuning_params);
int sbl_serdes_set_tuning_params(struct sbl_inst *sbl, int port_num,
		struct sbl_tuning_params *tuning_params);
int sbl_serdes_invalidate_tuning_params(struct sbl_inst *sbl, int port_num);
int sbl_serdes_invalidate_all_tuning_params(struct sbl_inst *sbl);

/* Timing flags */
int sbl_flags_get_poll_interval_from_flags(unsigned int flags);
int sbl_flags_get_delay_from_flags(unsigned int flags);

/*
 * serdes configurations
 */
int  sbl_serdes_add_config(struct sbl_inst *sbl, u64 tp_state_mask0,
		u64 tp_state_mask1, u64 tp_state_match0,
		u64 tp_state_match1, u64 port_mask, u8 serdes_mask,
		struct sbl_sc_values *vals, bool is_default);
int  sbl_serdes_del_config(struct sbl_inst *sbl, u64 tp_state_mask0,
		u64 tp_state_mask1, u64 tp_state_match0, u64 tp_state_match1,
		u64 port_mask, u8  serdes_mask);
int sbl_serdes_clear_all_configs(struct sbl_inst *sbl, bool clr_default);
void sbl_serdes_dump_configs(struct sbl_inst *sbl);
struct sbl_switch_info *sbl_get_switch_info(int *size);

/*
 * pml block intr handler
 */
int sbl_pml_hdlr(struct sbl_inst *sbl, int port_num, void *data);
void sbl_pml_recovery_cancel(struct sbl_inst *sbl, int port_num);

/*
 * serdes operations (using core interrupt mechanism)
 */
int sbl_pml_serdes_op_timing(struct sbl_inst *sbl, int port_num, u64 capture,
		u64 clear, u64 set);
int sbl_pml_serdes_op(struct sbl_inst *sbl, int port_num, u64 serdes_sel,
		u64 op, u64 data, u16 *result, int timeout, unsigned int flags);

/*
 * MAC
 */
void sbl_pml_mac_start(struct sbl_inst *sbl, int port_num);
void sbl_pml_mac_stop(struct sbl_inst *sbl, int port_num);

/*
 * misc
 */
void sbl_pml_llr_stop(struct sbl_inst *sbl, int port_num);
void sbl_pml_llr_disable(struct sbl_inst *sbl, int port_num);
void sbl_pml_llr_enable(struct sbl_inst *sbl, int port_num);
bool sbl_pml_llr_check_is_ready(struct sbl_inst *sbl, int port_num, unsigned int timeout_ms);
u32  sbl_pml_llr_get_state(struct sbl_inst *sbl, int port_num);
int  sbl_get_an_pages(struct sbl_inst *sbl, int port_num, int *count, u64 *pages);
bool sbl_pml_pcs_aligned(struct sbl_inst *sbl, int port_num);
void sbl_pml_pcs_recovery_enable(struct sbl_inst *sbl, int port_num);
void sbl_pml_pcs_recovery_disable(struct sbl_inst *sbl, int port_num);
void sbl_ignore_save_tuning_param(struct sbl_inst *sbl, int port_num, bool ignore);
void sbl_enable_opt_lane_degrade(struct sbl_inst *sbl, int port_num, bool enable);
void sbl_disable_pml_recovery(struct sbl_inst *sbl, int port_num, bool disable);
void sbl_pml_recovery_log_link_down(struct sbl_inst *sbl, int port_num);
void sbl_set_degraded_flag(struct sbl_inst *sbl, int port_num);
void sbl_clear_degraded_flag(struct sbl_inst *sbl, int port_num);
bool sbl_get_degraded_flag(struct sbl_inst *sbl, int port_num);

/*
 * sysfs support
 */
#ifdef CONFIG_SYSFS
int sbl_media_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_media_type_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_serdes_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_serdes_fw_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_pml_pcs_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_pml_pcs_lane_degrade_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_pml_pcs_speed_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_base_link_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_base_link_llr_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_base_link_loopback_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_debug_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
int sbl_sbm_fw_sysfs_sprint(struct sbl_inst *sbl, int ring, char *buf, size_t size);
int sbl_fec_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size);
#endif

/*
 * debug support
 */
void sbl_debug_clear_config(struct sbl_inst *sbl, int port_num);
void sbl_debug_update_config(struct sbl_inst *sbl, int port_num,
		u32 clear_flags, u32 set_flags);
int sbl_debug_get_config(struct sbl_inst *sbl, int port_num, u32 *options);

/*
 * pretty printing
 */
const char *sbl_link_state_str(enum sbl_base_link_status state);
const char *sbl_link_len_str(enum sbl_link_len len);
const char *sbl_link_vendor_str(enum sbl_link_vendor vendor);
const char *sbl_link_media_str(enum sbl_link_media media);
const char *sbl_an_mode_str(enum sbl_an_mode mode);
const char *sbl_link_mode_str(enum sbl_link_mode mode);
const char *sbl_rs_mode_str(enum sbl_rs_mode mode);
const char *sbl_ifg_mode_str(enum sbl_ifg_mode mode);
const char *sbl_ifg_config_str(enum sbl_ifg_config config);
const char *sbl_llr_mode_str(enum sbl_llr_mode mode);
const char *sbl_llr_down_behaviour_str(enum sbl_llr_down_behaviour behaviour);
const char *sbl_loopback_mode_str(enum sbl_loopback_mode state);
const char *sbl_serdes_effort_str(u32 effort);
const char *sbl_serdes_state_str(enum sbl_serdes_status state);
const char *sbl_async_alert_str(enum sbl_async_alert_type alert_type);
const char *sbl_fec_discard_str(enum sbl_fec_discard_type discard_type);
const char *sbl_down_origin_str(enum sbl_link_down_origin down_origin);

/*
 * SBL counter get functions
 */
int sbl_link_counters_get(struct sbl_inst *sbl, int port_num,
						int *counters, u16 first, u16 count);
int sbl_link_counters_read(struct sbl_inst *sbl, int port_num, u16 counter);

#endif /* _SBL_H_ */
