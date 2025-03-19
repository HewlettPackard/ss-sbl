/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2020-2021, 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SBL_CONSTANTS_H_
#define _SBL_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TRACE2
#define dev_trace2 dev_dbg
#endif /* TRACE2 */

#ifdef TRACE3
#define dev_trace3 dev_dbg
#endif /* TRACE3 */

#define SBL_MODEL_ROS_COLORADO "sc-ros"
#define SBL_MODEL_ROS_COLUMBIA "sc-ros-tor"
#define SBL_MODEL_ROS_TEXAS "sc-ros-tex"

#define SBUS_ADDR(ring, rx_addr)        ((ring << 8) | rx_addr)
#define SBUS_RING(sbus_addr)            ((sbus_addr & 0x300) >> 8)
#define SBUS_RX_ADDR(sbus_addr)         (sbus_addr & 0xff)

#define SBL_FW_REV_LEN		4
#define SBL_FW_BUILD_LEN	4

#define LEVEL_ERR                            4
#define LEVEL_WARN                           3
#define LEVEL_INFO                           2
#define LEVEL_DBG                            1

#define SERDES_OP_TIMEOUT_MS          1000
#define SBUS_OP_TIMEOUT_MS            1000
#define SERDES_OP_FLAGS               (SBL_FLAG_INTERVAL_1MS | SBL_FLAG_DELAY_5US)

#define SBL_ALL_PORTS                 -1

#define SBL_CONFIG_RX_PHASE_SLIP_CNT_DFLT    0xc
#define SBL_CONFIG_DFE_PRE_DELAY_DFLT        0x0
#define SBL_CONFIG_DFE_TIMEOUT_MS_DFLT       5000
#define SBL_CONFIG_DFE_RETRY_CNT_DFLT        2
#define SBL_CONFIG_NRZ_MIN_EYE_HEIGHT_DFLT   0xc0
#define SBL_CONFIG_PAM4_MIN_EYE_HEIGHT_DFLT  0x10

#define SBL_PORT_CONFIG_LINK_PARTNER_DFLT         0
#define SBL_PORT_CONFIG_TUNING_PATTERN_DFLT       0
#define SBL_PORT_CONFIG_LOOPBACK_MODE_DFLT        0
#define SBL_PORT_CONFIG_LINK_MODE_DFLT            0

// Provided by SI
#define SBL_ENC_NRZ                 SPICO_INT_DATA_ENC_NRZ
#define SBL_DIV_25G                 165
#define SBL_WID_25G                 SPICO_INT_DATA_WIDTH_25G
#define SBL_ENC_PAM4                SPICO_INT_DATA_ENC_PAM4
#define SBL_DIV_50G                 170
#define SBL_WID_50G                 SPICO_INT_DATA_WIDTH_50G
#define SBL_DIV_AN                  8
#define SBL_WID_AN                  SPICO_INT_DATA_WIDTH_2_5G

#define SBL_RX_TERM_AGND            0
#define SBL_RX_TERM_AVDD            1
#define SBL_RX_TERM_FLOAT           2

#define SBL_DS_CORE                 0
#define SBL_DS_PRBS                 1

#define SBL_SERDES_REFCLK           126250000

#define SBL_SBUS_DIVIDER_DFLT       3 // clock divider of 8
#define SBL_SBUS_DIVIDER_SPEEDUP    1 // clock divider of 2

#define SPICO_SBR_DATA_NONE                0x00000000
#define SPICO_SBR_DATA_SET_BURST_WR        0x80000000
#define SPICO_SBR_DATA_RESET_HIGH          0x000000c0
#define SPICO_SBR_DATA_RESET_LOW           0x00000040
#define SPICO_SBR_DATA_IMEM_CNTL_EN        0x00000240
#define SPICO_SBR_DATA_IMEM_CNTL_EN_RD     0x00000340
#define SPICO_SBR_DATA_IMEM_CNTL_DIS       0x00000040
#define SPICO_SBR_DATA_ECC_EN              0x000c0000
#define SPICO_SBR_DATA_SPICO_EN            0x00000140
#define SPICO_SBR_DATA_BE_012              0xc0000000
#define SPICO_SBR_DATA_BE_01               0x80000000
#define SPICO_SBR_DATA_BE_0                0x40000000

#define SPICO_SBR_DATA_W0_OFFSET           0
#define SPICO_SBR_DATA_W1_OFFSET           10
#define SPICO_SBR_DATA_W2_OFFSET           20

#define SPICO_SERDES_DATA_SET_GLOBAL_RESET  0x00000011
#define SPICO_SERDES_DATA_CLR_GLOBAL_RESET  0x00000010
#define SPICO_SERDES_DATA_SET_INTR_DIS      0x00000030
#define SPICO_SERDES_DATA_SET_IMEM_CNTL_EN  0xc0000000
#define SPICO_SERDES_DATA_CLR_IMEM_CNTL_EN  0x00000000
#define SPICO_SERDES_DATA_SET_ECC_EN        0x000c0000
#define SPICO_SERDES_DATA_CLR_ECC_ERR       0xc0000000
#define SPICO_SERDES_DATA_SET_SPICO_EN      0x00000002
#define SPICO_SERDES_DATA_SET_INTR_EN       0x00000000
#define SPICO_SERDES_DATA_RESET             0x00000000

#define SPICO_STATE_RESET                   0x00000000
#define SPICO_STATE_PAUSE                   0x00000012
#define SPICO_STATE_ERROR                   0x0000001f
#define SPICO_STATE_MASK                    0x0000001f

#define SPICO_INT_DATA_NONE             0x0000
#define SPICO_INT_DATA_SERDES_EN_OFF    0x0000
#define SPICO_INT_DATA_BR_1_25          0x9008
#define SPICO_INT_DATA_ELB              0x0100
#define SPICO_INT_DATA_ILB              0x0101
#define SPICO_INT_DATA_WM_20NRZ         0x0011
#define SPICO_INT_DATA_TPCE             0x0001 // TX Phase Cal Enable
#define SPICO_INT_DATA_STRUCT_OFFSET    0x8
#define SPICO_INT_DATA_MEMBER_OFFSET    0x0
#define SPICO_INT_DATA_STRUCT_CLTE      0x09
#define SPICO_INT_DATA_HF_MIN           0x07
#define SPICO_INT_DATA_HF_MAX           0x08
#define SPICO_INT_DATA_SET_TX_EN        0x0001
#define SPICO_INT_DATA_SET_RX_EN        0x0002
#define SPICO_INT_DATA_SET_TXO_EN       0x0004
#define SPICO_INT_DATA_ENC_NRZ          0x0000
#define SPICO_INT_DATA_ENC_PAM4         0x0088
#define SPICO_INT_DATA_WIDTH_2_5G       0x0022
#define SPICO_INT_DATA_WIDTH_25G        0x0033 // width-mode: 40
#define SPICO_INT_DATA_WIDTH_50G        0x0077 // width-mode: 80
#define SPICO_INT_DATA_TX_OVERRIDE      0x0020
#define SPICO_INT_DATA_RX_EID_EN        0x0040
#define SPICO_INT_DATA_SET_TXINV        0x0101
#define SPICO_INT_DATA_CLR_TXINV        0x0100
#define SPICO_INT_DATA_SET_RXINV        0x0210
#define SPICO_INT_DATA_CLR_RXINV        0x0200
#define SPICO_INT_DATA_SET_PRECODE      0x0344
#define SPICO_INT_DATA_CLR_PRECODE      0x0300
#define SPICO_INT_DATA_SET_GRAY_SWZ     0x0caa
#define SPICO_INT_DATA_CLR_GRAY_SWZ     0x0c00
#define SPICO_INT_DATA_RXT_AGND         0x0000
#define SPICO_INT_DATA_RXT_AVDD         0x0001
#define SPICO_INT_DATA_RXT_FLOAT        0x0002
#define SPICO_INT_DATA_SET_LOOPBACK     0x0101
#define SPICO_INT_DATA_CLR_LOOPBACK     0x0100
#define SPICO_INT_DATA_PRBS31_AS_TXGEN  0x0125
#define SPICO_INT_DATA_PRBS31_AS_RXGEN  0x0235
#define SPICO_INT_DATA_DISABLE_TXRXGEN  0x03ff
#define SPICO_INT_DATA_TXTX_RC_NOT_SS   0x9000
#define SPICO_INT_DATA_TXRX_FC_IGNORE   0x8800
#define SPICO_INT_DATA_DFE_ICAL         0x0001
#define SPICO_INT_DATA_DFE_CONT_PCAL    0x0006
#define SPICO_INT_DATA_DFE_PAUSE_PCAL   0x0002
#define SPICO_INT_DATA_DFE_READ_PARAMS  0x5b00
#define SPICO_INT_DATA_SET_TXEQ_PRE1    0x0000
#define SPICO_INT_DATA_SET_TXEQ_PRE2    0xc000
#define SPICO_INT_DATA_SET_TXEQ_PRE3    0x3000
#define SPICO_INT_DATA_SET_TXEQ_POST    0x8000
#define SPICO_INT_DATA_SET_TXEQ_ATTEN   0x4000
#define SPICO_INT_DATA_GET_TXEQ_PRE1    0x0100
#define SPICO_INT_DATA_GET_TXEQ_PRE2    0xc100
#define SPICO_INT_DATA_GET_TXEQ_PRE3    0x3100
#define SPICO_INT_DATA_GET_TXEQ_POST    0x8100
#define SPICO_INT_DATA_GET_TXEQ_ATTEN   0x4100
#define SPICO_INT_DATA_TXEQ_DATA_MASK   0x00ff
#define SPICO_INT_DATA_RXEQ_VAL_MASK    0x00ff
#define SPICO_INT_DATA_PRBS_SUCCESS     0x0002
#define SPICO_INT_DATA_PRBS_RETRY       0x0000
#define SPICO_INT_DATA_PROC_RESET       0x0001

#define SPICO_INT_DATA_RXEQ_STS_DFE_ST  0x0a00
#define SPICO_INT_DATA_RXEQ_STS_DFE_STS 0x0b00
#define SPICO_INT_DATA_RXEQ_FFE_PRE2    0x1000
#define SPICO_INT_DATA_RXEQ_FFE_PRE1    0x1100
#define SPICO_INT_DATA_RXEQ_FFE_POST1   0x1200
#define SPICO_INT_DATA_RXEQ_FFE_BFLF    0x1300
#define SPICO_INT_DATA_RXEQ_FFE_BFHF    0x1400
#define SPICO_INT_DATA_RXEQ_FFE_DRATE   0x1500
#define SPICO_INT_DATA_RXEQ_CTLE_HF     0x2000
#define SPICO_INT_DATA_RXEQ_CTLE_LF     0x2100
#define SPICO_INT_DATA_RXEQ_CTLE_BW     0x2200
#define SPICO_INT_DATA_RXEQ_CTLE_GS1    0x2400
#define SPICO_INT_DATA_RXEQ_CTLE_GS2    0x2500
#define SPICO_INT_DATA_RXEQ_DFE_GAIN    0x3000
#define SPICO_INT_DATA_RXEQ_DFE_GAIN2   0x3100
#define SPICO_INT_DATA_RXEQ_DFE_2       0x3200
#define SPICO_INT_DATA_RXEQ_DFE_3       0x3300
#define SPICO_INT_DATA_RXEQ_DFE_4       0x3400
#define SPICO_INT_DATA_RXEQ_DFE_5       0x3500
#define SPICO_INT_DATA_RXEQ_DFE_6       0x3600
#define SPICO_INT_DATA_RXEQ_DFE_7       0x3700
#define SPICO_INT_DATA_RXEQ_DFE_8       0x3800
#define SPICO_INT_DATA_RXEQ_DFE_9       0x3900
#define SPICO_INT_DATA_RXEQ_DFE_A       0x3a00
#define SPICO_INT_DATA_RXEQ_DFE_B       0x3b00
#define SPICO_INT_DATA_RXEQ_DFE_C       0x3c00
#define SPICO_INT_DATA_RXEQ_EH_THLE     0x4000
#define SPICO_INT_DATA_RXEQ_EH_THME     0x4100
#define SPICO_INT_DATA_RXEQ_EH_THUE     0x4200
#define SPICO_INT_DATA_RXEQ_EH_THLO     0x4300
#define SPICO_INT_DATA_RXEQ_EH_THMO     0x4400
#define SPICO_INT_DATA_RXEQ_EH_THUO     0x4500

#define SPICO_INT_DATA_RXEQ_SIGN_MASK   0x8000
#define SPICO_INT_DATA_RXEQ_VAL_MASK    0x00ff

#define SPICO_INT_DATA_HAL_DFE_APPLY    0x0018
#define SPICO_INT_DATA_HAL_CTLE_APPLY   0x0015
#define SPICO_INT_DATA_HAL_FFE_APPLY    0x0012
#define SPICO_INT_DATA_HAL_RXV_APPLY    0x0003
#define SPICO_INT_DATA_HAL_DC_APPLY     0x000a
#define SPICO_INT_DATA_HAL_TC_APPLY     0x000d
#define SPICO_INT_DATA_HAL_PCAL_SETUP   0x003f
#define SPICO_INT_DATA_HAL_CTLE_HF      0x0900
#define SPICO_INT_DATA_HAL_CTLE_LF      0x0901
#define SPICO_INT_DATA_HAL_CTLE_DC      0x0902
#define SPICO_INT_DATA_HAL_CTLE_BW      0x0903
#define SPICO_INT_DATA_HAL_CTLE_GS1     0x0904
#define SPICO_INT_DATA_HAL_CTLE_GS2     0x0905
#define SPICO_INT_DATA_HAL_CTLE_SCE     0x0906
#define SPICO_INT_DATA_HAL_FFE_PRE2     0x0d00
#define SPICO_INT_DATA_HAL_FFE_PRE1     0x0d01
#define SPICO_INT_DATA_HAL_FFE_POST1    0x0d02
#define SPICO_INT_DATA_HAL_FFE_4        0x0d03
#define SPICO_INT_DATA_HAL_FFE_5        0x0d04
#define SPICO_INT_DATA_HAL_FFE_6        0x0d05
#define SPICO_INT_DATA_HAL_FFE_7        0x0d06
#define SPICO_INT_DATA_HAL_DFE_GT1      0x0b00
#define SPICO_INT_DATA_HAL_DFE_GT2      0x0b01
#define SPICO_INT_DATA_HAL_DFE_G2       0x0b02
#define SPICO_INT_DATA_HAL_DFE_G3       0x0b03
#define SPICO_INT_DATA_HAL_DFE_G4       0x0b04
#define SPICO_INT_DATA_HAL_DFE_G5       0x0b05
#define SPICO_INT_DATA_HAL_DFE_G6       0x0b06
#define SPICO_INT_DATA_HAL_DFE_G7       0x0b07
#define SPICO_INT_DATA_HAL_DFE_G8       0x0b08
#define SPICO_INT_DATA_HAL_DFE_G9       0x0b09
#define SPICO_INT_DATA_HAL_DFE_G10      0x0b0a
#define SPICO_INT_DATA_HAL_DFE_G11      0x0b0b
#define SPICO_INT_DATA_HAL_DFE_G12      0x0b0c
#define SPICO_INT_DATA_HAL_DFE_G13      0x0b0d
#define SPICO_INT_DATA_HAL_GTP_CTLE_FX  0x0108
#define SPICO_INT_DATA_HAL_GTP_FFE_FX   0x0109
#define SPICO_INT_DATA_HAL_GTP_DFE_FX   0x010a
#define SPICO_INT_DATA_HAL_GTP_VER_FX   0x0115
#define SPICO_INT_DATA_HAL_CTLE_BASE    0x0900
#define SPICO_INT_DATA_HAL_FFE_BASE     0x0d00
#define SPICO_INT_DATA_HAL_DFE_BASE     0x0b00
#define SPICO_INT_DATA_HAL_RXVS_BASE    0x0300 /* Rx clock vernier settings */
#define SPICO_INT_DATA_HAL_RXVS_OFFSET  0x0005 /* Rx clock vernier settings */
#define SPICO_INT_DATA_HAL_RXVC_BASE    0x2f00 /* Rx clock vernier cals */
#define SPICO_INT_DATA_HAL_RSDO_BASE    0x0f00
#define SPICO_INT_DATA_HAL_RSDC_BASE    0x1100
#define SPICO_INT_DATA_HAL_RSTO_BASE    0x1200
#define SPICO_INT_DATA_HAL_RSTC_BASE    0x1400
#define SPICO_INT_DATA_HAL_EH_BASE      0x1700
#define SPICO_INT_DATA_HAL_GTP_BASE     0x0100
#define SPICO_INT_DATA_HAL_DCCD_BASE    0x2c00
#define SPICO_INT_DATA_HAL_P4LV_BASE    0x1f00

#define SPICO_INT_DATA_ICAL_EFFORT_SEL  0x0118
#define SPICO_INT_DATA_ICAL_EFFORT_0    0x0000 /* Min effort */
#define SPICO_INT_DATA_ICAL_EFFORT_1    0x0001 /* Max effort */
#define SPICO_INT_DATA_ICAL_EFFORT_2    0x0002 /* Min effort w/o vernier rst */
#define SPICO_INT_DATA_ICAL_EFFORT_10   0x0010 /* Med effort */

#define SPICO_INT_DATA_ICAL_MAX_EFFORT  SPICO_INT_DATA_ICAL_EFFORT_1
#define SPICO_INT_DATA_ICAL_MED_EFFORT  SPICO_INT_DATA_ICAL_EFFORT_10
#define SPICO_INT_DATA_ICAL_MIN_EFFORT  SPICO_INT_DATA_ICAL_EFFORT_0

#define SPICO_INT_DATA_EID_FILTER_SEL   0x011c
#define SPICO_INT_DATA_EID_FILTER_DFE   0x0155
#define SPICO_INT_DATA_EID_FILTER_OFF   0x0000

#define SBUS_AFE_CTRL_KNOWN_FW0_BASE     0x039e /* Different for each FW build */

#define TXEQ_PRE1_MIN                   -31
#define TXEQ_PRE1_MAX                   31
#define TXEQ_PRE2_MIN                   -31
#define TXEQ_PRE2_MAX                   31
#define TXEQ_PRE3_MIN                   -1
#define TXEQ_PRE3_MAX                   1
#define TXEQ_POST_MIN                   -31
#define TXEQ_POST_MAX                   31
#define TXEQ_ATTEN_MIN                  0
#define TXEQ_ATTEN_MAX                  31
#define RXEQ_DFE_GS1_MIN                0
#define RXEQ_DFE_GS1_MAX                3
#define RXEQ_DFE_GS2_MIN                0
#define RXEQ_DFE_GS2_MAX                3

#define DFE_LOS_MASK                    0x0200
#define DFE_CAL_RUN_IN_PRGRS_MASK       0x0037
#define DFE_CAL_DONE                    0x0080

#define SERDES_MEM_ADDR_O_CORE_STATUS   0x26

#define SPICO_INT_DATA_CTLE_MIN         ((SPICO_INT_DATA_STRUCT_CLTE << SPICO_INT_DATA_STRUCT_OFFSET) | (SPICO_INT_DATA_HF_MIN << SPICO_INT_DATA_MEMBER_OFFSET))
#define SPICO_INT_DATA_CTLE_HF_MIN_VAL  0x3
#define SPICO_INT_DATA_CTLE_MAX         ((SPICO_INT_DATA_STRUCT_CLTE << SPICO_INT_DATA_STRUCT_OFFSET) | (SPICO_INT_DATA_HF_MAX << SPICO_INT_DATA_MEMBER_OFFSET))
#define SPICO_INT_DATA_CTLE_HF_MAX_VAL  0x7

#define SPICO_INT_DATA_RXP_APPLY_OFFSET 15
#define SPICO_INT_DATA_RX_PHASE_OFFSET  8
#define SPICO_INT_DATA_RX_PHASE_MAX     0x3f


#define SPICO_RESULT_SERDES_CRC_PASS    0
#define SPICO_RESULT_SERDES_CRC_FAIL    0xff

#define SPICO_RESULT_SBR_CRC_PASS       1
#define SPICO_RESULT_SBR_CRC_FAIL       0xffff

#define SBUS_INT_LOOP_LIMIT             2000
#define SERDES_EN_LOOP_LIMIT            20
#define SERDES_EN_LOOP_DELAY            1

#define SERDES_CORE_STATUS_TX_RDY_MASK		0x400000000
#define SERDES_CORE_STATUS_RX_RDY_MASK		0x200000000
#define SERDES_CORE_STATUS_RX_IDLE_DETECT_MASK  0x100000000
#define SERDES_CORE_STATUS_CORE_STS_MASK	0x0ffffffff
#define SERDES_CORE_STATUS_RX_SIG_OK_MASK       0x000000010
#define SERDES_CORE_STATUS_SPICO_READY_MASK     0x000000020


#define TUNE_DATA_PRBS                  0
#define TUNE_DATA_CORE                  1
#define TUNE_DATA                       TUNE_DATA_CORE

#define BDC_SERDES_RATE_UNINITIALIZED   0
#define BDC_SERDES_RATE_DOWN_MIN        (BDC_SERDES_RATE_UNINITIALIZED + 1)
#define BDC_SERDES_RATE_DOWN_MAX        1000
#define BDC_SERDES_RATE_25G_NRZ_MIN     (BDC_SERDES_DOWN_RATE_MAX + 1)
#define BDC_SERDES_RATE_25G_NRZ_MAX     30000
#define BDC_SERDES_RATE_50G_PAM4_MIN    (BDC_SERDES_RATE_25G_NRZ_MAX + 1)
#define BDC_SERDES_RATE_50G_PAM4_MAX    60000

#define ROS_ETH_SERDES_SBUS_BCAST       0xee

#define ROSHMS_PORT_UNAVAILABLE         0
#define ROSHMS_PORT_AVAILABLE           1

#define NUM_PAM4_EYES                   6
#define NUM_NRZ_EYES                    1

#define SBL_BER_DWELL               2
#define SBL_CORE_STATUS_RD_RETRY_DELAY  1

#define SPICO_INT_DATA_PRBS_RETRY_LIMIT 8
#define SPICO_INT_DATA_PRBS_RETRY_DELAY 1

#define SPICO_PROC_RESET_RETRY_TIMEOUT_MS 2000

#define SERDES_POST_FLASH_READY_CNT     50

#ifdef __cplusplus
}
#endif
#endif /* _SBL_CONSTANTS_H_ */
