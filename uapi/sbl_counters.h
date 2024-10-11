/* SPDX-License-Identifier: GPL-2.0 */

/**
 * @file uapi/sbl_counters.h
 *
 * Copyright 2019, 2022 Hewlett Packard Enterprise Development LP
 *
 * @brief SBL software counter support
 *
 */

#ifndef _SBL_UAPI_COUNTERS_H_
#define _SBL_UAPI_COUNTERS_H_

#define SBL_NUM_COUNTERS SBL_LINK_NUM_COUNTERS
#define SBL_COUNTERS SBL_LINK_COUNTERS
#define SBL_COUNTERS_NAME SBL_LINK_COUNTERS_NAME
#define SBL_COUNTERS_FIRST sbl_serdes0_fw_reload
#define SBL_PML_REC_HISTOGRAM_MAX 120

#define SBL_LINK_COUNTERS sbl_serdes0_fw_reload,   \
			  sbl_serdes1_fw_reload,   \
			  sbl_serdes2_fw_reload,   \
			  sbl_serdes3_fw_reload,   \
			  sbl_serdes0_spico_reset, \
			  sbl_serdes1_spico_reset, \
			  sbl_serdes2_spico_reset, \
			  sbl_serdes3_spico_reset, \
			  sbl_serdes0_pll_reset,   \
			  sbl_serdes1_pll_reset,   \
			  sbl_serdes2_pll_reset,   \
			  sbl_serdes3_pll_reset,   \
			  sbl_serdes_fw_reload_skip,  \
			  sbl_pml_recovery_attempts,  \
			  sbl_pml_recovery_successes, \
			  sbl_pml_recovery_origin_bl_ldown,  \
			  sbl_pml_recovery_origin_bl_lfault, \
			  sbl_pml_recovery_origin_bl_rfault, \
			  sbl_pml_recovery_origin_bl_align,  \
			  sbl_pml_recovery_origin_bl_hiser,  \
			  sbl_pml_recovery_origin_bl_llr,    \
			  sbl_pml_recovery_histogram_0_9ms,   \
			  sbl_pml_recovery_histogram_10_19ms, \
			  sbl_pml_recovery_histogram_20_29ms, \
			  sbl_pml_recovery_histogram_30_39ms, \
			  sbl_pml_recovery_histogram_40_49ms, \
			  sbl_pml_recovery_histogram_50_59ms, \
			  sbl_pml_recovery_histogram_60_69ms, \
			  sbl_pml_recovery_histogram_70_79ms, \
			  sbl_pml_recovery_histogram_80_89ms, \
			  sbl_pml_recovery_histogram_90_99ms, \
			  sbl_pml_recovery_histogram_100_109ms, \
			  sbl_pml_recovery_histogram_110_119ms, \
			  sbl_pml_recovery_histogram_high,	\
			  sbl_pml_recovery_rate_exceeded,	\
			  sbl_fec_ucw_err,	\
			  sbl_fec_ccw_err,	\
			  sbl_fec_txr_err,	\
			  sbl_fec_warn,		\
			  sbl_fec_up_fail

#define SBL_LINK_COUNTERS_NAME "sbl_serdes0_fw_reload",   \
				"sbl_serdes1_fw_reload",   \
			        "sbl_serdes2_fw_reload",   \
				"sbl_serdes3_fw_reload",   \
				"sbl_serdes0_spico_reset", \
				"sbl_serdes1_spico_reset", \
				"sbl_serdes2_spico_reset", \
				"sbl_serdes3_spico_reset", \
				"sbl_serdes0_pll_reset",   \
				"sbl_serdes1_pll_reset",   \
				"sbl_serdes2_pll_reset",   \
				"sbl_serdes3_pll_reset",   \
				"sbl_serdes_fw_reload_skip",  \
				"sbl_pml_recovery_attempts",  \
				"sbl_pml_recovery_successes", \
				"sbl_pml_recovery_origin_bl_ldown",  \
				"sbl_pml_recovery_origin_bl_lfault", \
				"sbl_pml_recovery_origin_bl_rfault", \
				"sbl_pml_recovery_origin_bl_align",  \
				"sbl_pml_recovery_origin_bl_hiser",  \
				"sbl_pml_recovery_origin_bl_llr",	 \
				"sbl_pml_recovery_histogram_0_9ms",	 \
				"sbl_pml_recovery_histogram_10_19ms", \
				"sbl_pml_recovery_histogram_20_29ms", \
				"sbl_pml_recovery_histogram_30_39ms", \
				"sbl_pml_recovery_histogram_40_49ms", \
				"sbl_pml_recovery_histogram_50_59ms", \
				"sbl_pml_recovery_histogram_60_69ms", \
				"sbl_pml_recovery_histogram_70_79ms", \
				"sbl_pml_recovery_histogram_80_89ms", \
				"sbl_pml_recovery_histogram_90_99ms", \
				"sbl_pml_recovery_histogram_100_109ms", \
				"sbl_pml_recovery_histogram_110_119ms", \
				"sbl_pml_recovery_histogram_high",	   \
				"sbl_pml_recovery_rate_exceeded",	   \
				"sbl_fec_ucw_err",		\
				"sbl_fec_ccw_err",		\
				"sbl_fec_txr_err",		\
				"sbl_fec_warn",			\
				"sbl_fec_up_fail"

/**
 * @brief SBL link level counter indexes
 *
 */

enum sbl_link_counters_idx {

	serdes0_fw_reload = 0,		/**< serdes 0 firmware reload count */
	serdes1_fw_reload,		/**< serdes 1 firmware reload count */
	serdes2_fw_reload,		/**< serdes 2 firmware reload count */
	serdes3_fw_reload,		/**< serdes 3 firmware reload count */

	serdes0_spico_reset,		/**< serdes 0 spico reset count */
	serdes1_spico_reset,		/**< serdes 1 spico reset count */
	serdes2_spico_reset,		/**< serdes 2 spico reset count */
	serdes3_spico_reset,		/**< serdes 3 spico reset count */

	serdes0_pll_reset,		/**< serdes 0 pll reset count */
	serdes1_pll_reset,		/**< serdes 1 pll reset count */
	serdes2_pll_reset,		/**< serdes 2 pll reset count */
	serdes3_pll_reset,		/**< serdes 3 pll reset count */

	serdes_fw_reload_skip,		/**< serdes fw reloads skipped count */

	pml_recovery_attempts,			/**< pml recovery attempts */
	pml_recovery_successes,			/**< pml recovery successes */
	pml_recovery_origin_bl_ldown,		/**< pml recovery count for link down */
	pml_recovery_origin_bl_lfault,		/**< pml recovery count for local fault */
	pml_recovery_origin_bl_rfault,		/**< pml recovery count for remote fault */
	pml_recovery_origin_bl_align,		/**< pml recovery count for align fault */
	pml_recovery_origin_bl_hiser,		/**< pml recovery count for hiser */
	pml_recovery_origin_bl_llr,		/**< pml recovery count for max llr replay */
	pml_recovery_histogram_0_9ms,		/**< pml recovery time 0-9 ms */
	pml_recovery_histogram_10_19ms,		/**< pml recovery time 10-19 ms */
	pml_recovery_histogram_20_29ms,		/**< pml recovery time 20-29 ms */
	pml_recovery_histogram_30_39ms,		/**< pml recovery time 30-39 ms */
	pml_recovery_histogram_40_49ms,		/**< pml recovery time 40-49 ms */
	pml_recovery_histogram_50_59ms,		/**< pml recovery time 50-59 ms */
	pml_recovery_histogram_60_69ms,		/**< pml recovery time 60-69 ms */
	pml_recovery_histogram_70_79ms,		/**< pml recovery time 70-79 ms */
	pml_recovery_histogram_80_89ms,		/**< pml recovery time 80-89 ms */
	pml_recovery_histogram_90_99ms,		/**< pml recovery time 90-99 ms */
	pml_recovery_histogram_100_109ms,	/**< pml recovery time 100-109 ms */
	pml_recovery_histogram_110_119ms,	/**< pml recovery time 110-119 ms */
	pml_recovery_histogram_high,		/**< pml recovery time 120+ ms (SBL_PML_REC_HISTOGRAM_MAX) */
	pml_recovery_rate_exceeded,		/**< pml recovery rate limit exceeded */
	fec_ucw_err,		/** fec ucw bad */
	fec_ccw_err,		/** fec ccw bad */
	fec_txr_err,		/** fec txr bad */
	fec_warn,		/** fec ccw warning */
	fec_up_fail,		/** fec start check fail */

	SBL_LINK_NUM_COUNTERS,		/* the number of SBL counters */
};

#endif /* _SBL_UAPI_COUNTERS_H_ */
