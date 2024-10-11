/* SPDX-License-Identifier: GPL-2.0 */

/**
 * @file uapi/sbl.h
 *
 * Copyright 2019-2024 Hewlett Packard Enterprise Development LP
 *
 * @brief Base-link configuration
 *
 */

#ifndef _SBL_UAPI_BASE_LINK_H_
#define _SBL_UAPI_BASE_LINK_H_

#include "linux/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SBL_INSTANCE_ATTR_MAGIC           0x69736d61  /* siam */
#define SBL_MEDIA_ATTR_MAGIC              0x6d736d61  /* smam */
#define SBL_LINK_ATTR_MAGIC               0x6c736d61  /* slam */


#define SBL_KNOWN_FW0_REV                           0x109E
#define SBL_KNOWN_FW0_BUILD                         0x208D
#define SBL_FW_DEFAULT_SBM_FNAME                    "sbus_master.0x1021_2001.rom.bin"
#define SBL_FW_DEFAULT_SERDES_FNAME                 "serdes.0x109E_208D.rom.bin"
#define SBL_FW_NAME_LEN                           64


/**
 * @brief Various defaults for link startup
 *
 */
#define SBL_LINK_START_TIMEOUT_PEC                48
	/* Time out for Electrical links using AN = (1.2 * SBL_DFLT_DFE_TIMEOUT_PEC) as they are synchronized by AN */
#define SBL_LINK_START_TIMEOUT_AOC                88
	/* Time out for Electrical without AN/optical links = (2.2 * SBL_DFLT_DFE_TIMEOUT_AOC) to ensure a full overlap */
#define SBL_LINK_DFLT_AN_RETRY_TIMEOUT             5
#define SBL_LINK_DFLT_AN_MAX_RETRY                 5
#define SBL_LINK_ALL_LANES                       0xf


/**
 * @brief Autoneg timeouts in ms
 *
 *   The link options SBL_OPT_AN_TIMEOUT_SSHOT
 *   and  SBL_OPT_AN_TIMEOUT_IEEE are used to
 *   select which set are used
 *
 *   TODO Check IEEE values
 */
#define SBL_LINK_AUTONEG_TIMEOUT_SSHOT_200         22000
#define SBL_LINK_AUTONEG_TIMEOUT_SSHOT_100         10000
#define SBL_LINK_AUTONEG_TIMEOUT_SSHOT_50           8000

#define SBL_LINK_AUTONEG_TIMEOUT_IEEE_200           4000
#define SBL_LINK_AUTONEG_TIMEOUT_IEEE_100           2250
#define SBL_LINK_AUTONEG_TIMEOUT_IEEE_50            1125

#define SBL_DFLT_PML_REC_TIMEOUT                       60  /* 60ms */
#define SBL_DFLT_PML_REC_RL_MAX_DURATION               60  /* 60ms */
#define SBL_DFLT_PML_REC_RL_WINDOW_SIZE              1000  /* 1000ms */

/**
 * @brief timing flags for s-bus operations
 *
 *   Delays are the time to pause before issuing first operation
 *   and intervals are the time to wait between operations
 *
 *   Values less than 1ms will use a busy wait, values 1ms and greater
 *   will sleep
 */
enum sbl_serdes_sbus_op_flag
{
	SBL_FLAG_DELAY_3US       =  1<<0,  /**< 3us delay */
	SBL_FLAG_DELAY_4US       =  1<<1,  /**< 4us delay */
	SBL_FLAG_DELAY_5US       =  1<<2,  /**< 5us delay */
	SBL_FLAG_DELAY_10US      =  1<<3,  /**< 10U delay */
	SBL_FLAG_DELAY_20US      =  1<<4,  /**< 20us delay */
	SBL_FLAG_DELAY_50US      =  1<<5,  /**< 50 us delay */
	SBL_FLAG_DELAY_100US     =  1<<6,  /**< 100us delay */
	SBL_FLAG_INTERVAL_1MS    =  1<<7,  /**< 1ms interval */
	SBL_FLAG_INTERVAL_10MS   =  1<<8,  /**< 10ms interval */
	SBL_FLAG_INTERVAL_100MS  =  1<<9,  /**< 100ms interval */
	SBL_FLAG_INTERVAL_1S     =  1<<10, /**< 1s interval */
};

#include "uapi/sbl_serdes_defaults.h"


/**
 * @brief instance attributes
 */
struct sbl_instance_attr
{
	__u32 magic;                           /**< = SBL_INSTANCE_ATTR_MAGIC */

	char  inst_name[10];                   /**< instance name  */
	char  eth_if_name[10];                 /**< interface name */

	char  *sbm_fw_fname;                   /**< sbus master fw file name */
	char  *serdes_fw_fname;                /**< serdes fw file name */

	__u32 rx_phase_slip_cnt;               /**< phase slip */

	__u32 sbus_op_flags_slow;              /**< flags for sbus operations */
	__u32 sbus_op_flags_fast;              /**< flags for sbus operations after sbus speed-up*/
	__u32 sbus_op_timeout_ms;
	__u32 sbus_int_timeout;                /**< Sbus interrupt timeout (s) */
	__u32 sbus_int_poll_interval;          /**< Sbus interrupt polling interval (ms) */
	__u32 serdes_op_flags;                 /**< flags for serdes operations */
	__u32 serdes_op_timeout_ms;
	__u32 serdes_en_timeout;               /* SerDes enable timeout (s) */
	__u32 serdes_en_poll_interval;         /* SerDes enable done polling interval (ms) */
	__u32 core_status_rd_timeout;          /* o_core_status read timeout (s) */
	__u32 core_status_rd_poll_interval;    /* o_core_status read polling interval (ms) */
};


/**
 * @brief Default static instance attributes initialiser
 */
#define SBL_INSTANCE_ATTR_INITIALIZER \
	{ \
		.magic = SBL_INSTANCE_ATTR_MAGIC, \
		.sbm_fw_fname = SBL_FW_DEFAULT_SBM_FNAME, \
		.serdes_fw_fname = SBL_FW_DEFAULT_SERDES_FNAME, \
		.inst_name = {0}, \
		.eth_if_name = {0}, \
		.rx_phase_slip_cnt = SBL_DFLT_RX_PHASE_SLIP_CNT, \
		.sbus_op_flags_slow = SBL_DFLT_SBUS_OP_FLAGS_SLOW, \
		.sbus_op_flags_fast = SBL_DFLT_SBUS_OP_FLAGS_FAST, \
		.sbus_op_timeout_ms = SBL_DFLT_SBUS_OP_TIMEOUT_MS, \
		.sbus_int_timeout = SBL_DFLT_SBUS_INT_TIMEOUT, \
		.sbus_int_poll_interval = SBL_DFLT_SBUS_INT_POLL_INTERVAL, \
		.serdes_op_flags = SBL_DFLT_SERDES_OP_FLAGS, \
		.serdes_op_timeout_ms = SBL_DFLT_SERDES_OP_TIMEOUT_MS, \
		.serdes_en_timeout = SBL_DFLT_SERDES_EN_TIMEOUT, \
		.serdes_en_poll_interval = SBL_DFLT_SERDES_EN_POLL_INTERVAL, \
		.core_status_rd_timeout = SBL_DFLT_CORE_STATUS_RD_TIMEOUT, \
		.core_status_rd_poll_interval = SBL_DFLT_CORE_STATUS_RD_POLL_INTERVAL, \
	}

/**
 * @brief Default static instance attributes initialiser
 */
static inline void sbl_instance_attr_initializer(struct sbl_instance_attr *attr)
{
	attr->magic = SBL_INSTANCE_ATTR_MAGIC;
	attr->sbm_fw_fname = SBL_FW_DEFAULT_SBM_FNAME;
	attr->serdes_fw_fname = SBL_FW_DEFAULT_SERDES_FNAME;
	attr->rx_phase_slip_cnt = SBL_DFLT_RX_PHASE_SLIP_CNT;
	attr->sbus_op_flags_slow = SBL_DFLT_SBUS_OP_FLAGS_SLOW;
	attr->sbus_op_flags_fast = SBL_DFLT_SBUS_OP_FLAGS_FAST;
	attr->sbus_op_timeout_ms = SBL_DFLT_SBUS_OP_TIMEOUT_MS;
	attr->sbus_int_timeout = SBL_DFLT_SBUS_INT_TIMEOUT;
	attr->sbus_int_poll_interval = SBL_DFLT_SBUS_INT_POLL_INTERVAL;
	attr->serdes_op_flags = SBL_DFLT_SERDES_OP_FLAGS;
	attr->serdes_op_timeout_ms = SBL_DFLT_SERDES_OP_TIMEOUT_MS;
	attr->serdes_en_timeout = SBL_DFLT_SERDES_EN_TIMEOUT;
	attr->serdes_en_poll_interval = SBL_DFLT_SERDES_EN_POLL_INTERVAL;
	attr->core_status_rd_timeout = SBL_DFLT_CORE_STATUS_RD_TIMEOUT;
	attr->core_status_rd_poll_interval = SBL_DFLT_CORE_STATUS_RD_POLL_INTERVAL;
}


/**
 * @brief The physical type of the link media
 */
enum sbl_link_media
{
	SBL_LINK_MEDIA_INVALID = 0,
	SBL_LINK_MEDIA_UNKNOWN,
	SBL_LINK_MEDIA_ELECTRICAL,  /**< physical wires */
	SBL_LINK_MEDIA_OPTICAL,     /**< optical fiber  */
};


/**
 * @brief The physical type of the link media with digital/analog
 */
enum sbl_link_media_extended
{
	SBL_EXT_LINK_MEDIA_ELECTRICAL      = 1<<0, /**< physical wires        */
	SBL_EXT_LINK_MEDIA_OPTICAL_ANALOG  = 1<<1, /**< analog optical fiber  */
	SBL_EXT_LINK_MEDIA_OPTICAL_DIGITAL = 1<<2, /**< digital optical fiber */
	SBL_EXT_LINK_MEDIA_ELECTRICAL_ACT  = 1<<4, /**< active electrical     */
};


/**
 * @brief The cable vendor
 */
enum sbl_link_vendor
{
	SBL_LINK_VENDOR_INVALID        = 0,
	SBL_LINK_VENDOR_TE             = 1<<0,
	SBL_LINK_VENDOR_LEONI          = 1<<1,
	SBL_LINK_VENDOR_MOLEX          = 1<<2,
	SBL_LINK_VENDOR_HISENSE        = 1<<3,
	SBL_LINK_VENDOR_DUST_PHOTONICS = 1<<4,
	SBL_LINK_VENDOR_FINISAR        = 1<<5,
	SBL_LINK_VENDOR_LUXSHARE       = 1<<6,
	SBL_LINK_VENDOR_FIT            = 1<<7,
	SBL_LINK_VENDOR_FT             = 1<<8,
	SBL_LINK_VENDOR_MELLANOX       = 1<<9,
	SBL_LINK_VENDOR_HITACHI        = 1<<10,
	SBL_LINK_VENDOR_HPE            = 1<<11,
	SBL_LINK_VENDOR_CLOUD_LIGHT    = 1<<12,
};


/**
 * @brief Cable length indicator
 */
/* Remember to update validation function */
enum sbl_link_len
{
	SBL_LINK_LEN_INVALID   = 0,
	SBL_LINK_LEN_BACKPLANE = 1ULL << 0,
	SBL_LINK_LEN_000_300   = 1ULL << 1,
	SBL_LINK_LEN_000_400   = 1ULL << 2,
	SBL_LINK_LEN_000_750   = 1ULL << 3,
	SBL_LINK_LEN_000_800   = 1ULL << 4,
	SBL_LINK_LEN_001_000   = 1ULL << 5,
	SBL_LINK_LEN_001_100   = 1ULL << 6,
	SBL_LINK_LEN_001_150   = 1ULL << 7,
	SBL_LINK_LEN_001_200   = 1ULL << 8,
	SBL_LINK_LEN_001_400   = 1ULL << 9,
	SBL_LINK_LEN_001_420   = 1ULL << 10,
	SBL_LINK_LEN_001_500   = 1ULL << 11,
	SBL_LINK_LEN_001_600   = 1ULL << 12,
	SBL_LINK_LEN_001_640   = 1ULL << 13,
	SBL_LINK_LEN_001_700   = 1ULL << 14,
	SBL_LINK_LEN_001_800   = 1ULL << 15,
	SBL_LINK_LEN_001_900   = 1ULL << 16,
	SBL_LINK_LEN_001_910   = 1ULL << 17,
	SBL_LINK_LEN_002_000   = 1ULL << 18,
	SBL_LINK_LEN_002_100   = 1ULL << 19,
	SBL_LINK_LEN_002_130   = 1ULL << 20,
	SBL_LINK_LEN_002_200   = 1ULL << 21,
	SBL_LINK_LEN_002_300   = 1ULL << 22,
	SBL_LINK_LEN_002_390   = 1ULL << 23,
	SBL_LINK_LEN_002_400   = 1ULL << 24,
	SBL_LINK_LEN_002_500   = 1ULL << 25,
	SBL_LINK_LEN_002_600   = 1ULL << 26,
	SBL_LINK_LEN_002_620   = 1ULL << 27,
	SBL_LINK_LEN_002_700   = 1ULL << 28,
	SBL_LINK_LEN_002_800   = 1ULL << 29,
	SBL_LINK_LEN_002_900   = 1ULL << 30,
	SBL_LINK_LEN_002_990   = 1ULL << 31,
	SBL_LINK_LEN_003_000   = 1ULL << 32,
	SBL_LINK_LEN_004_000   = 1ULL << 33,
	SBL_LINK_LEN_005_000   = 1ULL << 34,
	SBL_LINK_LEN_006_000   = 1ULL << 35,
	SBL_LINK_LEN_007_000   = 1ULL << 36,
	SBL_LINK_LEN_008_000   = 1ULL << 37,
	SBL_LINK_LEN_010_000   = 1ULL << 38,
	SBL_LINK_LEN_014_000   = 1ULL << 39,
	SBL_LINK_LEN_015_000   = 1ULL << 40,
	SBL_LINK_LEN_019_000   = 1ULL << 41,
	SBL_LINK_LEN_025_000   = 1ULL << 42,
	SBL_LINK_LEN_030_000   = 1ULL << 43,
	SBL_LINK_LEN_035_000   = 1ULL << 44,
	SBL_LINK_LEN_050_000   = 1ULL << 45,
	SBL_LINK_LEN_075_000   = 1ULL << 46,
	SBL_LINK_LEN_100_000   = 1ULL << 47,
};


/**
 * @brief Attributes describing the media configuration
 *
 */
struct sbl_media_attr
{
	__u32 magic;
	__u32 media;   /**< The link media type */
	__u64 len;     /**< Cable length */
	__u32 info;    /**< Additional information flags */
	__u32 vendor;  /**< Cable vendor info */
};


/**
 * @brief static media attributes initialiser
 *
 */
#define SBL_MEDIA_ATTR_INITIALIZER   \
	{ \
		.magic  = SBL_MEDIA_ATTR_MAGIC, \
		.media  = SBL_LINK_MEDIA_INVALID, \
		.len    = SBL_LINK_LEN_INVALID, \
		.info   = 0, \
		.vendor = SBL_LINK_VENDOR_INVALID, \
	}


/**
 * @brief dynamic media attributes initialiser
 *
 */
static inline void sbl_media_attr_initializer(struct sbl_media_attr *attr)
{
	attr->magic  = SBL_MEDIA_ATTR_MAGIC;
	attr->media  = SBL_LINK_MEDIA_INVALID;
	attr->len    = SBL_LINK_LEN_INVALID;
	attr->info   = 0;
	attr->vendor = SBL_LINK_VENDOR_INVALID;
}


/**
 * @brief media attributes flags
 *
 */
enum sbl_media_attr_info_flags
{
	SBL_MEDIA_INFO_SUPPORTS_BS_200G         = 1<<0,  /**< Media supports SBL_LINK_MODE_BS_200G */
	SBL_MEDIA_INFO_SUPPORTS_BJ_100G         = 1<<1,  /**< Media supports SBL_LINK_MODE_BJ_100G */
	SBL_MEDIA_INFO_SUPPORTS_CD_100G         = 1<<2,  /**< Media supports SBL_LINK_MODE_CD_100G */
	SBL_MEDIA_INFO_SUPPORTS_CD_50G          = 1<<3,  /**< Media supports SBL_LINK_MODE_CD_50G  */
	SBL_MEDIA_INFO_ANALOG                   = 1<<4,  /**< Media is analog                      */
	SBL_MEDIA_INFO_DIGITAL                  = 1<<5,  /**< Media is digital                     */
	SBL_MEDIA_INFO_SUPPORTS_HEADSHELL_RESET = 1<<6,  /**< Media supports headshell reset       */
	SBL_MEDIA_INFO_SUPPORTS_BS_400G         = 1<<7,  /**< Media supports SBL_LINK_MODE_BS_400G */
};


/**
 * @brief IEEE modes for a link
 */
enum sbl_link_mode
{
	SBL_LINK_MODE_INVALID = 0,
	SBL_LINK_MODE_BS_200G = 1<<0, /**< 4 lanes of 50Gbps PAM-4 */
	SBL_LINK_MODE_BJ_100G = 1<<1, /**< 4 lanes of 25Gbps NRZ   */
	SBL_LINK_MODE_CD_100G = 1<<2, /**< 2 lanes of 50Gbps PAM-4 */
	SBL_LINK_MODE_CD_50G  = 1<<3, /**< 1 lane of 50Gbps PAM-4  */
};


/**
 * @brief AN modes for a link
 */
enum sbl_an_mode
{
	SBL_AN_MODE_INVALID = 0,
	SBL_AN_MODE_UNKNOWN,
	SBL_AN_MODE_OFF,         /**< AN off, use link_mode */
	SBL_AN_MODE_ON,          /**< AN to highest common speed */
	SBL_AN_MODE_FIXED,       /**< AN to fixed speed set in link_mode */
};


/**
 * @brief option flags
 */
enum sbl_options_flags
{
	SBL_OPT_FABRIC_LINK                = 1<<1,  /**< assert link is a fabric link */
	SBL_OPT_SERDES_LPD                 = 1<<2,  /**< use the serdes to detect the link partner */
	SBL_OPT_DFE_SAVE_PARAMS            = 1<<3,  /**< Save current tuning params after tuning */
	SBL_OPT_USE_SAVED_PARAMS           = 1<<4,  /**< Load saved tuning params before tuning */
	SBL_OPT_RESET_CLEAR_PARAMS         = 1<<5,  /**< Clear any saved tuning params during reset */
	SBL_OPT_ENABLE_PCAL                = 1<<6,  /**< Enable periodic retuning */
	SBL_OPT_DFE_ALWAYS_MAX_EFFORT      = 1<<7,  /**< Always tune with full effort */
	SBL_OPT_DFE_ALWAYS_MED_EFFORT      = 1<<8,  /**< Always tune with medium effort */
	SBL_OPT_DFE_ALWAYS_MIN_EFFORT      = 1<<9,  /**< Always tune with low effort */
	SBL_OPT_AUTONEG_TIMEOUT_IEEE       = 1<<10, /**< Use IEEE timeout for autoneg link up */
	SBL_OPT_AUTONEG_TIMEOUT_SSHOT      = 1<<11, /**< Use Slingshot timeout for autoneg link up */
	SBL_OPT_AUTONEG_100CR4_FIXUP       = 1<<12, /**< Add cr4 capability to lp's an tec abilities */
	SBL_OPT_RELOAD_FW_ON_TIMEOUT       = 1<<13, /**< Reload serdes fw on tune/minitune timeouts */
	SBL_OPT_ALLOW_MEDIA_BAD_MODE       = 1<<14, /**< Mode not supported by media is non-fatal */
	SBL_OPT_ALLOW_MEDIA_BAD_LEN        = 1<<15, /**< Unrecognised media len is non-fatal */
	SBL_OPT_ENABLE_ETHER_LLR           = 1<<16, /**< enable LLR if detected */
	SBL_OPT_ENABLE_IFG_HPC_WITH_LLR    = 1<<17, /**< enable HPC with LLR when detected*/
	SBL_OPT_ENABLE_IFG_CONFIG          = 1<<18, /**< enable use of ifg config */
	SBL_OPT_DISABLE_AN_LLR             = 1<<19, /**< disable AN LLR detect */
	SBL_OPT_LANE_DEGRADE               = 1<<20, /**< enable auto lane degrade */
	SBL_DISABLE_PML_RECOVERY           = 1<<21, /**< disable pml recovery */
};


/**
 * @brief The loopback mode
 *
 */
enum sbl_loopback_mode
{
	SBL_LOOPBACK_MODE_INVALID = 0,
	SBL_LOOPBACK_MODE_LOCAL   = 1<<0, /**< Loopback in local serdes                           */
	SBL_LOOPBACK_MODE_REMOTE  = 1<<1, /**< Loopback in remote serdes (not currently possible) */
	SBL_LOOPBACK_MODE_OFF     = 1<<2, /**< No loopback                                        */
};


/**
 * @brief Link partner
 */
enum sbl_link_partner
{
	SBL_LINK_PARTNER_INVALID = 0,
	SBL_LINK_PARTNER_SWITCH  = 1<<0, /**< Switch port */
	SBL_LINK_PARTNER_NIC     = 1<<1, /**< Edge port   */
	SBL_LINK_PARTNER_NIC_C2  = 1<<2, /**< Cassini2 Edge port   */
};


/**
 * @brief Tuning pattern
 */
enum sbl_tuning_pattern
{
	SBL_TUNING_PATTERN_INVALID = 0,
	SBL_TUNING_PATTERN_CORE    = 1<<0, /**< Tune off core data        */
	SBL_TUNING_PATTERN_PRBS    = 1<<1, /**< Tune off a PRBS13 pattern */
};


/**
 * @brief Precoding config
 */
enum sbl_precoding_config
{
	SBL_PRECODING_INVALID = 0,
	SBL_PRECODING_UNKNOWN,
	SBL_PRECODING_DEFAULT,    /**< On for fabric links, Off for Ethernet links */
	SBL_PRECODING_ON,         /**< Always on */
	SBL_PRECODING_OFF,        /**< Always off */
};


/**
 * @brief The status of a link SerDes
 *
 */
enum sbl_serdes_status
{
	SBL_SERDES_STATUS_UNKNOWN    = 0,
	SBL_SERDES_STATUS_AUTONEG    = 1<<0,  /**< SerDes in autoneg mode */
	SBL_SERDES_STATUS_LPD_MT     = 1<<1,  /**< SerDes detecting the link partner by minitune */
	SBL_SERDES_STATUS_DOWN       = 1<<2,  /**< SerDes down  */
	SBL_SERDES_STATUS_TUNING     = 1<<3,  /**< SerDes is tuning  */
	SBL_SERDES_STATUS_RUNNING    = 1<<4,  /**< SerDes running */
	SBL_SERDES_STATUS_ERROR      = 1<<5,  /**< SerDes has an error */
	SBL_SERDES_STATUS_RESETTING  = 1<<6,  /**< SerDes is resetting */
};


/**
 * @brief The status of a link SerDes
 *
 */
enum sbl_fw_status
{
	SBL_FW_STATUS_UNKNOWN            = 0,
	SBL_FW_STATUS_NOT_FLASHED        = 1<<0,  /**< Not flashed  */
	SBL_FW_STATUS_FLASHED            = 1<<1,  /**< SBM and SerDes flashed */
	SBL_FW_STATUS_ERROR              = 1<<2,  /**< Flash error */
};


/**
 * @brief FEC (Reed-Solomon) mode
 */
enum sbl_rs_mode
{
	SBL_RS_MODE_INVALID = 0,
	SBL_RS_MODE_UNKNOWN,
	SBL_RS_MODE_OFF,            /**< error correction OFF error checking OFF syndrome checking OFF marking OFF */
	SBL_RS_MODE_OFF_SYN,        /**< error correction OFF error checking OFF syndrome checking  ON marking OFF */
	SBL_RS_MODE_OFF_CHK,        /**< error correction OFF error checking  ON syndrome checking OFF marking OFF */
	SBL_RS_MODE_ON,             /**< error correction  ON error checking  ON syndrome checking OFF marking OFF */
	SBL_RS_MODE_ON_SYN_MRK,     /**< error correction  ON error checking OFF syndrome checking  ON marking  ON */
	SBL_RS_MODE_ON_CHK_SYN_MRK, /**< error correction  ON error checking  ON syndrome checking  ON marking  ON */
};


/**
 * @brief IFG config
 */
enum sbl_ifg_config
{
	SBL_IFG_CONFIG_INVALID = 0,
	SBL_IFG_CONFIG_UNKNOWN,
	SBL_IFG_CONFIG_HPC,          /**< mode hpc,  adj ignored */
	SBL_IFG_CONFIG_IEEE_200G,    /**< mode ieee, adj 200     */
	SBL_IFG_CONFIG_IEEE_100G,    /**< mode ieee, adj 100     */
	SBL_IFG_CONFIG_IEEE_50G,     /**< mode ieee, adj 50      */
};


/**
 * @brief link-level retry (LLR) mode
 */
enum sbl_llr_mode
{
	SBL_LLR_MODE_INVALID = 0,
	SBL_LLR_MODE_UNKNOWN,
	SBL_LLR_MODE_OFF,          /**< No Retry */
	SBL_LLR_MODE_MONITOR,      /**< Monitor but don't retry */
	SBL_LLR_MODE_ON,           /**< Retry  */
	SBL_LLR_MODE_AUTO,         /**< try LLR */
};


/**
 * @brief LLR link down behaviour
 *
 *  @note Fabric link must be set to SBL_LLR_LINK_DOWN_BLOCK
 *
 */
enum sbl_llr_down_behaviour
{
	SBL_LLR_LINK_DOWN_INVALID = 0,
	SBL_LLR_LINK_DOWN_UNKNOWN,
	SBL_LLR_LINK_DOWN_DISCARD,      /**< Discard frame */
	SBL_LLR_LINK_DOWN_BLOCK,        /**< Block waiting for link up */
	SBL_LLR_LINK_DOWN_BEST_EFFORT,  /**< <b>Do not use</b> For Debug  */
};


/**
 * @brief Other LLR settings
 */
#define SBL_LLR_REPLAY_CT_MAX_UNLIMITED                                0xFFULL
#define SBL_DFLT_REPLAY_CT_MAX                                         0xFEULL  /* max - 1 */

/**
 * @brief Attributes for link configuration
 *
 *   These attributes must be configured for a link.
 *   Different setting will be required for fabric or Ethernet links
 *
 */
struct sbl_base_link_attr
{
	__u32 magic;

	/* option flags */
	__u32 options;                   /**< misc config options */

	/* bringup */
	__u32 start_timeout;             /**< timeout for base_link_start (s) */

	/* electrical or optical exclusive configuration */
	__u32 config_target;             /**< PEC or AOC valid */
	union
	{
		struct
		{
			/* auto negotiation */
			__u32 an_mode;                 /**< Requested autonegotiate mode */
			__u32 an_retry_timeout;        /**< Timeout for an AN attempt to complete before retrying (s) */
			__u32 an_max_retry;            /**< Max number of retries before failing */
		} pec;
		struct
		{
			/* optical transceiver locking delay */
			__u32 optical_lock_delay;      /**< time to wait for lock (ms) */
			__u32 optical_lock_interval;   /**< wakeup interval (ms) */
			__u32 reserved;                /**< unused */
		} aoc;
	};

	/* link partner detection */
	__u32 lpd_timeout;               /**< timeout for a lp detection attempt to complete before retrying (s) */
	__u32 lpd_poll_interval;         /**< lp detection polling interval (ms) */

	/* requested config */
	__u32 link_mode;                 /**< Requested link speed mode */
	__u32 loopback_mode;             /**< Requested Loopback mode */
	__u32 link_partner;              /**< NIC or SWITCH link partner */
	__u32 tuning_pattern;            /**< PRBS or CORE tuning pattern */
	__u32 precoding;                 /**< Precoding configuration */

	/* serdes attributes */
	__u32 dfe_pre_delay;             /**< Delay before starting DFE tune (s) */
	__u32 dfe_timeout;               /**< DFE tune wait time (ms) */
	__u32 dfe_poll_interval;         /**< DFE tune done polling interval (ms) */
	__u32 pcal_eyecheck_holdoff;     /**< period to ignore eye heights after pcal starts (ms) */
	__u32 nrz_min_eye_height;        /**< Min eye height criteria for electrical cables with nrz */
	__u32 nrz_max_eye_height;        /**< Max eye height criteria for electrical cables with nrz */
	__u32 pam4_min_eye_height;       /**< Min eye height criteria for optical cables with pam4  */
	__u32 pam4_max_eye_height;       /**< Max eye height criteria for optical cables with pam4 */

	/* pml block attributes */
	__u32 fec_mode;                  /**< FEC (RS) mode */
	__u32 enable_autodegrade;        /**< Enable automatic link degrade handling */
	__u32 llr_mode;                  /**< Link-level retry (LLR) mode */
	__u32 ifg_config;                /**< Inter-frame gap (IFG) config */

	struct
	{
		__u32 timeout;           /**< PML recovery timeout */
		__u32 rl_max_duration;   /**< Rate limiter recovery time per window */
		__u32 rl_window_size;    /**< Rate limiter window size */
	} pml_recovery;
};


/**
 * @brief PEC or AOC configured
 */
enum sbl_base_link_config_target
{
	SBL_BASE_LINK_CONFIG_INVALID   = 0,
	SBL_BASE_LINK_CONFIG_UNKNOWN,
	SBL_BASE_LINK_CONFIG_PEC,
	SBL_BASE_LINK_CONFIG_AOC,
	SBL_BASE_LINK_CONFIG_AEC,
};


/**
 * @brief The status of a base link
 *
 */
enum sbl_base_link_status
{
	SBL_BASE_LINK_STATUS_UNKNOWN          = 0,
	SBL_BASE_LINK_STATUS_UNCONFIGURED     = 1<<0,  /**< Base link is not configured */
	SBL_BASE_LINK_STATUS_STARTING         = 1<<1,  /**< Base link bring-up in progress */
	SBL_BASE_LINK_STATUS_UP               = 1<<2,  /**< Base link Up */
	SBL_BASE_LINK_STATUS_STOPPING         = 1<<3,  /**< Base link bring-down in progress */
	SBL_BASE_LINK_STATUS_DOWN             = 1<<4,  /**< Base link Down */
	SBL_BASE_LINK_STATUS_RESETTING        = 1<<5,  /**< Base link reset in progress */
	SBL_BASE_LINK_STATUS_ERROR            = 1<<6,  /**< Base link has an error */
};


/**
 * @brief Debug options
 *
 */
enum sbl_debug_options {
	SBL_DEBUG_ALL_OPTS                = 0xffffffff,  /**< all flags  */

	SBL_DEBUG_TRACE_LINK_DOWN         = (1<<0),      /**< report link down - generally too verbose*/
	SBL_DEBUG_IGNORE_HISER            = (1<<1),      /**< make hiser errors a warning */
	SBL_DEBUG_INHIBIT_CLEANUP         = (1<<2),      /**< do not cleanup after start fail */
	SBL_DEBUG_INHIBIT_SPLL_RESET      = (1<<3),      /**< do not reset serdes PLLs after tuning failure */

	SBL_DEBUG_BAD_PARAM_1             = (1<<4),      /**< allow one bad tuning param */
	SBL_DEBUG_BAD_PARAM_2             = (1<<5),      /**< allow two bad tuning params */
	SBL_DEBUG_INHIBIT_RELOAD_FW       = (1<<6),      /**< stop serdes fw reload on startup failure */
	SBL_DEBUG_FORCE_RELOAD_FW         = (1<<7),      /**< force serdes fw to reload on startup failure */

	SBL_DEBUG_FORCE_MAX_EFFORT        = (1<<8),      /**< force maximum effort tuning */
	SBL_DEBUG_FORCE_MED_EFFORT        = (1<<9),      /**< force_medium effort tuning */
	SBL_DEBUG_FORCE_MIN_EFFORT        = (1<<10),     /**< force_minium effort tuning */
	SBL_DEBUG_INHIBIT_USE_SAVED_TP    = (1<<11),     /**< do not use saved tuning params */

	SBL_DEBUG_FORCE_PRECODING_ON      = (1<<12),     /**< Force serdes precoding on */
	SBL_DEBUG_FORCE_PRECODING_OFF     = (1<<13),     /**< Force serdes precoding off */
	SBL_DEBUG_ALLOW_MEDIA_BAD_MODE    = (1<<14),     /**< Mode not supported by media not error */
	SBL_DEBUG_ALLOW_MEDIA_BAD_LEN     = (1<<15),     /**< Unrecognised media length not error */

	SBL_DEBUG_INHIBIT_PCAL            = (1<<16),     /**< do not start PCAL */
	SBL_DEBUG_INHIBIT_RELOAD_SBM_FW   = (1<<17),     /**< stop sbus master fw reload on serdes fw reload failure */
	SBL_DEBUG_FORCE_RELOAD_SBM_FW     = (1<<18),     /**< force sbus master fw reload on serdes fw reload failure */
	SBL_DEBUG_DISABLE_AN_NEXT_PAGES   = (1<<19),     /**< disable next pages during auto negotiation */

	SBL_DEBUG_KEEP_SERDES_UP          = (1<<20),     /**< keep serdes up when link is going down */
	SBL_DEBUG_SERDES_MAP_DELAY        = (1<<21),     /**< add delay before checking serdes eye heights */
	SBL_DEBUG_FORCE_RELOAD_SERDES_FW  = (1<<22),     /**< force serdes fw reload during link reset */
	SBL_DEBUG_ALLOW_LOOP_TIME_FAIL    = (1<<23),     /**< fail loop time measurement */

	SBL_DEBUG_IGNORE_ALIGN            = (1<<24),     /**< ignore alignment interrupt */
	SBL_DEBUG_TRACE_PML_INT           = (1<<25),     /**< enable pml interrupt output */
	SBL_DEBUG_REMOTE_FAULT_RECOVERY   = (1<<26),     /**< enable pml recovery for remote faults */
	SBL_DEBUG_IGNORE_HIGH_FEC_UCW     = (1<<27),     /**< enable FEC UCW debug */

	SBL_DEBUG_DEV0                    = (1<<28),     /**< for development - no fixed use */
	SBL_DEBUG_IGNORE_HIGH_FEC_TXR     = (1<<29),     /**< enable FEC TXR debug */
	SBL_DEBUG_IGNORE_HIGH_FEC_CCW     = (1<<30),     /**< enable FEC CCW debug */
	SBL_DEBUG_TEST                    = (1<<31),     /**< for testing the debug system  */
};


#ifdef __cplusplus
}
#endif

#endif  /* _SBL_UAPI_BASE_LINK_H_ */
