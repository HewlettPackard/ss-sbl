/* SPDX-License-Identifier: GPL-2.0 */

/**
 * @file uapi/sbl_serdes.h
 *
 * Copyright 2019, 2022, 2024 Hewlett Packard Enterprise Development LP
 *
 * @brief Base-link SerDes configuration
 *
 *  Some paranoia here to make sure we don't use old format structures
 */


#ifndef _SBL_UAPI_BASE_LINK_SERDES_H_
#define _SBL_UAPI_BASE_LINK_SERDES_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SBL_TUNING_PARAM_MAGIC           0x74736d70  /* stpm */
#define SBL_TUNING_PARAM_VERSION                  2

#define SBL_SERDES_CONFIG_MAGIC          0x63736d76  /* scvm */
#define SBL_MAX_OOB_SERDES_PARAMS        0 /* Allow no more than this number of
					      params to be out of bounds */
#define SBL_CTLE_HF_MIN          0
#define SBL_CTLE_HF_MAX          15
#define SBL_CTLE_LF_MIN          0
#define SBL_CTLE_LF_MAX          15
#define SBL_CTLE_DC_MIN          0
#define SBL_CTLE_DC_MAX          0
#define SBL_CTLE_BW_MIN          0
#define SBL_CTLE_BW_MAX          15
#define SBL_CTLE_GS1_MIN         0
#define SBL_CTLE_GS1_MAX         3
#define SBL_CTLE_GS2_MIN         0
#define SBL_CTLE_GS2_MAX         3
#define SBL_CTLE_SCE_MIN         0
#define SBL_CTLE_SCE_MAX         0
#define SBL_CTLE_HF_MIN_MIN      0
#define SBL_CTLE_HF_MIN_MAX      0
#define SBL_CTLE_HF_MAX_MIN      15
#define SBL_CTLE_HF_MAX_MAX      15
#define SBL_CTLE_LF_MIN_MIN      0
#define SBL_CTLE_LF_MIN_MAX      3  //relaxed
#define SBL_CTLE_LF_MAX_MIN      10 //relaxed
#define SBL_CTLE_LF_MAX_MAX      15

#define SBL_FFE_PRE2_MIN	 -10 //relaxed
#define SBL_FFE_PRE2_MAX	 2  //relaxed to 2 for BJ_100G
#define SBL_FFE_PRE1_MIN	 0
#define SBL_FFE_PRE1_MAX	 11  //relaxed
#define SBL_FFE_POST1_MIN	 -3
#define SBL_FFE_POST1_MAX	 8
#define SBL_FFE_BFLF_MIN	 0
#define SBL_FFE_BFLF_MAX	 0
#define SBL_FFE_BFHF_MIN	 0
#define SBL_FFE_BFHF_MAX	 0
#define SBL_FFE_DATARATE_MIN	 0
#define SBL_FFE_DATARATE_MAX	 0
#define SBL_FFE_SCE_MIN		 0
#define SBL_FFE_SCE_MAX		 0
#define SBL_FFE_PRE1_MIN_MIN	 0
#define SBL_FFE_PRE1_MIN_MAX	 0
#define SBL_FFE_PRE1_MAX_MIN	 15
#define SBL_FFE_PRE1_MAX_MAX	 15
#define SBL_FFE_PRE2_MIN_MIN	 -10
#define SBL_FFE_PRE2_MIN_MAX	 -10
#define SBL_FFE_PRE2_MAX_MIN	 10
#define SBL_FFE_PRE2_MAX_MAX	 10
#define SBL_FFE_BFLF_ICAL_MIN	 0
#define SBL_FFE_BFLF_ICAL_MAX	 0
#define SBL_FFE_POST1_MIN_MIN	 -3
#define SBL_FFE_POST1_MIN_MAX	 -3
#define SBL_FFE_POST1_MAX_MIN	 8
#define SBL_FFE_POST1_MAX_MAX	 8

#define SBL_DFE_GT1_MIN		 255
#define SBL_DFE_GT1_MAX		 255
#define SBL_DFE_GT2_MIN		 255
#define SBL_DFE_GT2_MAX		 255
#define SBL_DFE_G2_MIN		 -31 //relaxed
#define SBL_DFE_G2_MAX		 15  //relaxed
#define SBL_DFE_G3_MIN		 -9  //relaxed
#define SBL_DFE_G3_MAX		 14
#define SBL_DFE_G4_MIN		 -6  //relaxed
#define SBL_DFE_G4_MAX		 7   //relaxed
#define SBL_DFE_G5_MIN		 -4  //relaxed
#define SBL_DFE_G5_MAX		 12  //relaxed
#define SBL_DFE_G6_MIN		 -4  //relaxed
#define SBL_DFE_G6_MAX		 6   //relaxed
#define SBL_DFE_G7_MIN		 -4  //relaxed
#define SBL_DFE_G7_MAX		 6   //relaxed
#define SBL_DFE_G8_MIN		 -5
#define SBL_DFE_G8_MAX		 7   //relaxed
#define SBL_DFE_G9_MIN		 -4  //constrained
#define SBL_DFE_G9_MAX		 6
#define SBL_DFE_G10_MIN		 -7
#define SBL_DFE_G10_MAX		 5
#define SBL_DFE_G11_MIN		 -4  //relaxed
#define SBL_DFE_G11_MAX		 4   //relaxed
#define SBL_DFE_G12_MIN		 -3  //relaxed
#define SBL_DFE_G12_MAX		 4   //relaxed
#define SBL_DFE_G13_MIN		 0
#define SBL_DFE_G13_MAX		 0

/**
 * @brief SerDes tuning parameters
 *
 */
enum sbl_ctle_params {
	CTLE_HF_OFFSET = 0,
	CTLE_LF_OFFSET,
	CTLE_DC_OFFSET,
	CTLE_BW_OFFSET,
	CTLE_GS1_OFFSET,
	CTLE_GS2_OFFSET,
	CTLE_SCE_OFFSET,
	CTLE_HF_MIN_OFFSET,
	CTLE_HF_MAX_OFFSET,
	CTLE_LF_MIN_OFFSET,
	CTLE_LF_MAX_OFFSET,
	NUM_CTLE_PARAMS
};

enum sbl_ffe_params {
	FFE_PRE2_OFFSET = 0,
	FFE_PRE1_OFFSET,
	FFE_POST1_OFFSET,
	FFE_BFLF_OFFSET,
	FFE_BFHF_OFFSET,
	FFE_DATARATE_OFFSET,
	FFE_SCE_OFFSET,
	FFE_PRE1_MIN_OFFSET,
	FFE_PRE1_MAX_OFFSET,
	FFE_PRE2_MIN_OFFSET,
	FFE_PRE2_MAX_OFFSET,
	FFE_BFLF_ICAL_OFFSET,
	FFE_POST1_MIN_OFFSET,
	FFE_POST1_MAX_OFFSET,
	NUM_FFE_PARAMS
};

enum sbl_dfe_params {
	DFE_GT1_OFFSET = 0,
	DFE_GT2_OFFSET,
	DFE_G2_OFFSET,
	DFE_G3_OFFSET,
	DFE_G4_OFFSET,
	DFE_G5_OFFSET,
	DFE_G6_OFFSET,
	DFE_G7_OFFSET,
	DFE_G8_OFFSET,
	DFE_G9_OFFSET,
	DFE_G10_OFFSET,
	DFE_G11_OFFSET,
	DFE_G12_OFFSET,
	DFE_G13_OFFSET,
	NUM_DFE_PARAMS
};

enum sbl_rxvc_params {
	RXVC_UOC_OFFSET = 0,
	RXVC_UEC_OFFSET,
	RXVC_MOC_OFFSET,
	RXVC_MEC_OFFSET,
	RXVC_LOC_OFFSET,
	RXVC_LEC_OFFSET,
	NUM_RXVC_PARAMS
};

enum sbl_rxvs_params {
	RXVS_UOD_OFFSET = 0,
	RXVS_UED_OFFSET,
	RXVS_MOD_OFFSET,
	RXVS_MED_OFFSET,
	RXVS_LOD_OFFSET,
	RXVS_LED_OFFSET,
	RXVS_TOD_OFFSET,
	RXVS_TED_OFFSET,
	RXVS_EOD_OFFSET,
	RXVS_EED_OFFSET,
	RXVS_TD_OFFSET,
	NUM_RXVS_PARAMS
};

enum sbl_rsdo_params {
	RSDO_LE_OFFSET = 0,
	RSDO_LO_OFFSET,
	RSDO_ME_OFFSET,
	RSDO_MO_OFFSET,
	RSDO_UE_OFFSET,
	RSDO_UO_OFFSET,
	NUM_RSDO_PARAMS
};

enum sbl_rsdc_params {
	RSDC_LE_OFFSET = 0,
	RSDC_LO_OFFSET,
	RSDC_ME_OFFSET,
	RSDC_MO_OFFSET,
	RSDC_UE_OFFSET,
	RSDC_UO_OFFSET,
	NUM_RSDC_PARAMS
};

enum sbl_rsto_params {
	RSTO_TE_OFFSET = 0,
	RSTO_TO_OFFSET,
	RSTO_CE_OFFSET,
	RSTO_CO_OFFSET,
	NUM_RSTO_PARAMS
};

enum sbl_rstc_params {
	RSTC_TE_OFFSET = 0,
	RSTC_TO_OFFSET,
	RSTC_CE_OFFSET,
	RSTC_CO_OFFSET,
	NUM_RSTC_PARAMS
};

enum sbl_eh_params {
	EH_HLE_OFFSET = 0,
	EH_HME_OFFSET,
	EH_HUE_OFFSET,
	EH_HLO_OFFSET,
	EH_HMO_OFFSET,
	EH_HUO_OFFSET,
	EH_SEL_OFFSET,
	EH_SEM_OFFSET,
	EH_SEU_OFFSET,
	EH_SOL_OFFSET,
	EH_SOM_OFFSET,
	EH_SOU_OFFSET,
	NUM_EH_PARAMS
};

enum sbl_gtp_params {
	GTP_ERTH_OFFSET = 0,
	GTP_NCDW_OFFSET,
	GTP_NCLD_OFFSET,
	GTP_EOVS_OFFSET,
	GTP_PCDW_OFFSET,
	GTP_PDDW_OFFSET,
	GTP_PDPD_OFFSET,
	GTP_LMDW_OFFSET,
	GTP_CTLF_OFFSET,
	GTP_RXFF_OFFSET,
	GTP_DFEF_OFFSET,
	GTP_GTMM_OFFSET,
	GTP_G2MM_OFFSET,
	GTP_PCLL_OFFSET,
	GTP_DOSP_OFFSET,
	GTP_LPDW_OFFSET,
	GTP_DPDW_OFFSET,
	GTP_PCLD_OFFSET,
	GTP_ICID_OFFSET,
	GTP_CLMM_OFFSET,
	GTP_CDMM_OFFSET,
	GTP_VRFX_OFFSET,
	GTP_PRTM_OFFSET,
	GTP_GROP_OFFSET,
	GTP_ICLE_OFFSET,
	GTP_URCC_OFFSET,
	GTP_LTFL_OFFSET,
	GTP_DTCF_OFFSET,
	NUM_GTP_PARAMS
};

enum sbl_dccd_params {
	DCCD_UE_OFFSET = 0,
	DCCD_UO_OFFSET,
	DCCD_ME_OFFSET,
	DCCD_MO_OFFSET,
	DCCD_LE_OFFSET,
	DCCD_LO_OFFSET,
	NUM_DCCD_PARAMS
};

enum sbl_p4lv_params {
	P4LV_X3XE_OFFSET = 0,
	P4LV_X3XO_OFFSET,
	P4LV_X2XE_OFFSET,
	P4LV_X2XO_OFFSET,
	P4LV_X1XE_OFFSET,
	P4LV_X1XO_OFFSET,
	P4LV_X0XE_OFFSET,
	P4LV_X0XO_OFFSET,
	NUM_P4LV_PARAMS
};

enum sbl_afec_params {
	AFEC_HFPM_OFFSET = 0,
	AFEC_HFMN_OFFSET,
	AFEC_HFMX_OFFSET,
	AFEC_TEMP_OFFSET,
	AFEC_NWHF_OFFSET,
	AFEC_NWP1_OFFSET,
	NUM_AFEC_PARAMS
};

/**
 * @brief Arrays of SerDes tuning parameters.
 */
struct sbl_serdes_params
{
	__s16 ctle[NUM_CTLE_PARAMS];  /**< CTLE tuning params */
	__s16 ffe[NUM_FFE_PARAMS];    /**< FFE tuning params */
	__s16 dfe[NUM_DFE_PARAMS];    /**< DFE tuning params */
	__s16 rxvs[NUM_RXVS_PARAMS];  /**< Rx CLK Vernier Settings */
	__s16 rxvc[NUM_RXVC_PARAMS];  /**< Rx CLK Vernier Cals */
	__s16 rsdo[NUM_RSDO_PARAMS];  /**< Rx Sampler Data Offset */
	__s16 rsdc[NUM_RSDC_PARAMS];  /**< Rx Sampler Data Cal */
	__s16 rsto[NUM_RSTO_PARAMS];  /**< Rx Sampler Test/CDR Offset */
	__s16 rstc[NUM_RSTC_PARAMS];  /**< Rx Sampler Test/CDR Cal */
	__s16 eh[NUM_EH_PARAMS];      /**< Eye Heights */
	__s16 gtp[NUM_GTP_PARAMS];    /**< Global Tuning Params */
	__s16 dccd[NUM_DCCD_PARAMS];  /**< Data Channel Cal Deltas */
	__s16 p4lv[NUM_P4LV_PARAMS];  /**< PAM4 Levels */
	__s16 afec[NUM_AFEC_PARAMS];  /**< AFE Control */
};

/**
 * @brief Set of tuning parameters used to configure a link, allowing the SerDes
 *         to come up without needing to DFE tune.
 *
 *        tp_state_hash maps the params with the SerDes state for which they
 *         were derived. This allows us to prevent applying saved parameters
 *         to configurations different from which they were intended for.
 */
struct sbl_tuning_params
{
	__u32 magic;          /**< = SBL_TUNING_PARAM_MAGIC */
	__u32 version;        /**< version number */
	__u64 tp_state_hash0;  /**< Info for lp, lb, tp, lb, mt, mv */
	__u64 tp_state_hash1;  /**< Info for ml */
	struct sbl_serdes_params params[4];
};	


/**
 * @brief SerDes configuration values
 *
 */
#define SBL_SC_MAX_INTR          16

struct sbl_sc_values
{
	__u32 magic;                        /**< = SBL_SERDES_CONFIG_MAGIC */
	__u32 atten;                        /**< attenuation */
	__u32 pre;                          /**<  */
	__u32 post;                         /**<  */
	__u32 pre2;                         /**<  */
	__u32 pre3;                         /**<  */
	__u32 gs1;                          /**< gainshape 1 */
	__u32 gs2;                          /**< gainshape 2 */
	__u32 num_intr;                     /**< number of serdes core interrupts (serdes ops)*/
	__u16 intr_val[SBL_SC_MAX_INTR];    /**< interrupts to perform */
	__u16 data_val[SBL_SC_MAX_INTR];    /**< data for interrupts */
};


/*
 * serdes configuration hash.
 *
 *   used for serdes configurations and saving tuning parameters.
 */
#define SBL_TPH_LP_OFFSET	0x0
#define SBL_TPH_LB_OFFSET	0x8
#define SBL_TPH_TP_OFFSET	0x10
#define SBL_TPH_LM_OFFSET	0x18
#define SBL_TPH_MT_OFFSET	0x20
#define SBL_TPH_MV_OFFSET	0x28

#define SBL_TPH_ML_OFFSET	0x0

#define SBL_TPH_LP_MASK		0xffULL
#define SBL_TPH_LB_MASK		0xffULL
#define SBL_TPH_TP_MASK		0xffULL
#define SBL_TPH_LM_MASK		0xffULL
#define SBL_TPH_MT_MASK		0xffULL
#define SBL_TPH_MV_MASK		0xffffULL

#define SBL_TPH_ML_MASK		0xffffffffffffffffULL

#define SBL_TPH_LP_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_LP_MASK) << \
					    SBL_TPH_LP_OFFSET))
#define SBL_TPH_LB_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_LB_MASK) << \
					    SBL_TPH_LB_OFFSET))
#define SBL_TPH_TP_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_TP_MASK) << \
					    SBL_TPH_TP_OFFSET))
#define SBL_TPH_LM_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_LM_MASK) << \
					    SBL_TPH_LM_OFFSET))
#define SBL_TPH_MT_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_MT_MASK) << \
					    SBL_TPH_MT_OFFSET))
#define SBL_TPH_MV_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_MV_MASK) << \
					    SBL_TPH_MV_OFFSET))

#define SBL_TPH_ML_SET(hash, val) (hash |= (((__u64)val & SBL_TPH_ML_MASK) << \
					    SBL_TPH_ML_OFFSET))

/*
 * Function to generate a hash of the given state configuration.
 * The function can be used to create either match values or match masks
 *     hash0:
 *     link_partner:	0x00000000000000ff
 *     loopback_mode:	0x000000000000ff00
 *     tuning_pattern:	0x0000000000ff0000
 *     link_mode:	0x00000000ff000000
 *     media_type:	0x000000ff00000000
 *     media_vendor:	0x00ffff0000000000
 *     hash1:
 *     media_len:	0xffffffffffffffff
 *     e.g. If the configuration applies for all LPs and TPs, and for LB off,
 *          LM of BJ_100, 1m copper cables for all vendors, then set the
 *          following:
 *     hash0 = 0x00ff010201ff04ff
 *     hash1 = 0x0000000000000020
 *
 */
static inline __u64 sbl_create_tp_hash0(__u8 link_partner_mask,
					__u8 loopback_mode_mask,
					__u8 tuning_pattern_mask,
					__u8 link_mode_mask,
					__u8 media_type_mask,
					__u16 media_vendor_mask)
{
	__u64 hash = 0;

	SBL_TPH_LP_SET(hash, link_partner_mask);
	SBL_TPH_LB_SET(hash, loopback_mode_mask);
	SBL_TPH_TP_SET(hash, tuning_pattern_mask);
	SBL_TPH_LM_SET(hash, link_mode_mask);
	SBL_TPH_MT_SET(hash, media_type_mask);
	SBL_TPH_MV_SET(hash, media_vendor_mask);

	return hash;
}
static inline __u64 sbl_create_tp_hash1(__u64 media_len_mask)
{
	__u64 hash = 0;

	SBL_TPH_ML_SET(hash, media_len_mask);

	return hash;
}

#ifdef __cplusplus
}
#endif

#endif  /* _SBL_UAPI_BASE_LINK_SERDES_H_ */
