// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP. All Rights Reserved. */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>

#include "uapi/ethernet/sbl_sbm_constants.h"

#include "sbl_sbm_serdes.h"
#include "sbl_serdes_map.h"

bool is_cm4_serdes_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

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
}

bool is_pcie_serdes_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if ((ring == 0) && (rx_addr == SBUS_RING0_PCIE_SERDES_SPICO))
		return true;

	return false;
}

bool is_pcie_serdes_pcs_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if ((ring == 0) && (rx_addr == SBUS_RING0_PCIE_PCS))
		return true;

	return false;
}

bool is_sbm_crm_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if (((ring == 0) && (rx_addr == SBUS_RING0_SBM0)) ||
	    ((ring == 1) && (rx_addr == SBUS_RING1_SBM1)) ||
	    (rx_addr == SBUS_BCAST_SBM)) {
		return true;
	}
	return false;
}

bool is_sbm_spico_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if (((ring == 0) && (rx_addr == SBUS_RING0_SBM0_SPICO)) ||
			((ring == 1) && (rx_addr == SBUS_RING1_SBM1_SPICO)) ||
			(rx_addr == SBUS_BCAST_SBM_SPICO)) {
		return true;
	}
	return false;
}

void
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
	} else {
		sprintf(data_str, "UNKNOWN");
	}
}

void sbl_sbus_addr_get(u32 *sbus_addr)
{
	*sbus_addr = SBUS_ADDR(0, SBUS_RING0_CM4_SERDES_FIRST);
}
