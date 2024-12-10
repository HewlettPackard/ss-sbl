// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019, 2021, 2024 Hewlett Packard Enterprise Development LP. All Rights Reserved. */

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#else
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#define msleep(val) (usleep(val * 1000))
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif /*  __KERNEL__ */

#include "uapi/sbl_iface_constants.h"
#include "sbl_sbm_serdes_iface.h"
#include "sbl_serdes_map.h"
#include "sbl_iface.h"

#ifdef __KERNEL__
#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void sbus_msg(void *inst, u32 sbus_addr, u32 req_data,
		     u8 reg_addr, u8 command, u32 rsp_data,
		     u8 result_code, u8 overrun, int timeout,
		     u32 flags, int rc, int severity) __maybe_unused;
static void spico_interrupt_to_string(u32 sbus_addr, u16 interrupt,
				      char *intr_str) __maybe_unused;
#endif /* CONFIG_SBL_PLATFORM_CAS_EMU && !CONFIG_SBL_PLATFORM_CAS_SIM */
#endif /*  __KERNEL__ */

bool
is_cm4_serdes_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	// Valid for Rosetta
	if (((ring == 0) &&
	     (rx_addr != SBUS_RING0_PMRO0) &&
	     (rx_addr != SBUS_RING0_THERMVOLT) &&
	     (rx_addr != SBUS_RING0_PMRO1) &&
	     ((rx_addr >= SBUS_RING0_CM4_SERDES_FIRST) &&
	      (rx_addr <= SBUS_RING0_CM4_SERDES_LAST))) ||
	    ((ring == 1) &&
	     (rx_addr != SBUS_RING1_PMRO2) &&
	     (rx_addr != SBUS_RING1_PMRO3) &&
	     ((rx_addr >= SBUS_RING1_CM4_SERDES_FIRST) &&
	      (rx_addr <= SBUS_RING1_CM4_SERDES_LAST))) ||
	    (rx_addr == SBUS_BCAST_CM4_SERDES_SPICO)) {
		return true;
	}
	return false;
#else
	// Valid for Cassini
	if (((ring == 0 || ring == 1) &&
	     ((rx_addr >= SBUS_RINGX_CM4_SERDES_FIRST) &&
	      (rx_addr <= SBUS_RINGX_CM4_SERDES_LAST))) ||
	    (rx_addr == SBUS_BCAST_CM4_SERDES_SPICO)) {
		return true;
	}
	return false;
#endif
}

bool
is_pcie_serdes_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	// Valid for Rosetta
	if ((ring == 0) && (rx_addr == SBUS_RING0_PCIE_SERDES_SPICO))
		return true;

	return false;
#else
	// Valid for Cassini
	if ((ring == 0 || ring == 1) &&
	     ((rx_addr >= SBUS_RINGX_PCIE_SERDES_SPICO_FIRST) &&
	      (rx_addr >= SBUS_RINGX_PCIE_SERDES_SPICO_FIRST) &&
	      ((rx_addr & 0x1) == SBUS_RINGX_PCIE_SERDES_SPICO_LSB))) {
		return true;
	}
	return false;
#endif
}

bool
is_pcie_serdes_pcs_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	// Valid for Rosetta
	if ((ring == 0) && (rx_addr == SBUS_RING0_PCIE_PCS))
		return true;

	return false;
#else
	// Valid for Cassini
	if ((ring == 0 || ring == 1) &&
	     ((rx_addr >= SBUS_RINGX_PCIE_SERDES_PCS_FIRST) &&
	      (rx_addr <= SBUS_RINGX_PCIE_SERDES_PCS_LAST) &&
	      ((rx_addr & 0x1) == SBUS_RINGX_PCIE_SERDES_PCS_LSB))) {
		return true;
	}
	return false;
#endif
}

bool
is_sbm_crm_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	// Valid for Rosetta
	if (((ring == 0) && (rx_addr == SBUS_RING0_SBM0)) ||
	    ((ring == 1) && (rx_addr == SBUS_RING1_SBM1)) ||
	    (rx_addr == SBUS_BCAST_SBM)) {
		return true;
	}
	return false;
#else
	// Valid for Cassini
	if (((ring == 0 || ring == 1) && (rx_addr == SBUS_RINGX_SBM)) ||
	    (rx_addr == SBUS_BCAST_SBM)) {
		return true;
	}
	return false;
#endif
}

bool
is_sbm_spico_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	// Valid for Rosetta
	if (((ring == 0) && (rx_addr == SBUS_RING0_SBM0_SPICO)) ||
	    ((ring == 1) && (rx_addr == SBUS_RING1_SBM1_SPICO)) ||
	    (rx_addr == SBUS_BCAST_SBM_SPICO)) {
		return true;
	}
	return false;
#else
	// Valid for Cassini
	if (((ring == 0 || ring == 1) && (rx_addr == SBUS_RINGX_SBM_SPICO)) ||
	    (rx_addr == SBUS_BCAST_SBM_SPICO)) {
		return true;
	}
	return false;
#endif
}

#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void
sbus_addr_to_string(u32 sbus_addr, char *data_str)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	// Valid for any sbus master
	if (rx_addr == SBUS_BCAST_PCIE_SERDES_SPICO) {
		sprintf(data_str, "PCIE_SERDES_SPICO(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_PCIE_SERDES_PCS) {
		sprintf(data_str, "PCIE_SERDES_PCS(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_CM4_SERDES_SPICO) {
		sprintf(data_str, "CM4_SERDES_SPICO(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_SBM_SPICO) {
		sprintf(data_str, "SBM_SPICO(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_SBM) {
		sprintf(data_str, "SBM(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_PLL) {
		sprintf(data_str, "PLL(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_PMRO) {
		sprintf(data_str, "PMRO(BROADCAST)");
	} else if (rx_addr == SBUS_BCAST_THERMVOLT) {
		sprintf(data_str, "THERMVOLT(BROADCAST)");
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
    // Valid for Rosetta
	} else if (ring == 0) {
		if (rx_addr == SBUS_RING0_CORE_PLL)
			sprintf(data_str, "CORE_PLL");
		else if (rx_addr == SBUS_RING0_PCIE_PLL)
			sprintf(data_str, "PCIE_PLL");
		else if (rx_addr == SBUS_RING0_PCIE_SERDES_SPICO)
			sprintf(data_str, "PCIE_SERDES_SPICO");
		else if (rx_addr == SBUS_RING0_PCIE_PCS)
			sprintf(data_str, "PCIE_PCS");
		else if (rx_addr == SBUS_RING0_PMRO0)
			sprintf(data_str, "PMRO0");
		else if (rx_addr == SBUS_RING0_THERMVOLT)
			sprintf(data_str, "THERMVOLT");
		else if (rx_addr == SBUS_RING0_PMRO1)
			sprintf(data_str, "PMRO1");
		else if (rx_addr == SBUS_RING0_SBM0)
			sprintf(data_str, "SBM0");
		else if (rx_addr == SBUS_RING0_SBM0_SPICO)
			sprintf(data_str, "SBM0_SPICO");
		else if (is_cm4_serdes_addr(sbus_addr))
			sprintf(data_str, "CM4_SERDES");
		else
			sprintf(data_str, "UNKNOWN");
	} else if (ring == 1) {
		if (rx_addr == SBUS_RING1_PMRO2)
			sprintf(data_str, "PMRO2");
		else if (rx_addr == SBUS_RING1_PMRO3)
			sprintf(data_str, "PMRO3");
		else if (rx_addr == SBUS_RING1_SBM1)
			sprintf(data_str, "SBM1");
		else if (rx_addr == SBUS_RING1_SBM1_SPICO)
			sprintf(data_str, "SBM1_SPICO");
		else if (is_cm4_serdes_addr(sbus_addr))
			sprintf(data_str, "CM4_SERDES");
		else
			sprintf(data_str, "UNKNOWN");
#else
    // Valid for Cassini - rings 0 and 1 are the same
	} else if (ring == 0 || ring == 1) {
		if (rx_addr == SBUS_RINGX_PCIE_PLL0)
			sprintf(data_str, "PCIE_PLL0");
		else if (rx_addr == SBUS_RINGX_PCIE_PLL1)
			sprintf(data_str, "PCIE_PLL1");
		else if (rx_addr == SBUS_RINGX_PCIE_PLL2)
			sprintf(data_str, "PCIE_PLL2");
		else if (rx_addr == SBUS_RINGX_CORE_PLL)
			sprintf(data_str, "CORE_PLL");
		else if (rx_addr == SBUS_RINGX_PMRO0)
			sprintf(data_str, "PMRO0");
		else if (rx_addr == SBUS_RINGX_PMRO1)
			sprintf(data_str, "PMRO1");
		else if (is_pcie_serdes_addr(sbus_addr))
			sprintf(data_str, "PCIE_SERDES_SPICO");
		else if (is_pcie_serdes_pcs_addr(sbus_addr))
			sprintf(data_str, "PCIE_SERDES_PCS0");
		else if (rx_addr == SBUS_RINGX_SBM)
			sprintf(data_str, "SBM");
		else if (rx_addr == SBUS_RINGX_SBM_SPICO)
			sprintf(data_str, "SBM_SPICO");
		else if (is_cm4_serdes_addr(sbus_addr))
			sprintf(data_str, "CM4_SERDES");
		else
			sprintf(data_str, "UNKNOWN");
#endif
	} else {
		sprintf(data_str, "UNKNOWN");
	}
}
#endif

#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void
sbus_reg_addr_to_string(u32 sbus_addr, u8 reg_addr, char *reg_addr_str)
{
	if (is_sbm_spico_addr(sbus_addr)) {
		switch (reg_addr) {
		case SPICO_SBR_ADDR_SRAM_BIST:
			sprintf(reg_addr_str, "SRAM_BIST");
			break;
		case SPICO_SBR_ADDR_CTL:
			sprintf(reg_addr_str, "CTL");
			break;
		case SPICO_SBR_ADDR_DMEM_IN:
			sprintf(reg_addr_str, "DMEM_IN");
			break;
		case SPICO_SBR_ADDR_IMEM:
			sprintf(reg_addr_str, "IMEM");
			break;
		case SPICO_SBR_ADDR_DMEM:
			sprintf(reg_addr_str, "DMEM");
			break;
		case SPICO_SBR_ADDR_STEP_BP:
			sprintf(reg_addr_str, "STEP_BP");
			break;
		case SPICO_SBR_ADDR_BP_ADDR:
			sprintf(reg_addr_str, "BP_ADDR");
			break;
		case SPICO_SBR_ADDR_INTR:
			sprintf(reg_addr_str, "INTR");
			break;
		case SPICO_SBR_ADDR_DMEM_OUT:
			sprintf(reg_addr_str, "DMEM_OUT");
			break;
		case SPICO_SBR_ADDR_RDATA:
			sprintf(reg_addr_str, "RDATA");
			break;
		case SPICO_SBR_ADDR_PC:
			sprintf(reg_addr_str, "PC");
			break;
		case SPICO_SBR_ADDR_PC_OVERRIDE:
			sprintf(reg_addr_str, "PC_OVERRIDE");
			break;
		case SPICO_SBR_ADDR_FLAG:
			sprintf(reg_addr_str, "FLAG");
			break;
		case SPICO_SBR_ADDR_OP:
			sprintf(reg_addr_str, "OP");
			break;
		case SPICO_SBR_ADDR_SP_EC:
			sprintf(reg_addr_str, "SP_EC");
			break;
		case SPICO_SBR_ADDR_STATE:
			sprintf(reg_addr_str, "STATE");
			break;
		case SPICO_SBR_ADDR_RESULT:
			sprintf(reg_addr_str, "RESULT");
			break;
		case SPICO_SBR_ADDR_A_B_REG:
			sprintf(reg_addr_str, "A_B_REG");
			break;
		case SPICO_SBR_ADDR_IMEM_BURST_DATA:
			sprintf(reg_addr_str, "IMEM_BURST_DATA");
			break;
		case SPICO_SBR_ADDR_IMEM_BURST_ADDR:
			sprintf(reg_addr_str, "IMEM_BURST_ADDR");
			break;
		case SPICO_SBR_ADDR_ECC:
			sprintf(reg_addr_str, "ECC");
			break;
		case SPICO_SBR_ADDR_UNNAMED_0:
			sprintf(reg_addr_str, "UNNAMED_0");
			break;
		case SPICO_SBR_ADDR_UNNAMED_1:
			sprintf(reg_addr_str, "UNNAMED_1");
			break;
		case SPICO_SBR_ADDR_UNNAMED_2:
			sprintf(reg_addr_str, "UNNAMED_2");
			break;
		case SPICO_SBR_ADDR_UNNAMED_3:
			sprintf(reg_addr_str, "UNNAMED_3");
			break;
		case SPICO_SBR_ADDR_UNNAMED_4:
			sprintf(reg_addr_str, "UNNAMED_4");
			break;
		default:
			sprintf(reg_addr_str, "SPICO_SBR_UNKNOWN");
			break;
		}
	} else if (is_sbm_crm_addr(sbus_addr)) {
		switch (reg_addr) {
		case SBM_CRM_ADDR_CISM0:
			sprintf(reg_addr_str, "CISM0");
			break;
		case SBM_CRM_ADDR_CISM1:
			sprintf(reg_addr_str, "CISM1");
			break;
		case SBM_CRM_ADDR_LAST_ADDR:
			sprintf(reg_addr_str, "LAST_ADDR");
			break;
		case SBM_CRM_ADDR_CLK_DIV:
			sprintf(reg_addr_str, "CLK_DIV");
			break;
		case SBM_CRM_ADDR_CLK_DIV_RST:
			sprintf(reg_addr_str, "CLK_DIV_RST");
			break;
		case SBM_CRM_ADDR_CLK_DIV_RST_P0:
			sprintf(reg_addr_str, "CLK_DIV_RST_P0");
			break;
		case SBM_CRM_ADDR_CLK_DIV_RST_P1:
			sprintf(reg_addr_str, "CLK_DIV_RST_P1");
			break;
		case SBM_CRM_ADDR_CISM_RX_ADDR:
			sprintf(reg_addr_str, "CISM_RX_ADDR");
			break;
		case SBM_CRM_ADDR_CISM_CMD1:
			sprintf(reg_addr_str, "CISM_CMD1");
			break;
		case SBM_CRM_ADDR_CISM_DATA_ADDR1:
			sprintf(reg_addr_str, "CISM_DATA_ADDR1");
			break;
		case SBM_CRM_ADDR_CISM_DATA_WORD1:
			sprintf(reg_addr_str, "CISM_DATA_WORD1");
			break;
		case SBM_CRM_ADDR_CISM_CMD2:
			sprintf(reg_addr_str, "CISM_CMD2");
			break;
		case SBM_CRM_ADDR_CISM_DATA_ADDR2:
			sprintf(reg_addr_str, "CISM_DATA_ADDR2");
			break;
		case SBM_CRM_ADDR_CISM_DATA_WORD2:
			sprintf(reg_addr_str, "CISM_DATA_WORD2");
			break;
		case SBM_CRM_ADDR_DATA_OUT_SEL:
			sprintf(reg_addr_str, "DATA_OUT_SEL");
			break;
		case SBM_CRM_ADDR_BIST:
			sprintf(reg_addr_str, "BIST");
			break;
		case SBM_CRM_ADDR_ROM_EN:
			sprintf(reg_addr_str, "ROM_EN");
			break;
		case SBM_CRM_ADDR_ROM_FAILED_ADDR:
			sprintf(reg_addr_str, "ROM_FAILED_ADDR");
			break;
		case SBM_CRM_ADDR_ROM_DATA_VAL_CNT:
			sprintf(reg_addr_str, "ROM_DATA_VAL_CNT");
			break;
		case SBM_CRM_ADDR_ROM_ACK:
			sprintf(reg_addr_str, "ROM_ACK");
			break;
		case SBM_CRM_ADDR_ROM_STOP_ADDR:
			sprintf(reg_addr_str, "ROM_STOP_ADDR");
			break;
		case SBM_CRM_ADDR_ROM_OUTPUT:
			sprintf(reg_addr_str, "ROM_OUTPUT");
			break;
		case SBM_CRM_ADDR_ROM_DATA0:
			sprintf(reg_addr_str, "ROM_DATA0");
			break;
		case SBM_CRM_ADDR_ROM_DATA1:
			sprintf(reg_addr_str, "ROM_DATA1");
			break;
		case SBM_CRM_ADDR_CLK_HALT:
			sprintf(reg_addr_str, "CLK_HALT");
			break;
		case SBM_CRM_ADDR_GEN_WRITE:
			sprintf(reg_addr_str, "GEN_WRITE");
			break;
		case SBM_CRM_ADDR_GEN_READ:
			sprintf(reg_addr_str, "GEN_READ");
			break;
		case SBM_CRM_ADDR_GEN_WRITE_P0:
			sprintf(reg_addr_str, "GEN_WRITE_P0");
			break;
		case SBM_CRM_ADDR_GEN_WRITE_P1:
			sprintf(reg_addr_str, "GEN_WRITE_P1");
			break;
		case SBM_CRM_ADDR_GEN_WRITE_P2:
			sprintf(reg_addr_str, "GEN_WRITE_P2");
			break;
		case SBM_CRM_ADDR_GEN_WRITE_P3:
			sprintf(reg_addr_str, "GEN_WRITE_P3");
			break;
		case SBM_CRM_ADDR_GEN_READ_P0:
			sprintf(reg_addr_str, "GEN_READ_P0");
			break;
		case SBM_CRM_ADDR_GEN_READ_P1:
			sprintf(reg_addr_str, "GEN_READ_P1");
			break;
		case SBM_CRM_ADDR_GEN_READ_P2:
			sprintf(reg_addr_str, "GEN_READ_P2");
			break;
		case SBM_CRM_ADDR_GEN_READ_P3:
			sprintf(reg_addr_str, "GEN_READ_P3");
			break;
		case SBM_CRM_ADDR_SBUS_ID:
			sprintf(reg_addr_str, "SBUS_ID");
			break;
		case SBM_CRM_ADDR_IP_IDCODE:
			sprintf(reg_addr_str, "IP_IDCODE");
			break;
		default:
			sprintf(reg_addr_str, "SBM_CRM_UNKNOWN");
			break;
		}
	} else if (is_cm4_serdes_addr(sbus_addr)) {
		switch (reg_addr) {
		case SPICO_SERDES_ADDR_IMEM:
			sprintf(reg_addr_str, "IMEM");
			break;
		case SPICO_SERDES_ADDR_INTR0:
			sprintf(reg_addr_str, "INTR0");
			break;
		case SPICO_SERDES_ADDR_INTR1:
			sprintf(reg_addr_str, "INTR1");
			break;
		case SPICO_SERDES_ADDR_RESET_EN:
			sprintf(reg_addr_str, "RESET_EN");
			break;
		case SPICO_SERDES_ADDR_INTR_DIS:
			sprintf(reg_addr_str, "INTR_DIS");
			break;
		case SPICO_SERDES_ADDR_BIST:
			sprintf(reg_addr_str, "BIST");
			break;
		case SPICO_SERDES_ADDR_IMEM_BURST:
			sprintf(reg_addr_str, "IMEM_BURST");
			break;
		case SPICO_SERDES_ADDR_ECC:
			sprintf(reg_addr_str, "ECC");
			break;
		case SPICO_SERDES_ADDR_ECCLOG:
			sprintf(reg_addr_str, "ECCLOG");
			break;
		case SPICO_SERDES_ADDR_BCAST:
			sprintf(reg_addr_str, "BCAST");
			break;
		case SPICO_SERDES_ADDR_IP_IDCODE:
			sprintf(reg_addr_str, "IP_IDCODE");
			break;
		default:
			sprintf(reg_addr_str, "UNKNOWN");
			break;
		}
	} else {
		sprintf(reg_addr_str, "UNKNOWN");
	}
}
#endif

#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void
sbus_cmd_to_string(u8 command, char *cmd_str)
{
	char str0[SBL_MAX_STR_LEN], str1[SBL_MAX_STR_LEN], str2[SBL_MAX_STR_LEN];

	switch (command & SBUS_CMD_MODE_MASK) {
	case SBUS_CMD_MODE_CTLR:
		sprintf(str0, "CM_CTLR");
		break;
	case SBUS_CMD_MODE_RCVR:
		sprintf(str0, "CM_RCVR");
		break;
	default:
		sprintf(str0, "CM_UNKNOWN");
		break;
	}
	switch (command & SBUS_IFACE_DST_MASK) {
	case SBUS_IFACE_DST_TAP:
		sprintf(str1, "ID_TAP");
		break;
	case SBUS_IFACE_DST_CORE:
		sprintf(str1, "ID_CORE");
		break;
	case SBUS_IFACE_DST_SPICO:
		sprintf(str1, "ID_SPICO");
		break;
	case SBUS_IFACE_DST_SPARE:
		sprintf(str1, "ID_SPARE");
		break;
	default:
		sprintf(str1, "ID_UNKNOWN");
		break;
	}
	switch (command & SBUS_CMD_MASK) {
	case SBUS_CMD_RESET:
		sprintf(str2, "CMD_RESET");
		break;
	case SBUS_CMD_WRITE:
		sprintf(str2, "CMD_WRITE");
		break;
	case SBUS_CMD_READ:
		sprintf(str2, "CMD_READ");
		break;
	case SBUS_CMD_READ_RESULT:
		sprintf(str2, "CMD_READ_RESULT");
		break;
	default:
		sprintf(str2, "CMD_UNKNOWN");
		break;
	}
	sprintf(cmd_str, "%s:%s:%s", str0, str1, str2);
}
#endif

#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void
sbus_result_code_to_string(u8 result_code, char *rc_string)
{
	switch (result_code) {
	case SBUS_RC_RESET:
		sprintf(rc_string, "RESET");
		break;
	case SBUS_RC_WRITE_COMPLETE:
		sprintf(rc_string, "WRITE_COMPLETE");
		break;
	case SBUS_RC_WRITE_FAILED:
		sprintf(rc_string, "WRITE_FAILED");
		break;
	case SBUS_RC_READ_ALL_COMPLETE:
		sprintf(rc_string, "READ_ALL_COMPLETE");
		break;
	case SBUS_RC_READ_COMPLETE:
		sprintf(rc_string, "READ_COMPLETE");
		break;
	case SBUS_RC_READ_FAILED:
		sprintf(rc_string, "READ_FAILED");
		break;
	case SBUS_RC_CMD_ISSUE_DONE:
		sprintf(rc_string, "CMD_ISSUE_DONE");
		break;
	case SBUS_RC_MODE_CHANGE_COMPLETE:
		sprintf(rc_string, "MODE_CHANGE_COMPLETE");
		break;
	default:
		sprintf(rc_string, "UNKNOWN");
		break;
	}
}
#endif

#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void
spico_interrupt_to_string(u32 sbus_addr, u16 interrupt, char *intr_str)
{
	if (is_sbm_spico_addr(sbus_addr)) {
		switch (interrupt) {
		case SPICO_INT_SBMS_REV_ID:
			sprintf(intr_str, "SBMS_REV_ID");
			break;
		case SPICO_INT_SBMS_BUILD_ID:
			sprintf(intr_str, "SBMS_BUILD_ID");
			break;
		case SPICO_INT_SBMS_DO_CRC:
			sprintf(intr_str, "SBMS_DO_CRC");
			break;
		case SPICO_INT_SBMS_READ_DMEM_VAL:
			sprintf(intr_str, "SBMS_READ_DMEM_VAL");
			break;
		case SPICO_INT_SBMS_DO_XDMEM_CRC:
			sprintf(intr_str, "SBMS_DO_XDMEM_CRC");
			break;
		case SPICO_INT_SBMS_GET_PMRO_DATA:
			sprintf(intr_str, "SBMS_GET_PMRO_DATA");
			break;
		case SPICO_INT_SBMS_GET_TEMP_DATA:
			sprintf(intr_str, "SBMS_GET_TEMP_DATA");
			break;
		case SPICO_INT_SBMS_GET_VOLT_DATA:
			sprintf(intr_str, "SBMS_GET_VOLT_DATA");
			break;
		case SPICO_INT_SBMS_IMEM_CRC_CHECK:
			sprintf(intr_str, "SBMS_IMEM_CRC_CHECK");
			break;
		case SPICO_INT_SBMS_IMEM_SWP_SA:
			sprintf(intr_str, "SBMS_IMEM_SWP_SA");
			break;
		case SPICO_INT_SBMS_PCIE3_SWP:
			sprintf(intr_str, "SBMS_PCIE3_SWP");
			break;
		case SPICO_INT_SBMS_DRMON_SETUP:
			sprintf(intr_str, "SBMS_DRMON_SETUP");
			break;
		case SPICO_INT_SBMS_TEMP_SETUP:
			sprintf(intr_str, "SBMS_TEMP_SETUP");
			break;
		case SPICO_INT_SBMS_RR_PCAL:
			sprintf(intr_str, "SBMS_RR_PCAL");
			break;
		default:
			if ((interrupt >= SPICO_INT_SBMS_DDR_HMB_FIRST) &&
			    (interrupt <= SPICO_INT_SBMS_DDR_HMB_LAST)) {
				sprintf(intr_str, "SBMS_DDR_HMB");
			} else {
				sprintf(intr_str, "SBMS_UNKNOWN");
			}
			break;
		}
	} else if (is_cm4_serdes_addr(sbus_addr)) {
		switch (interrupt) {
		case SPICO_INT_CM4_REV_ID:
			sprintf(intr_str, "CM4_REV_ID");
			break;
		case SPICO_INT_CM4_SERDES_EN:
			sprintf(intr_str, "CM4_SERDES_EN");
			break;
		case SPICO_INT_CM4_PRBS_CTRL:
			sprintf(intr_str, "CM4_PRBS_CTRL");
			break;
		case SPICO_INT_CM4_COMPARE_CTRL:
			sprintf(intr_str, "CM4_COMPARE_CTRL");
			break;
		case SPICO_INT_CM4_PMD_CTRL:
			sprintf(intr_str, "CM4_PMD_CTRL");
			break;
		case SPICO_INT_CM4_TX_BAUD:
			sprintf(intr_str, "CM4_TX_BAUD");
			break;
		case SPICO_INT_CM4_RX_BAUD:
			sprintf(intr_str, "CM4_RX_BAUD");
			break;
		case SPICO_INT_CM4_AN_CONFIG:
			sprintf(intr_str, "CM4_AN_CONFIG");
			break;
		case SPICO_INT_CM4_LOOPBACK:
			sprintf(intr_str, "CM4_LOOPBACK");
			break;
		case SPICO_INT_CM4_DFE_CTRL:
			sprintf(intr_str, "CM4_DFE_CTRL");
			break;
		case SPICO_INT_CM4_TX_PHASE_CAL:
			sprintf(intr_str, "CM4_TX_PHASE_CAL");
			break;
		case SPICO_INT_CM4_BIT_SLIP:
			sprintf(intr_str, "CM4_BIT_SLIP");
			break;
		case SPICO_INT_CM4_TX_PHASE_SLIP:
			sprintf(intr_str, "CM4_TX_PHASE_SLIP");
			break;
		case SPICO_INT_CM4_RX_PHASE_SLIP:
			sprintf(intr_str, "CM4_RX_PHASE_SLIP");
			break;
		case SPICO_INT_CM4_PCIE_PD:
			sprintf(intr_str, "CM4_PCIE_PD");
			break;
		case SPICO_INT_CM4_PLL_RECAL:
			sprintf(intr_str, "CM4_PLL_RECAL");
			break;
		case SPICO_INT_CM4_SAS_APTA_CTRL:
			sprintf(intr_str, "CM4_SAS_APTA_CTRL");
			break;
		case SPICO_INT_CM4_POLARITY_CTRL:
			sprintf(intr_str, "CM4_POLARITY_CTRL");
			break;
		case SPICO_INT_CM4_WIDTH_MODE:
			sprintf(intr_str, "CM4_WIDTH_MODE");
			break;
		case SPICO_INT_CM4_TXEQ_LOAD:
			sprintf(intr_str, "CM4_TXEQ_LOAD");
			break;
		case SPICO_INT_CM4_ERROR_RST:
			sprintf(intr_str, "CM4_ERROR_RST");
			break;
		case SPICO_INT_CM4_BIG_REG_SEL:
			sprintf(intr_str, "CM4_BIG_REG_SEL");
			break;
		case SPICO_INT_CM4_BIG_REG_WR:
			sprintf(intr_str, "CM4_BIG_REG_WR");
			break;
		case SPICO_INT_CM4_BIG_REG_RD:
			sprintf(intr_str, "CM4_BIG_REG_RD");
			break;
		case SPICO_INT_CM4_ERR_INJECT:
			sprintf(intr_str, "CM4_ERR_INJECT");
			break;
		case SPICO_INT_CM4_DO_DATA_CAP:
			sprintf(intr_str, "CM4_DO_DATA_CAP");
			break;
		case SPICO_INT_CM4_WAIT_TTD:
			sprintf(intr_str, "CM4_WAIT_TTD");
			break;
		case SPICO_INT_CM4_ERR_STS:
			sprintf(intr_str, "CM4_ERR_STS");
			break;
		case SPICO_INT_CM4_ERR_TIMER_STS:
			sprintf(intr_str, "CM4_ERR_TIMER_STS");
			break;
		case SPICO_INT_CM4_PCIE_SLICES:
			sprintf(intr_str, "CM4_PCIE_SLICES");
			break;
		case SPICO_INT_CM4_PCIE3_RX_EQ:
			sprintf(intr_str, "CM4_PCIE3_RX_EQ");
			break;
		case SPICO_INT_CM4_SET_RX_EQ:
			sprintf(intr_str, "CM4_SET_RX_EQ");
			break;
		case SPICO_INT_CM4_GET_RX_EQ:
			sprintf(intr_str, "CM4_GET_RX_EQ");
			break;
		case SPICO_INT_CM4_CORE_TO_CNTL_EN:
			sprintf(intr_str, "CM4_CORE_TO_CNTL_EN");
			break;
		case SPICO_INT_CM4_AN_DME_PAGE:
			sprintf(intr_str, "CM4_AN_DME_PAGE");
			break;
		case SPICO_INT_CM4_INT_RX_TERM:
			sprintf(intr_str, "CM4_INT_RX_TERM");
			break;
		case SPICO_INT_CM4_HAL_READ:
			sprintf(intr_str, "CM4_HAL_READ");
			break;
		case SPICO_INT_CM4_HAL_WRITE:
			sprintf(intr_str, "CM4_HAL_WRITE");
			break;
		case SPICO_INT_CM4_HAL_CALL:
			sprintf(intr_str, "CM4_HAL_CALL");
			break;
		case SPICO_INT_CM4_SPICO_CLK_SWP:
			sprintf(intr_str, "CM4_SPICO_CLK_SWP");
			break;
		case SPICO_INT_CM4_PCIE3_TX_MRG1:
			sprintf(intr_str, "CM4_PCIE3_TX_MRG1");
			break;
		case SPICO_INT_CM4_PCIE3_TX_MRG2:
			sprintf(intr_str, "CM4_PCIE3_TX_MRG2");
			break;
		case SPICO_INT_CM4_PCIE3_TX_MRG3:
			sprintf(intr_str, "CM4_PCIE3_TX_MRG3");
			break;
		case SPICO_INT_CM4_PCIE3_TX_MRG4:
			sprintf(intr_str, "CM4_PCIE3_TX_MRG4");
			break;
		case SPICO_INT_CM4_PCIE3_TX_MRG5:
			sprintf(intr_str, "CM4_PCIE3_TX_MRG5");
			break;
		case SPICO_INT_CM4_PROC_RESET:
			sprintf(intr_str, "CM4_PROC_RESET");
			break;
		case SPICO_INT_CM4_PCIE_SWP_SETUP:
			sprintf(intr_str, "CM4_PCIE_SWP_SETUP");
			break;
		case SPICO_INT_CM4_IP_STAT_INFO:
			sprintf(intr_str, "CM4_IP_STAT_INFO");
			break;
		case SPICO_INT_CM4_CRC:
			sprintf(intr_str, "CM4_CRC");
			break;
		case SPICO_INT_CM4_PMD_CONFIG:
			sprintf(intr_str, "CM4_PMD_CONFIG");
			break;
		case SPICO_INT_CM4_BUILD_ID:
			sprintf(intr_str, "CM4_BUILD_ID");
			break;
		case SPICO_INT_CM4_O_CORE_STS_15_0:
			sprintf(intr_str, "CM4_O_CORE_STS_15_0");
			break;
		case SPICO_INT_CM4_O_CORE_STS_31_16:
			sprintf(intr_str, "CM4_O_CORE_STS_31_16");
			break;
		default:
			if (interrupt & SPICO_INT_CM4_MEM_READ) {
				sprintf(intr_str, "CM4_MEM_READ_0x%03X",
					interrupt & ~SPICO_INT_CM4_MEM_READ);
			} else if (interrupt & SPICO_INT_CM4_MEM_WRITE) {
				sprintf(intr_str, "CM4_MEM_WRITE_0x%03X",
					interrupt & ~SPICO_INT_CM4_MEM_WRITE);
			} else {
				sprintf(intr_str, "CM4_UNKNOWN");
			}
			break;
		}
	} else {
		sprintf(intr_str, "UNKNOWN");
	}
}
#endif

#if !defined(CONFIG_SBL_PLATFORM_CAS_EMU) && !defined(CONFIG_SBL_PLATFORM_CAS_SIM)
static void sbus_msg(void *inst, u32 sbus_addr, u32 req_data,
		     u8 reg_addr, u8 command, u32 rsp_data,
		     u8 result_code, u8 overrun, int timeout,
		     u32 flags, int rc, int severity)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	char sbus_addr_str[SBL_MAX_STR_LEN], cmd_str[SBL_MAX_STR_LEN];
	char reg_addr_str[SBL_MAX_STR_LEN], rc_str[SBL_MAX_STR_LEN];
	char message[SBL_MAX_STR_LEN];

	if (!sbl)
		return;

	sbus_addr_to_string(sbus_addr, sbus_addr_str);
	sbus_reg_addr_to_string(sbus_addr, reg_addr, reg_addr_str);
	sbus_cmd_to_string(command, cmd_str);
	sbus_result_code_to_string(result_code, rc_str);

	snprintf(message, SBL_MAX_STR_LEN, "SBUS_OP: addr:0x%03x(%-20s) req_data:0x%08x reg_addr:0x%04x(%-15s)",
		 sbus_addr, sbus_addr_str, req_data, reg_addr, reg_addr_str);
	snprintf(message, SBL_MAX_STR_LEN, "SBUS_OP: command:0x%02x(%-24s), rsp_data:0x%08x result_code:0x%01x(%-14s)",
		 command, cmd_str, rsp_data, result_code, rc_str);
	snprintf(message, SBL_MAX_STR_LEN, "SBUS_OP: overrun:%d timeout:0x%04x flags:0x%04x rc:%d",
		 overrun, timeout, flags, rc);
	if (severity >= LEVEL_ERR)
		SBL_ERR(sbl->dev, "%s", message);
	else if (severity >= LEVEL_WARN)
		SBL_WARN(sbl->dev, "%s", message);
	else if (severity >= LEVEL_INFO)
		SBL_INFO(sbl->dev, "%s", message);
	else
		SBL_TRACE1(sbl->dev, "%s", message);

}
#endif

int sbl_sbus_wr(void *inst, u32 sbus_addr, u8 reg_addr,
		u32 sbus_data)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	u32 unused;

	return sbl_sbus_op_aux(sbl, sbus_addr, reg_addr, SBUS_IFACE_DST_CORE |
			       SBUS_CMD_WRITE, sbus_data, &unused);
}
#ifdef __KERNEL__
EXPORT_SYMBOL(sbl_sbus_wr);
#endif


int sbl_sbus_rd(void *inst, u32 sbus_addr, u8 reg_addr,
		u32 *result)
{
	SBL_INST *sbl = (SBL_INST *)inst;

	return sbl_sbus_op_aux(sbl, sbus_addr, reg_addr, SBUS_IFACE_DST_CORE |
			       SBUS_CMD_READ, 0, result);
}
#ifdef __KERNEL__
EXPORT_SYMBOL(sbl_sbus_rd);
#endif

int sbl_serdes_mem_rd(void *inst, u32 port_num, u32 serdes,
		      u32 addr, u16 *data)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int err;

	SBL_TRACE2(sbl->dev, "p%ds%d: addr:0x%x", port_num, serdes, addr);
	addr &= SPICO_INT_MEM_READ_ADDR_MASK;
	err = sbl_serdes_spico_int(sbl, port_num, serdes,
					SPICO_INT_CM4_MEM_READ | addr,
					SPICO_INT_DATA_NONE, data,
					SPICO_INT_RETURN_RESULT);
	if (err)
		return err;

	SBL_TRACE2(sbl->dev, "p%ds%d: addr:0x%x data:0x%x", port_num, serdes, addr, *data);

	return 0;
}

int sbl_serdes_mem_wr(void *inst, u32 port_num, u32 serdes,
		      u32 addr, u16 data)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int err;

	SBL_TRACE2(sbl->dev, "p%ds%d: addr:0x%x", port_num, serdes, addr);
	addr &= SPICO_INT_MEM_READ_ADDR_MASK;
	err = sbl_serdes_spico_int(sbl, port_num, serdes,
					SPICO_INT_CM4_MEM_WRITE | addr,
					data, NULL, SPICO_INT_IGNORE_RESULT);
	if (err)
		return err;

	SBL_TRACE2(sbl->dev, "p%ds%d: addr:0x%x data:0x%x", port_num, serdes, addr,
		   data);

	return 0;
}

int sbl_serdes_mem_rmw(void *inst, u32 port_num, u32 serdes,
		       u32 addr, u16 data, u16 mask)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int err;
	u16 rdata, wdata;

	SBL_TRACE2(sbl->dev, "p%ds%d: addr:0x%x data:0x%x mask:0x%x", port_num,
		   serdes, addr, data, mask);
	err = sbl_serdes_mem_rd(sbl, port_num, serdes, addr, &rdata);
	if (err)
		return err;

	wdata = (rdata & ~mask) | (data & mask);
	err = sbl_serdes_mem_wr(sbl, port_num, serdes, addr, wdata);
	if (err)
		return err;

	SBL_TRACE2(sbl->dev, "p%ds%d: addr:0x%x rmw_data:0x%x", port_num, serdes,
		   addr, wdata);

	return 0;
}

int sbl_spico_burst_upload(void *inst, u32 sbus, u32 reg,
			   size_t fw_size, const u8 *fw_data)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int err;
	u32 byte;

	if (!fw_data || fw_size <= 0) {
		SBL_ERR(sbl->dev,
			"Bad firmware for sbus:0x%02x reg:0x%x fw->size:%zd!",
			sbus, reg, fw_size);
		return -EINVAL;
	}

	SBL_TRACE1(sbl->dev, "sbus:0x%02x reg:0x%x fw->size:%zd", sbus, reg,
		fw_size);
	for (byte = 0; byte < fw_size-4; byte += 6) {
		err = sbl_sbus_wr(sbl, sbus, reg, SPICO_SBR_DATA_BE_012 |
				       (fw_data[byte+0]<<8 | fw_data[byte+1]) <<
				       SPICO_SBR_DATA_W0_OFFSET |
				       (fw_data[byte+2]<<8 | fw_data[byte+3]) <<
				       SPICO_SBR_DATA_W1_OFFSET |
				       (fw_data[byte+4]<<8 | fw_data[byte+5]) <<
				       SPICO_SBR_DATA_W2_OFFSET);
		if (err)
			return err;
	}
	if (fw_size - byte == 4) {
		err = sbl_sbus_wr(sbl, sbus, reg, SPICO_SBR_DATA_BE_01 |
				       (fw_data[byte+0]<<8 | fw_data[byte+1]) <<
				       SPICO_SBR_DATA_W0_OFFSET |
				       (fw_data[byte+2]<<8 | fw_data[byte+3]) <<
				       SPICO_SBR_DATA_W1_OFFSET);
		if (err)
			return err;

	} else if (fw_size - byte == 2) {
		err = sbl_sbus_wr(sbl, sbus, reg, SPICO_SBR_DATA_BE_0 |
				       (fw_data[byte+0]<<8 | fw_data[byte+1]) <<
				       SPICO_SBR_DATA_W0_OFFSET);
		if (err)
			return err;
	}
	return 0;
}

#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined(CONFIG_SBL_PLATFORM_CAS_SIM)
int sbl_sbus_op_aux(void *inst, u32 sbus_addr, u8 reg_addr,
		     u8 command, u32 sbus_data, u32 *result)
{
	*result = 0;
	return 0;
}
#else
int sbl_sbus_op_aux(void *inst, u32 sbus_addr, u8 reg_addr,
		     u8 command, u32 sbus_data, u32 *result)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	u8 result_code = 0, overrun = 0;
	int err = -1;
	int retry_cnt = 0;
	int retry_limit = 5;
	u32 sbus_ring = SBUS_RING(sbus_addr);
	u32 rx_addr = SBUS_RX_ADDR(sbus_addr);
	bool valid_result = false;
	int sbus_op_timeout_ms = sbl_iface_get_sbus_op_timeout_ms(sbl);
	int sbus_op_flags = sbl_iface_get_sbus_op_flags(sbl);
#ifdef __KERNEL__
	void *accessor = sbl;
#else
#if defined(CONFIG_SBL_PLATFORM_CAS_HW)
	void *accessor = sbl;
#else
	void *accessor = sbl->hswh;
#endif
#endif /*  __KERNEL__ */

#ifdef __KERNEL__
	// Safety check
	if (!mutex_is_locked(SBUS_RING_MTX(sbl, sbus_ring))) {
		sbus_msg(sbl, sbus_addr, sbus_data, reg_addr, command,
			 0, result_code, overrun, sbus_op_timeout_ms,
			 sbus_op_flags, err, LEVEL_WARN);
		WARN(1, "%s: Unlocked SBUS ring %d!", __func__, sbus_ring);
	}
#endif /*  __KERNEL__ */

	// Perform SBus operation
	while (retry_cnt++ < retry_limit) {
		err = sbl_iface_sbus_op(accessor, sbus_ring, sbus_data,
					     reg_addr, rx_addr, command, result,
					     &result_code, &overrun,
					     sbus_op_timeout_ms,
					     sbus_op_flags);
		if (err) {
			sbus_msg(sbl, sbus_addr, sbus_data, reg_addr, command,
				 0, result_code, overrun, sbus_op_timeout_ms,
				 sbus_op_flags, err, LEVEL_WARN);
		}
		if (err || overrun) {
			SBL_INFO(sbl->dev, "Resetting SBUS ring %d!", sbus_ring);
			err = sbl_iface_sbus_op_reset(sbl, sbus_ring);
			if (err) {
				SBL_WARN(sbl->dev,
					 "sbl_iface_sbus_op_reset failed! sbus_ring:%d rc:%d",
					 sbus_ring, err);
				return err;
			}
		} else {
			break;
		}
	}

	// Validate results
	if (err || overrun) {
		SBL_WARN(sbl->dev, "roshms_sbus_op failed!");
		return -EIO;
	}
	switch (command & 0x3) {
	case SBUS_CMD_RESET:
		if (result_code == SBUS_RC_RESET)
			valid_result = true;

		break;
	case SBUS_CMD_WRITE:
		if (result_code == SBUS_RC_WRITE_COMPLETE)
			valid_result = true;

		break;
	case SBUS_CMD_READ:
		if (result_code == SBUS_RC_READ_COMPLETE)
			valid_result = true;

		break;
	case SBUS_CMD_READ_RESULT:
		if (result_code == SBUS_RC_READ_ALL_COMPLETE)
			valid_result = true;

		break;
	}

	if (!valid_result) {
		SBL_WARN(sbl->dev, "Unexpected result code (%d) 0x%x!",
				result_code, command);
		sbus_msg(sbl, sbus_addr, sbus_data, reg_addr, command,
				*result, result_code, overrun, sbus_op_timeout_ms,
				sbus_op_flags, err, LEVEL_WARN);
		return -ENOMSG;
	}
	sbus_msg(sbl, sbus_addr, sbus_data, reg_addr, command,
			*result, result_code, overrun, sbus_op_timeout_ms,
			sbus_op_flags, err, LEVEL_DBG);

	return 0;
}
#endif /* defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM) */

#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined(CONFIG_SBL_PLATFORM_CAS_SIM)
int sbl_sbm_spico_int(void *inst, u32 sbus_addr, int code, int data,
		      u32 *result)
{
	*result = 0;
	return 0;
}
#else
int sbl_sbm_spico_int(void *inst, u32 sbus_addr, int code, int data,
		      u32 *result)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int err;
	u32 intr_out;
	u32 intr_in;
	char intr_str[SBL_MAX_STR_LEN];
	int sbus_int_timeout = sbl_iface_get_sbus_int_timeout(sbl);
	int sbus_int_poll_interval = sbl_iface_get_sbus_int_poll_interval(sbl);
#ifdef __KERNEL__
	unsigned long last_jiffy;
	u32 sbus_ring = SBUS_RING(sbus_addr);
#else
	clock_t last_clock;
#endif /*  __KERNEL__ */

#ifdef __KERNEL__
	// Safety check
	if (!mutex_is_locked(SBUS_RING_MTX(sbl, sbus_ring))) {
		SBL_WARN(sbl->dev, "%s: Unlocked SBUS ring %d!", __func__, sbus_ring);
		WARN(1, "sbl_sbus_op_aux: Unlocked SBUS ring %d!", sbus_ring);
	}
#endif /*  __KERNEL__ */

	spico_interrupt_to_string(sbus_addr, code, intr_str);

	// Inject the interrupt via SBus writes
	intr_in = ((data & SBMS_INTERRUPT_DATA_MASK)<<SBMS_INTERRUPT_DATA_OFFSET) |
		((code & SBMS_INTERRUPT_CODE_MASK)<<SBMS_INTERRUPT_CODE_OFFSET);

	err = sbl_sbus_wr(sbl, sbus_addr, SPICO_SBR_ADDR_DMEM_IN, intr_in);
	if (err)
		return err;

	err = sbl_sbus_rd(sbl, sbus_addr, SPICO_SBR_ADDR_INTR, &intr_out);
	if (err)
		return err;

	intr_out |= SBMS_INTERRUPT_STATUS_OK;
	err = sbl_sbus_wr(sbl, sbus_addr, SPICO_SBR_ADDR_INTR, intr_out);
	if (err)
		return err;

	intr_out ^= SBMS_INTERRUPT_STATUS_OK;
	err = sbl_sbus_wr(sbl, sbus_addr, SPICO_SBR_ADDR_INTR, intr_out);
	if (err)
		return err;

	// Poll for interrupt completion
#ifdef __KERNEL__
	last_jiffy = jiffies+msecs_to_jiffies(1000*sbus_int_timeout);
#else
	last_clock = clock() + sbus_int_timeout*CLOCKS_PER_SEC;
#endif /*  __KERNEL__ */
	do {
		err = sbl_sbus_rd(sbl, sbus_addr, SPICO_SBR_ADDR_DMEM_OUT,
				       &intr_out);
		if (err)
			return err;

		// Check in progress bit
		if ((intr_out & SBMS_INTERRUPT_IN_PROGRESSS_MASK) == 0)
			break;

		msleep(sbus_int_poll_interval);
#ifdef __KERNEL__
	} while (time_is_after_jiffies(last_jiffy));
#else
	} while (clock() < last_clock);
#endif /*  __KERNEL__ */
	if (intr_out & SBMS_INTERRUPT_IN_PROGRESSS_MASK) {
		SBL_ERR(sbl->dev,
			"SBM_INT: sbus_addr:0x%03x int:0x%x(%s) data:0x%x timed out (timeout:%ds)!",
			sbus_addr, code, intr_str, data, sbus_int_timeout);
		return -ETIME;
	}

	// Reread to ensure result is valid
	err = sbl_sbus_rd(sbl, sbus_addr, SPICO_SBR_ADDR_DMEM_OUT,
			       &intr_out);
	if (err) {
		SBL_ERR(sbl->dev,
			"SBM_INT: sbus_addr:0x%03x int:0x%x(%s) data:0x%x failed!",
			sbus_addr, code, intr_str, data);
		return err;
	}

	// Extract interrupt result
	if ((intr_out & SBMS_INTERRUPT_STATUS_MASK) == 1) {
		*result = (intr_out & SBMS_INTERRUPT_DATA_MASK) >>
			SBMS_INTERRUPT_DATA_OFFSET;
	} else {
		SBL_ERR(sbl->dev,
			"SBM_INT: sbus_addr:0x%03x int:0x%x(%s) data:0x%x Failed with status 0x%x!",
			sbus_addr, code, intr_str, data, (intr_out & SBMS_INTERRUPT_STATUS_MASK));
		return -EBADE;
	}

	SBL_TRACE1(sbl->dev,
		"SBM_INT: sbus_addr:0x%03x int:0x%x(%s) data:0x%x -> 0x%x",
		sbus_addr, code, intr_str, data, *result);
	return 0;
}
#ifdef __KERNEL__
EXPORT_SYMBOL(sbl_sbm_spico_int);
#endif
#endif /* defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM) */

/**
 * @brief Returns a SBUS master address based on a ring number
 *
 * @param chip in: device num
 * @param ring in: sbus ring num
 * @param sbus in: sbus rx addr
 * @param port out: pointer to port num
 * @param serdes out: pointer to port num
 *
 * @return 0 on success or -1 on failure
 */
static int sbl_chip_ring_sbus_to_port_serdes(void *inst, int chip,
					     int ring, int sbus,
					     int *port_out, int *serdes_out)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int port, serdes;

	for (port = 0; port < sbl->switch_info->num_ports; ++port) {
		for (serdes = 0; serdes < sbl->switch_info->num_serdes; ++serdes) {
			if (chip != sbl->switch_info->ports[port].serdes[serdes].device)
				continue;

			if (ring != sbl->switch_info->ports[port].serdes[serdes].sbus_ring)
				continue;

			if (sbus != sbl->switch_info->ports[port].serdes[serdes].rx_addr)
				continue;

			*port_out = port;
			*serdes_out = serdes;
			return 0;
		}
	}

	SBL_WARN(sbl->dev,
		 "Unable to determine port/serdes for chip %d ring %d sbus %d",
		 chip, ring, sbus);
	return -1;
}

int sbl_serdes_spico_int2(void *inst, u32 sbus_addr,
			  int code, int data, u16 *result, u8 result_action)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int port_num, serdes;

	if (sbl_chip_ring_sbus_to_port_serdes(sbl, 0,
						SBUS_RING(sbus_addr),
						SBUS_RX_ADDR(sbus_addr),
						&port_num, &serdes)) {
		SBL_ERR(sbl->dev,
			"Failed converting %d:%d:%d to port/serdes!",
			0, SBUS_RING(sbus_addr), SBUS_RX_ADDR(sbus_addr));
		return -1;
	}

	return sbl_serdes_spico_int(sbl, port_num, serdes, code, data, result,
				    result_action);
}

#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined(CONFIG_SBL_PLATFORM_CAS_SIM)
int sbl_serdes_spico_int(void *inst, u32 port_num, u32 serdes,
			  int code, int data, u16 *result, u8 result_action)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	u16 *p_result;
	u16 result_out;

	if ((result_action == SPICO_INT_RETURN_RESULT) && (result == NULL)) {
		SBL_ERR(sbl->dev, "result pointer was NULL!");
		return -1;
	}
	if (result_action != SPICO_INT_RETURN_RESULT)
		p_result = &result_out;
	else
		p_result = result;

	*p_result = code;
	return 0;
}
#else
int sbl_serdes_spico_int(void *inst, u32 port_num, u32 serdes,
			  int code, int data, u16 *result, u8 result_action)
{
	SBL_INST *sbl = (SBL_INST *)inst;
	int err;
	char intr_str[SBL_MAX_STR_LEN];
	u16 result_out = 0;
	u16 *p_result;
	u32 sbus_addr;
	int serdes_op_timeout_ms = sbl_iface_get_serdes_op_timeout_ms(sbl);
	int serdes_op_flags = sbl_iface_get_serdes_op_flags(sbl);
#ifdef __KERNEL__
	void *accessor = sbl;
#else
#if defined(CONFIG_SBL_PLATFORM_CAS_HW)
	void *accessor = sbl;
#else
	void *accessor = sbl->hswh;
#endif
#endif /*  __KERNEL__ */

	if ((result_action == SPICO_INT_RETURN_RESULT) && (result == NULL)) {
		SBL_ERR(sbl->dev, "result pointer was NULL!");
		return -1;
	}
	if (result_action != SPICO_INT_RETURN_RESULT)
		p_result = &result_out;
	else
		p_result = result;

	// Make a fake SerDes sbus_addr so spico_interrupt_to_string gives us the
	//  right debug string. This function is only called for CM4 SerDes, so this
	//  will always give the correct string.
#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
	sbus_addr = SBUS_ADDR(0, SBUS_RING0_CM4_SERDES_FIRST);
#else
	sbus_addr = SBUS_ADDR(0, SBUS_RINGX_CM4_SERDES_FIRST);
#endif
	spico_interrupt_to_string(sbus_addr, code, intr_str);

	err = sbl_iface_pml_serdes_op(accessor, port_num, serdes, code,
					   data, p_result, serdes_op_timeout_ms,
					   serdes_op_flags);
	if (err) {
		SBL_ERR(sbl->dev,
				"SERDES_INT: p%ds%d sbl_serdes_op failed! (rc:%d) int:0x%02x(%s) data:0x%04x",
				port_num, serdes, err, code, intr_str, data);
		return err;
	}
	SBL_TRACE1(sbl->dev,
			"SERDES_INT: p%ds%d int:0x%02x(%s) data:0x%04x -> 0x%04x",
			port_num, serdes, code, intr_str, data, *p_result);

	if ((result_action == SPICO_INT_VALIDATE_RESULT) &&
	    ((*p_result & SPICO_INT_RESULT_CODE_MASK) != code)) {
		SBL_ERR(sbl->dev,
			"SERDES_INT: p%ds%d int:0x%02x(%s) data:0x%04x -> 0x%04x Unexpected result! Expected 0x%04x!",
			port_num, serdes, code, intr_str, data, *p_result & SPICO_INT_RESULT_CODE_MASK, code);
		return -EBADE;
	}

	return 0;
}
#ifdef __KERNEL__
EXPORT_SYMBOL(sbl_serdes_spico_int);
#endif
#endif /* defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined(CONFIG_SBL_PLATFORM_CAS_SIM) */
