/* SPDX-License-Identifier: GPL-2.0 */

/*
 * sbl_internal.h
 *
 * Copyright 2019-2023 Hewlett Packard Enterprise Development LP
 *
 *
 *
 */

#ifndef _SBL_INTERNAL_H_
#define _SBL_INTERNAL_H_

#include "linux/completion.h"
#include "uapi/sbl_serdes.h"


#ifdef TRACE2
#define DEV_TRACE2(dev, format, args...) dev_dbg(dev, format, ## args)
#else
#define DEV_TRACE2(dev, format, args...) dev_ignore(dev, format, ## args)
#endif /* TRACE2 */

#ifdef TRACE3
#define DEV_TRACE3(dev, format, args...) dev_dbg(dev, format, ## args)
#else
#define DEV_TRACE3(dev, format, args...) dev_ignore(dev, format, ## args)
#endif /* TRACE3 */

#ifndef fallthrough
#define fallthrough __attribute__((__fallthrough__))
#endif

#define SBL_AN_MAX_TX_PAGES             3

/*
 * Max number of times we can tune using saved params and find params out-of-bounds
 * before we give up and do a full tune without saved params
 */
#define SBL_MAX_TUNE_PARAM_OOB_FAILS   4


/*
 * The amount of time SBL waits before reloading firmware
 * when firmware corruption is detected.
 */
#define SBL_SERDES_STATE_DUMP_DELAY 200 /* ms */

/* approximate time for signal to travel 1m (for copper cables) */
#define SBL_MEDIA_NS_PER_M                              4
/* approximate time for signal to travel 1m (for optical cables) */
#define SBL_MEDIA_OPTICAL_NS_PER_M                      5
/* approximate time for signal to pass through an optical transceiver */
#define SBL_MEDIA_OPTICAL_TRANCEIVER_DELAY             35  /* ns */
/* approximate time for packet to pass from PCS to Serdes */
#define SBL_ASIC_TX_DELAY                              25  /* ns */
#define SBL_ASIC_RX_DELAY                              91  /* ns */

#define SBL_PML_REC_POLL_INTERVAL                       4  /* ms */
#define SBL_PML_REC_LLR_TIMEOUT_OFFSET                  8  /* ms */

struct sbl_pml_recovery
{
	struct sbl_inst *sbl;
	struct timer_list timer;
	bool  started;
	__u32 port_num;
	__u32 timeout;
	__u32 down_origin;
	__u64 init_jiffies;
	__u64 last_poll_jiffies;
	__u64 rl_window_start;
	int rl_time_remaining;
};
/*
 * link database record
 */
struct sbl_link {
	int num;                                  /* link/port number */
	spinlock_t lock;                          /* Data lock */

	struct sbl_media_attr     mattr;          /* physical media properties */
	bool mconfigured;                         /* is the media attr configured */
	struct sbl_base_link_attr blattr;         /* link related configuration */
	bool blconfigured;                        /* is the base-link attr configured */

	u32 sstate;                               /* serdes state */
	u32 serr;                                 /* serdes error number */
	u32 blstate;                              /* base link state */
	u32 blerr;                                /* base link error number */
	u32 link_info;                            /* misc informative bits describing links internal state */

	struct mutex busy_mtx;                    /* held when starting/stopping */

	spinlock_t timeout_lock;                  /* protect timing values */
	unsigned long last_start_jiffy;           /* last jiffy before link start time out */

	u32 start_timeout;                        /* active start timeout */
	ktime_t start_time_begin;                 /* start timestamp (for sysfs) */
	struct timespec64 start_time;             /* time for start to complete (link up time) (for sysfs) */
	ktime_t up_time_begin;                    /* timestamp when an/lpd completed (for sysfs) */
	struct timespec64 up_time;                /* time for link to come up after an/lpd (for sysfs) */
	ktime_t tune_time_begin;                  /* timestamp when serdes started tuning (for sysfs) */
	struct timespec64 tune_time;              /* time for serdes tuning attempt to complete (for sysfs) */
	struct timespec64 total_tune_time;        /* total time for serdes tuning to complete (for sysfs) */

	u32 active_rx_lanes;                      /* pcs rx lanes in use for the current mode */
	u32 active_fec_lanes;                     /* fec lanes in use for the current mode */
	bool pcs_config;                          /* is the pcs hw configured */

	u64 an_tx_page[SBL_AN_MAX_TX_PAGES];      /* pages to send during autoneg */
	int an_tx_count;                          /* count of autoneg pages to send */
	u64 an_rx_page[SBL_AN_MAX_RX_PAGES];      /* pages received during autoneg */
	int an_rx_count;                          /* count of autoneg pages received */
	u32 an_try_count;                         /* number of autoneg attempts */
	u32 an_nonce;                             /* The nonce used for autoneg */
	struct completion an_hw_change;           /* signal an hw err flag has been set */
	bool an_timeout_active;                   /* are we using the autoneg timeout */
	bool an_100cr4_fixup_applied;             /* have applied this fixup */
	u32 an_options;                           /* actual an options received */
	int lp_subtype;                           /* link partner subtype */

	bool reload_serdes_fw;                    /* do we need to reload the serdes fw */
	bool lp_detected;                         /* has link partner been detected */
	int lpd_try_count;                        /* count of lp detect attempts */

	int dfe_tune_count;                       /* track the number of dfe tuning attempts */
	bool optical_delay_active;                /* waiting in dfe-pre-delay before serdes tuning */
	bool dfe_predelay_active;                 /* waiting in dfe-pre-delay before serdes tuning */
	bool pcal_running;                        /* periodic calibration running */
	u32 tune_param_oob_count;                 /* number of consecutive tunes with bad tuning params */
	unsigned long pcal_start_jiffies;         /* time pcal started */
	u32 ical_effort;                          /* serdes tuning effort level */
	struct sbl_tuning_params tuning_params;   /* saved serdes tuning parameters */
	struct mutex tuning_params_mtx;           /* lock tuning params */
	bool start_cancelled;                     /* starting procedure was cancelled */

	u32 link_mode;                            /* actual link mode to use after AN */
	u32 ifg_config;                           /* actual ifg config */
	u32 loopback_mode;                        /* actual loopback mode to use */
	bool precoding_enabled;                   /* Is precoding on */
	u32 llr_mode;                             /* actual llr mode used */
	u32 llr_options;                          /* actual llr options used */
	u64 llr_loop_time;                        /* the measured llr round trip time (ns) */

	u64 intr_err_flgs;                        /* error flags registered with handler */
	struct mutex serdes_mtx;                  /* lock for serdes operations */
	atomic_t debug_config;                    /* debug flags */

	bool       pcs_recovery_flag;             /* PCS recovery flag */
	spinlock_t pcs_recovery_lock;             /* PCS recovery lock */

	struct sbl_pml_recovery pml_recovery;     /* PML recovery fields */

	bool is_degraded;                         /* link is degraded flag */
	spinlock_t is_degraded_lock;              /* link is degraded lock */

	atomic_t *counters;                       /* SBL link counters */

	struct fec_data *fec_data;
	unsigned long fec_discard_time;           /* fec mon discard trigger time */
	int fec_discard_type;                     /* fec mon discard trigger type*/
	spinlock_t fec_discard_lock;              /* fec mon discard trigger lock */
};


/* debug support */
bool sbl_debug_option(struct sbl_inst *sbl, int port_num, u32 flags);
void dev_ignore(struct device *dev, const char *format, ...)
	__attribute__((format(printf, 2, 3)));


/* access the single char dev */
struct device *sbl_get_device(void);


/* serdes */
u32 sbl_serdes_get_config_tag(void);


/* timing and timeouts */
void sbl_link_init_start_timeout(struct sbl_inst *sbl, int port_num);
void sbl_link_update_start_timeout(struct sbl_inst *sbl, int port_num,
		unsigned int timeout_ms);
bool sbl_start_timeout(struct sbl_inst *sbl, int port_num);
void sbl_start_timeout_ensure_remaining(struct sbl_inst *sbl, int port_num,
		unsigned int remaining_s);
u32  sbl_get_start_timeout(struct sbl_inst *sbl, int port_num);
int  sbl_link_start_elapsed(struct sbl_inst *sbl, int port_num);
void sbl_link_start_record_timespec(struct sbl_inst *sbl, int port_num);
void sbl_link_up_begin(struct sbl_inst *sbl, int port_num);
void sbl_link_up_record_timespec(struct sbl_inst *sbl, int port_num);
void sbl_link_tune_begin(struct sbl_inst *sbl, int port_num);
int  sbl_link_tune_elapsed(struct sbl_inst *sbl, int port_num);
void sbl_link_tune_zero_total_timespec(struct sbl_inst *sbl, int port_num);
void sbl_link_tune_update_total_timespec(struct sbl_inst *sbl, int port_num);


/* misc */
int sbl_validate_port_num(struct sbl_inst *sbl, int port_num);
const char *sbl_an_state_str(int state);
const char *sbl_an_get_sm_state(struct sbl_inst *sbl, int port_num);


/*
 * These functions are provides by the calling framework
 *
 *   register access
 */
static inline u32  sbl_read32(struct sbl_inst *sbl, long offset)
{
	return (*sbl->ops.sbl_read32)(sbl->pci_accessor, offset);
}

static inline u64  sbl_read64(struct sbl_inst *sbl, long offset)
{
	return (*sbl->ops.sbl_read64)(sbl->pci_accessor, offset);
}

static inline void sbl_write32(struct sbl_inst *sbl, long offset, u32 val)
{
	(*sbl->ops.sbl_write32)(sbl->pci_accessor, offset, val);
}

static inline void sbl_write64(struct sbl_inst *sbl, long offset, u64 val)
{
	(*sbl->ops.sbl_write64)(sbl->pci_accessor, offset, val);
}

/*
 * sbus access
 */
static inline int sbl_sbus_op(struct sbl_inst *sbl, int ring, u32 req_data,
		u8 data_addr, u8 rx_addr, u8 command,
		u32 *rsp_data, u8 *result_code, u8 *overrun,
		int timeout, unsigned int flags)
{
	return (*sbl->ops.sbl_sbus_op)(sbl->accessor, ring, req_data,
			data_addr, rx_addr, command,
			rsp_data, result_code, overrun,
			timeout, flags);
}

static inline int sbl_sbus_op_reset(struct sbl_inst *sbl, int ring)
{
	return (*sbl->ops.sbl_sbus_op_reset)(sbl->accessor, ring);
}

/*
 * misc other functions
 */
static inline bool sbl_is_fabric_link(struct sbl_inst *sbl, int port_num)
{
	return (*sbl->ops.sbl_is_fabric_link)(sbl->accessor, port_num);
}

static inline int sbl_get_max_frame_size(struct sbl_inst *sbl, int port_num)
{
	return (*sbl->ops.sbl_get_max_frame_size)(sbl->accessor, port_num);
}

static inline void sbl_async_alert(struct sbl_inst *sbl, int port_num, int alert_type,
				   void *alert_data, int size)
{
	(*sbl->ops.sbl_async_alert)(sbl->accessor, port_num, alert_type, alert_data, size);
}

u64 sbl_llr_edge_cap_data_max_get(void);
u64 sbl_llr_edge_cap_seq_max_get(void);
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
u64 sbl_llr_fabric_cap_data_max_get(void);
u64 sbl_llr_fabric_cap_seq_max_get(void);
#endif

/*
 * SBL counter functions
 */
int sbl_link_counters_init(struct sbl_link *link);
void sbl_link_counters_term(struct sbl_link *link);
int sbl_link_counters_incr(struct sbl_inst *sbl, int port_num, u16 counter);

#ifdef CONFIG_SBL_PLATFORM_ROS_HW
#define sbl_dev_err(_dev, _fmt, ...) \
do { \
	dev_err((_dev), (_fmt), ##__VA_ARGS__); \
} while (0)
#define sbl_dev_dbg(_dev, _fmt, ...) \
do { \
	dev_dbg((_dev), (_fmt), ##__VA_ARGS__); \
} while (0)
#define sbl_dev_warn(_dev, _fmt, ...) \
do { \
	dev_warn((_dev), (_fmt), ##__VA_ARGS__); \
} while (0)
#define sbl_dev_info(_dev, _fmt, ...) \
do { \
	dev_info((_dev), (_fmt), ##__VA_ARGS__); \
} while (0)
#define sbl_dev_err_ratelimited(_dev, _fmt, ...) \
do { \
	dev_err_ratelimited((_dev), (_fmt), ##__VA_ARGS__); \
} while (0)
#define sbl_dev_dbg_ratelimited(_dev, _fmt, ...) \
do { \
	dev_dbg_ratelimited((_dev), (_fmt), ##__VA_ARGS__); \
} while (0)
#else
#define sbl_dev_err(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_err((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_err((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_dbg(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_dbg((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_dbg((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_warn(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_warn((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_warn((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_info(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_info((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_info((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_err_ratelimited(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_err_ratelimited((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_err_ratelimited((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_dbg_ratelimited(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_dbg_ratelimited((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_dbg_ratelimited((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#endif

#endif /* _SBL_INTERNAL_H_ */
