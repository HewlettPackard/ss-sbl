/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2020,2022-2023 Hewlett Packard Enterprise Development LP. All rights reserved. */

/**
 * @file uapi/sbl_serdes_defaults.h
 *
 * @brief Default serdes configuration values
 *
 *
 */


#ifndef _SBL_UAPI_SBL_SERDES_DEFAULTS_H_
#define _SBL_UAPI_SBL_SERDES_DEFAULTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defaults for per-port SerDes attributes
 *
 */
#define SBL_DFLT_OPTICAL_LOCK_DELAY                      2000
#define SBL_DFLT_OPTICAL_LOCK_INTERVAL                    500
#define SBL_DFLT_DFE_PRE_DELAY_PEC                          0
#define SBL_DFLT_DFE_PRE_DELAY_AOC                          0
#define SBL_DFLT_DFE_TIMEOUT_PEC                           40
#define SBL_DFLT_DFE_TIMEOUT_AOC                           40
#define SBL_DFLT_DFE_POLL_INTERVAL                        250
#define SBL_DFLT_NRZ_PEC_MIN_EYE_HEIGHT                  0xc0
#define SBL_DFLT_NRZ_PEC_MAX_EYE_HEIGHT                 0x200
#define SBL_DFLT_NRZ_AOC_MIN_EYE_HEIGHT                  0xc0
#define SBL_DFLT_NRZ_AOC_MAX_EYE_HEIGHT                 0x200
#define SBL_DFLT_PAM4_PEC_MIN_EYE_HEIGHT                 0x10
#define SBL_DFLT_PAM4_PEC_MAX_EYE_HEIGHT                 0x50
#define SBL_DFLT_PAM4_AOC_MIN_EYE_HEIGHT                 0x25
#define SBL_DFLT_PAM4_AOC_MAX_EYE_HEIGHT                 0x70
#define SBL_DFLT_LPD_TIMEOUT                               12
#define SBL_DFLT_LPD_POLL_INTERVAL                        333
#define SBL_DFLT_PCAL_EYECHECK_HOLDOFF_PEC                  0
#define SBL_DFLT_PCAL_EYECHECK_HOLDOFF_AOC                  0

/**
 * @brief Defaults SerDes attributes common for the instance
 *
 */
#define SBL_DFLT_SBUS_OP_FLAGS_SLOW        (SBL_FLAG_INTERVAL_1MS | SBL_FLAG_DELAY_50US)
#define SBL_DFLT_SBUS_OP_FLAGS_FAST        (SBL_FLAG_INTERVAL_1MS | SBL_FLAG_DELAY_5US)
#define SBL_DFLT_SERDES_OP_FLAGS           (SBL_FLAG_INTERVAL_1MS | SBL_FLAG_DELAY_5US)
#define SBL_DFLT_RX_PHASE_SLIP_CNT                        0xc
#define SBL_DFLT_SBUS_INT_TIMEOUT                           2
#define SBL_DFLT_SBUS_INT_POLL_INTERVAL                     1
#define SBL_DFLT_SBUS_OP_TIMEOUT_MS                      1000
#define SBL_DFLT_SERDES_EN_TIMEOUT                         20
#define SBL_DFLT_SERDES_EN_POLL_INTERVAL                    1
#define SBL_DFLT_SERDES_OP_TIMEOUT_MS                    1000
#define SBL_DFLT_CORE_STATUS_RD_TIMEOUT                     1
#define SBL_DFLT_CORE_STATUS_RD_POLL_INTERVAL              50
#define SBL_DFLT_PLATFORM                   SBL_PLAT_HARDWARE
/**
 * @brief SerDes config table field defaults
 *
 */
#define SBL_DFLT_PORT_CONFIG_ATTEN                           2
#define SBL_DFLT_PORT_CONFIG_PRE                             0
#define SBL_DFLT_PORT_CONFIG_POST                            0
#define SBL_DFLT_PORT_CONFIG_PRE2                            0
#define SBL_DFLT_PORT_CONFIG_PRE3                            0
#define SBL_DFLT_PORT_CONFIG_GS1                             0
#define SBL_DFLT_PORT_CONFIG_GS2                             0
#define SBL_DFLT_PORT_CONFIG_NUM_INTR                        0
#define SBL_DFLT_PORT_CONFIG_INTR_VAL                     { 0 }
#define SBL_DFLT_PORT_CONFIG_DATA_VAL                     { 0 }

#define SBL_DFLT_PORT_CONFIG_GS1_OPTICAL                     2
#define SBL_DFLT_PORT_CONFIG_GS2_OPTICAL                     0

#ifdef __cplusplus
}
#endif

#endif  /* _SBL_UAPI_SBL_SERDES_DEFAULTS_H_ */
