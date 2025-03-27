// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP. All Rights Reserved. */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>

#include "uapi/sbl_sbm_constants.h"
#include "sbl_sbm_serdes.h"
#include "sbl_serdes_map.h"

bool is_cm4_serdes_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if (((ring == 0 || ring == 1) &&
	     ((rx_addr >= SBUS_RINGX_CM4_SERDES_FIRST) &&
	      (rx_addr <= SBUS_RINGX_CM4_SERDES_LAST))) ||
	    (rx_addr == SBUS_BCAST_CM4_SERDES_SPICO)) {
		return true;
	}
	return false;
}

bool is_pcie_serdes_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if ((ring == 0 || ring == 1) &&
	     ((rx_addr >= SBUS_RINGX_PCIE_SERDES_SPICO_FIRST) &&
	      (rx_addr >= SBUS_RINGX_PCIE_SERDES_SPICO_FIRST) &&
	      ((rx_addr & 0x1) == SBUS_RINGX_PCIE_SERDES_SPICO_LSB))) {
		return true;
	}
	return false;
}

bool is_pcie_serdes_pcs_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if ((ring == 0 || ring == 1) &&
	     ((rx_addr >= SBUS_RINGX_PCIE_SERDES_PCS_FIRST) &&
	      (rx_addr <= SBUS_RINGX_PCIE_SERDES_PCS_LAST) &&
	      ((rx_addr & 0x1) == SBUS_RINGX_PCIE_SERDES_PCS_LSB))) {
		return true;
	}
	return false;
}

bool is_sbm_crm_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if (((ring == 0 || ring == 1) && (rx_addr == SBUS_RINGX_SBM)) ||
	    (rx_addr == SBUS_BCAST_SBM)) {

		return true;
	}
	return false;
}

bool is_sbm_spico_addr(u32 sbus_addr)
{
	u8 ring     = SBUS_RING(sbus_addr);
	u16 rx_addr = SBUS_RX_ADDR(sbus_addr);

	if (((ring == 0 || ring == 1) && (rx_addr == SBUS_RINGX_SBM_SPICO)) ||
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
	} else {
		sprintf(data_str, "UNKNOWN");
	}
}


void sbl_sbus_addr_get(u32 *sbus_addr)
{
	*sbus_addr = SBUS_ADDR(0, SBUS_RINGX_CM4_SERDES_FIRST);
}
