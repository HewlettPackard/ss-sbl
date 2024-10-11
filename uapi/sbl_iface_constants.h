/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2020,2022-2023 Hewlett Packard Enterprise Development LP. All rights reserved. */

/*
 * sbl_constants.h
 */
#ifndef _SBL_IFACE_CONSTANTS_H_
#define _SBL_IFACE_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Port Macro constants */
#define SBL_SERDES_LANES_PER_PORT            4

/* General constants */
#define SBL_MAX_STR_LEN                      256
#define LEVEL_ERR                            4
#define LEVEL_WARN                           3
#define LEVEL_INFO                           2
#define LEVEL_DBG                            1
	
/* SBus address conversion macros */
#define SBUS_ADDR(ring, rx_addr)        ((ring << 8) | rx_addr)
#define SBUS_RING(sbus_addr)            ((sbus_addr & 0x300) >> 8)
#define SBUS_RX_ADDR(sbus_addr)         (sbus_addr & 0xff)

/* SBus operation defines */
#define SBUS_CMD_MODE_OFFSET             7
#define SBUS_CMD_MODE_MASK               0x1 << SBUS_CMD_MODE_OFFSET
#define SBUS_IFACE_DST_OFFSET            5
#define SBUS_IFACE_DST_MASK              0x3 << SBUS_IFACE_DST_OFFSET
#define SBUS_CMD_OFFSET                  0
#define SBUS_CMD_MASK                    0xf << SBUS_CMD_OFFSET

#define SBUS_CMD_MODE_CTLR               1 << SBUS_CMD_MODE_OFFSET
#define SBUS_CMD_MODE_RCVR               0 << SBUS_CMD_MODE_OFFSET
#define SBUS_IFACE_DST_TAP               0 << SBUS_IFACE_DST_OFFSET
#define SBUS_IFACE_DST_CORE              1 << SBUS_IFACE_DST_OFFSET
#define SBUS_IFACE_DST_SPICO             2 << SBUS_IFACE_DST_OFFSET
#define SBUS_IFACE_DST_SPARE             3 << SBUS_IFACE_DST_OFFSET
#define SBUS_CMD_RESET                   0 << SBUS_CMD_OFFSET
#define SBUS_CMD_WRITE                   1 << SBUS_CMD_OFFSET
#define SBUS_CMD_READ                    2 << SBUS_CMD_OFFSET
#define SBUS_CMD_READ_RESULT             3 << SBUS_CMD_OFFSET

#define SBUS_RC_RESET                    0
#define SBUS_RC_WRITE_COMPLETE           1 
#define SBUS_RC_WRITE_FAILED             2
#define SBUS_RC_READ_ALL_COMPLETE        3
#define SBUS_RC_READ_COMPLETE            4
#define SBUS_RC_READ_FAILED              5
#define SBUS_RC_CMD_ISSUE_DONE           6
#define SBUS_RC_MODE_CHANGE_COMPLETE     7

// Rosetta
#define SBUS_RING0_CORE_PLL              0x1
#define SBUS_RING0_PCIE_PLL              0x2
#define SBUS_RING0_PCIE_SERDES_SPICO     0x3
#define SBUS_RING0_PCIE_PCS              0x4
#define SBUS_RING0_CM4_SERDES_FIRST      0x5
#define SBUS_RING0_PMRO0                 0x15
#define SBUS_RING0_THERMVOLT             0x16
#define SBUS_RING0_PMRO1                 0x57
#define SBUS_RING0_CM4_SERDES_LAST       0x87
#define SBUS_RING0_SBM0                  0x88
#define SBUS_RING0_SBM0_SPICO            0x89

#define SBUS_RING1_CM4_SERDES_FIRST      0x1
#define SBUS_RING1_PMRO2                 0x11
#define SBUS_RING1_PMRO3                 0x52
#define SBUS_RING1_CM4_SERDES_LAST       0x82
#define SBUS_RING1_SBM1                  0x83
#define SBUS_RING1_SBM1_SPICO            0x84

// Cassini
#define SBUS_RINGX_PCIE_PLL0                0x1
#define SBUS_RINGX_PCIE_PLL1                0x2
#define SBUS_RINGX_PCIE_PLL2                0x3
#define SBUS_RINGX_CORE_PLL                 0x4
#define SBUS_RINGX_THERMVOLT                0x5
#define SBUS_RINGX_PMRO0                    0x6
#define SBUS_RINGX_PMRO1                    0x7
#define SBUS_RINGX_PCIE_SERDES_SPICO_FIRST  0x8
#define SBUS_RINGX_PCIE_SERDES_SPICO_LAST   0x26
#define SBUS_RINGX_PCIE_SERDES_PCS_FIRST    0x9
#define SBUS_RINGX_PCIE_SERDES_PCS_LAST     0x27
#define SBUS_RINGX_CM4_SERDES_FIRST         0x28
#define SBUS_RINGX_CM4_SERDES_LAST          0x2B
#define SBUS_RINGX_SBM                      0x2C
#define SBUS_RINGX_SBM_SPICO                0x2D
#define SBUS_RINGX_PCIE_SERDES_SPICO_LSB    0x0
#define SBUS_RINGX_PCIE_SERDES_PCS_LSB      0x1

// Broadcast
#define SBUS_BCAST_PCIE_SERDES_SPICO     0xff
#define SBUS_BCAST_PCIE_SERDES_PCS       0xf7
#define SBUS_BCAST_CM4_SERDES_SPICO      0xee
#define SBUS_BCAST_SBM_SPICO             0xfd
#define SBUS_BCAST_SBM                   0xfe
#define SBUS_BCAST_PLL                   0xf6
#define SBUS_BCAST_PMRO                  0xf5
#define SBUS_BCAST_THERMVOLT             0xef

// SBus receiver interface
#define SPICO_SBR_ADDR_SRAM_BIST           0x0
#define SPICO_SBR_ADDR_CTL                 0x1
#define SPICO_SBR_ADDR_DMEM_IN             0x2
#define SPICO_SBR_ADDR_IMEM                0x3
#define SPICO_SBR_ADDR_DMEM                0x4
#define SPICO_SBR_ADDR_STEP_BP             0x5
#define SPICO_SBR_ADDR_BP_ADDR             0x6
#define SPICO_SBR_ADDR_INTR                0x7
#define SPICO_SBR_ADDR_DMEM_OUT            0x8
#define SPICO_SBR_ADDR_RDATA               0x9
#define SPICO_SBR_ADDR_PC                  0xa
#define SPICO_SBR_ADDR_PC_OVERRIDE         0xb
#define SPICO_SBR_ADDR_FLAG                0xc
#define SPICO_SBR_ADDR_OP                  0xd
#define SPICO_SBR_ADDR_SP_EC               0xe
#define SPICO_SBR_ADDR_STATE               0xf
#define SPICO_SBR_ADDR_RESULT              0x10
#define SPICO_SBR_ADDR_A_B_REG             0x11
#define SPICO_SBR_ADDR_IMEM_BURST_DATA     0x14
#define SPICO_SBR_ADDR_IMEM_BURST_ADDR     0x15
#define SPICO_SBR_ADDR_ECC                 0x16
#define SPICO_SBR_ADDR_UNNAMED_0           0x17
#define SPICO_SBR_ADDR_UNNAMED_1           0x18
#define SPICO_SBR_ADDR_UNNAMED_2           0x19
#define SPICO_SBR_ADDR_UNNAMED_3           0x1a
#define SPICO_SBR_ADDR_UNNAMED_4           0x1b
#define SPICO_SBR_ADDR_IP_IDCODE           0xff

// SBus Controller receiver memory
#define SBM_CRM_ADDR_CISM0             0x0
#define SBM_CRM_ADDR_CISM1             0x1
#define SBM_CRM_ADDR_LAST_ADDR         0x2
#define SBM_CRM_ADDR_CLK_DIV           0xa
#define SBM_CRM_ADDR_CLK_DIV_RST       0xb
#define SBM_CRM_ADDR_CLK_DIV_RST_P0    0xc
#define SBM_CRM_ADDR_CLK_DIV_RST_P1    0xd
#define SBM_CRM_ADDR_CISM_RX_ADDR      0x10
#define SBM_CRM_ADDR_CISM_CMD1         0x11
#define SBM_CRM_ADDR_CISM_DATA_ADDR1   0x12
#define SBM_CRM_ADDR_CISM_DATA_WORD1   0x13
#define SBM_CRM_ADDR_CISM_CMD2         0x14
#define SBM_CRM_ADDR_CISM_DATA_ADDR2   0x15
#define SBM_CRM_ADDR_CISM_DATA_WORD2   0x16
#define SBM_CRM_ADDR_DATA_OUT_SEL      0x20
#define SBM_CRM_ADDR_BIST              0x21
#define SBM_CRM_ADDR_PROC_STS          0x2a
#define SBM_CRM_ADDR_ROM_EN            0x30
#define SBM_CRM_ADDR_ROM_FAILED_ADDR   0x31
#define SBM_CRM_ADDR_ROM_DATA_VAL_CNT  0x32
#define SBM_CRM_ADDR_ROM_ACK           0x33
#define SBM_CRM_ADDR_ROM_STOP_ADDR     0x34
#define SBM_CRM_ADDR_ROM_OUTPUT        0x35
#define SBM_CRM_ADDR_ROM_DATA0         0x36
#define SBM_CRM_ADDR_ROM_DATA1         0x37
#define SBM_CRM_ADDR_CLK_HALT          0x40
#define SBM_CRM_ADDR_GEN_WRITE         0x80
#define SBM_CRM_ADDR_GEN_READ          0x81
#define SBM_CRM_ADDR_GEN_WRITE_P0      0x82
#define SBM_CRM_ADDR_GEN_WRITE_P1      0x83
#define SBM_CRM_ADDR_GEN_WRITE_P2      0x84
#define SBM_CRM_ADDR_GEN_WRITE_P3      0x85
#define SBM_CRM_ADDR_GEN_READ_P0       0x86
#define SBM_CRM_ADDR_GEN_READ_P1       0x87
#define SBM_CRM_ADDR_GEN_READ_P2       0x88
#define SBM_CRM_ADDR_GEN_READ_P3       0x89
#define SBM_CRM_ADDR_SBUS_ID           0xfe
#define SBM_CRM_ADDR_IP_IDCODE         0xff

#define SPICO_SBR_DATA_W0_OFFSET           0
#define SPICO_SBR_DATA_W1_OFFSET           10
#define SPICO_SBR_DATA_W2_OFFSET           20

#define SPICO_SERDES_ADDR_IMEM               0x0
#define SPICO_SERDES_ADDR_INTR0              0x3
#define SPICO_SERDES_ADDR_INTR1              0x4
#define SPICO_SERDES_ADDR_RESET_EN           0x7
#define SPICO_SERDES_ADDR_INTR_DIS           0x8
#define SPICO_SERDES_ADDR_BIST               0x9
#define SPICO_SERDES_ADDR_IMEM_BURST         0xa
#define SPICO_SERDES_ADDR_ECC                0xb
#define SPICO_SERDES_ADDR_ECCLOG             0xc
#define SPICO_SERDES_ADDR_BCAST              0xfd
#define SPICO_SERDES_ADDR_IP_IDCODE          0xff

// See Spico_Firmware_2002.pdf
// #define SPICO_INT_SBMC_REV_ID            0x00
// #define SPICO_INT_SBMC_BUILD_ID          0x01
// #define SPICO_INT_SBMC_GET_PMRO_DATA     0x02
// #define SPICO_INT_SBMC_GET_TEMP_DATA     0x03
// #define SPICO_INT_SBMC_GET_VOLT_DATA     0x04

#define SPICO_INT_SBMS_REV_ID            0x00
#define SPICO_INT_SBMS_BUILD_ID          0x01
#define SPICO_INT_SBMS_DO_CRC            0x02
#define SPICO_INT_SBMS_READ_DMEM_VAL     0x03
#define SPICO_INT_SBMS_DO_XDMEM_CRC      0x04
#define SPICO_INT_SBMS_GET_PMRO_DATA     0x16
#define SPICO_INT_SBMS_GET_TEMP_DATA     0x17
#define SPICO_INT_SBMS_GET_VOLT_DATA     0x18
#define SPICO_INT_SBMS_IMEM_CRC_CHECK    0x1a
#define SPICO_INT_SBMS_IMEM_SWP_SA       0x1c
#define SPICO_INT_SBMS_FW_STS            0x20
#define SPICO_INT_SBMS_PCIE3_SWP         0x28
#define SPICO_INT_SBMS_DRMON_SETUP       0x29
#define SPICO_INT_SBMS_TEMP_SETUP        0x2a
#define SPICO_INT_SBMS_RR_PCAL           0x2b
#define SPICO_INT_SBMS_DDR_HMB_FIRST     0x30
#define SPICO_INT_SBMS_DDR_HMB_LAST      0x3f

#define SBMS_INTERRUPT_DATA_OUT_MASK     0xffff0000
#define SBMS_INTERRUPT_IN_PROGRESSS_MASK 0x00008000
#define SBMS_INTERRUPT_STATUS_MASK       0x00007fff
#define SBMS_INTERRUPT_DATA_MASK         0xffff0000
#define SBMS_INTERRUPT_DATA_OFFSET       16
#define SBMS_INTERRUPT_CODE_MASK         0x0000ffff
#define SBMS_INTERRUPT_CODE_OFFSET       0

#define SBMS_INTERRUPT_STATUS_OK         0x1

#define SPICO_INT_CM4_REV_ID             0x00
#define SPICO_INT_CM4_SERDES_EN          0x01
#define SPICO_INT_CM4_PRBS_CTRL          0x02
#define SPICO_INT_CM4_COMPARE_CTRL       0x03
#define SPICO_INT_CM4_PMD_CTRL           0x04
#define SPICO_INT_CM4_TX_BAUD            0x05
#define SPICO_INT_CM4_RX_BAUD            0x06
#define SPICO_INT_CM4_AN_CONFIG          0x07
#define SPICO_INT_CM4_LOOPBACK           0x08
#define SPICO_INT_CM4_DFE_CTRL           0x0a
#define SPICO_INT_CM4_TX_PHASE_CAL       0x0b
#define SPICO_INT_CM4_BIT_SLIP           0x0c
#define SPICO_INT_CM4_TX_PHASE_SLIP      0x0d
#define SPICO_INT_CM4_RX_PHASE_SLIP      0x0e
#define SPICO_INT_CM4_PCIE_PD            0x10
#define SPICO_INT_CM4_PLL_RECAL          0x11
#define SPICO_INT_CM4_SAS_APTA_CTRL      0x12
#define SPICO_INT_CM4_POLARITY_CTRL      0x13
#define SPICO_INT_CM4_WIDTH_MODE         0x14
#define SPICO_INT_CM4_TXEQ_LOAD          0x15
#define SPICO_INT_CM4_ERROR_RST          0x16
#define SPICO_INT_CM4_BIG_REG_SEL        0x18
#define SPICO_INT_CM4_BIG_REG_WR         0x19
#define SPICO_INT_CM4_BIG_REG_RD         0x1a
#define SPICO_INT_CM4_ERR_INJECT         0x1b
#define SPICO_INT_CM4_DO_DATA_CAP        0x1c
#define SPICO_INT_CM4_WAIT_TTD           0x1d
#define SPICO_INT_CM4_ERR_STS            0x1e
#define SPICO_INT_CM4_ERR_TIMER_STS      0x1f
#define SPICO_INT_CM4_PCIE_SLICES        0x20
#define SPICO_INT_CM4_PCIE3_RX_EQ        0x24
#define SPICO_INT_CM4_SET_RX_EQ          0x26
#define SPICO_INT_CM4_GET_RX_EQ          0x126
#define SPICO_INT_CM4_CORE_TO_CNTL_EN    0x27
#define SPICO_INT_CM4_AN_DME_PAGE        0x29
#define SPICO_INT_CM4_INT_RX_TERM        0x2b
#define SPICO_INT_CM4_HAL_READ           0x2c
#define SPICO_INT_CM4_HAL_WRITE          0x6c
#define SPICO_INT_CM4_HAL_CALL           0xec
#define SPICO_INT_CM4_SPICO_CLK_SWP      0x30
#define SPICO_INT_CM4_PCIE3_TX_MRG1      0x31
#define SPICO_INT_CM4_PCIE3_TX_MRG2      0x32
#define SPICO_INT_CM4_PCIE3_TX_MRG3      0x33
#define SPICO_INT_CM4_PCIE3_TX_MRG4      0x34
#define SPICO_INT_CM4_PCIE3_TX_MRG5      0x35
#define SPICO_INT_CM4_PROC_RESET         0x39
#define SPICO_INT_CM4_PCIE_SWP_SETUP     0x3a
#define SPICO_INT_CM4_IP_STAT_INFO       0x3b
#define SPICO_INT_CM4_CRC                0x3c
#define SPICO_INT_CM4_PMD_CONFIG         0x3d
#define SPICO_INT_CM4_BUILD_ID           0x3f
#define SPICO_INT_CM4_O_CORE_STS_15_0    0x4027
#define SPICO_INT_CM4_O_CORE_STS_31_16   0x4069
#define SPICO_INT_CM4_MEM_READ           0x4000
#define SPICO_INT_CM4_MEM_WRITE          0x8000

#define SPICO_INT_RESULT_CODE_MASK      0xff
#define SPICO_INT_RETURN_RESULT         2
#define SPICO_INT_VALIDATE_RESULT       1
#define SPICO_INT_IGNORE_RESULT         0
#define SPICO_INT_DIVIDER_MASK          0xff
#define SPICO_INT_MEM_READ_ADDR_MASK    0xfff

#define SPICO_INT_DATA_NONE             0x0000

#define SPICO_SBR_DATA_BE_012              0xc0000000
#define SPICO_SBR_DATA_BE_01               0x80000000
#define SPICO_SBR_DATA_BE_0                0x40000000

// From apd16_IO_databook.pdf
#define SBUS_SENSOR_ADDR_RESET_START                     0
#define SBUS_SENSOR_ADDR_SENSOR_CLK_DIVIDE               1
#define SBUS_SENSOR_ADDR_REMOTE_SENSOR_MODE              3
#define SBUS_SENSOR_ADDR_SPARE_REG                       4
#define SBUS_SENSOR_ADDR_THERM_STATE                     5
#define SBUS_SENSOR_ADDR_TEMP_GAIN_OVRD_VAL              6
#define SBUS_SENSOR_ADDR_TEMP_OFFSET_OVRD_VAL            7
#define SBUS_SENSOR_ADDR_TEMP_C_RES_OVRD_VAL             8
#define SBUS_SENSOR_ADDR_TEMP_GAIN_TRIM_OVRD_VAL         9
#define SBUS_SENSOR_ADDR_TEMP_OFFSET_TRIM_OVRD_VAL       10
#define SBUS_SENSOR_ADDR_TEMP_OVRD_SEL                   11
#define SBUS_SENSOR_ADDR_VOLTAGE_GAIN_OVRD_VAL           12
#define SBUS_SENSOR_ADDR_VOLTAGE_OFFSET_OVRD_VAL         13
#define SBUS_SENSOR_ADDR_VOLTAGE_C_RES_OVRD_VAL          14
#define SBUS_SENSOR_ADDR_VOLTAGE_GAIN_TRIM_OVRD_VAL      15
#define SBUS_SENSOR_ADDR_VOLTAGE_OVRD_SEL                16
#define SBUS_SENSOR_ADDR_REMOTE_GAIN_OVRD_VAL            17
#define SBUS_SENSOR_ADDR_REMOTE_OFFSET_OVRD_VAL          18
#define SBUS_SENSOR_ADDR_REMOTE_C_RES_OVRD_VAL           19
#define SBUS_SENSOR_ADDR_REMOTE_OVRD_SEL                 20
#define SBUS_SENSOR_ADDR_ACCUMULATOR                     64
#define SBUS_SENSOR_ADDR_TEMPERATURE_SENSOR              65
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_0     66
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_1     67
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_2     68
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_3     69
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_4     70
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_5     71
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_6     72
#define SBUS_SENSOR_ADDR_REMOTE_TEMPERATURE_SENSOR_7     73
#define SBUS_SENSOR_ADDR_VDD_VOLTAGE_SENSOR              74
#define SBUS_SENSOR_ADDR_CORE_VDD_VIN_VOLTAGE_SENSOR     75
#define SBUS_SENSOR_ADDR_SENSOR_A2D_VIN_0_VOLTAGE_SENSOR 76
#define SBUS_SENSOR_ADDR_SENSOR_A2D_VIN_1_VOLTAGE_SENSOR 77
#define SBUS_SENSOR_ADDR_SENSOR_A2D_VIN_2_VOLTAGE_SENSOR 78
#define SBUS_SENSOR_ADDR_SENSOR_A2D_VIN_3_VOLTAGE_SENSOR 79
#define SBUS_SENSOR_ADDR_SENSOR_A2D_VIN_4_VOLTAGE_SENSOR 80
#define SBUS_SENSOR_ADDR_SENSOR_A2D_VIN_5_VOLTAGE_SENSOR 81
#define SBUS_SENSOR_ADDR_IP_IDENTIFICATION_REGISTER      255

#ifdef __cplusplus
}
#endif
#endif /* _SBL_IFACE_CONSTANTS_H_ */
