// SPDX-License-Identifier: GPL-2.0

/*
 * sbl_pml_mac.c
 *
 * Copyright 2019-2021 Hewlett Packard Enterprise Development LP
 *
 * core PML block functions
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <sbl/sbl_pml.h>

#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_internal.h"


static void sbl_pml_mac_get_mode(struct sbl_inst *sbl, int port_num, u64 *ifg_mode, u64 *ifg_adjustment);


/*
 *  configure
 */
int sbl_pml_mac_config(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 ifg_mode       = 0;
	u64 ifg_adjustment = 0;
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: MAC config", port_num);

	sbl_pml_mac_get_mode(sbl, port_num, &ifg_mode, &ifg_adjustment);

	/* tx mac config */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);
#ifdef CONFIG_SBL_MAC_PCS_EMU
	sbl_dev_warn(sbl->dev, "%d: MAC setting mac-pcs credits to 0xa", port_num);
	val64 = SBL_PML_CFG_TX_MAC_PCS_CREDITS_UPDATE(val64, 0xaULL);
#else
	val64 = SBL_PML_CFG_TX_MAC_PCS_CREDITS_UPDATE(val64,
			SBL_PML_CFG_TX_MAC_PCS_CREDITS_DFLT);
#endif
	val64 = SBL_PML_CFG_TX_MAC_IFG_MODE_UPDATE(val64, ifg_mode);
	val64 = SBL_PML_CFG_TX_MAC_IEEE_IFG_ADJUSTMENT_UPDATE(val64, ifg_adjustment);
	/* Short preamble is always 0 for now. See Rosetta ERRATA-2594 */
	val64 = SBL_PML_CFG_TX_MAC_SHORT_PREAMBLE_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_TX_MAC_MAC_OPERATIONAL_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);  /* flush */

	/* rx mac config */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);
	val64 = SBL_PML_CFG_RX_MAC_SHORT_PREAMBLE_UPDATE(val64, 0ULL);
	val64 = SBL_PML_CFG_RX_MAC_FILTER_ILLEGAL_SIZE_UPDATE(val64, 1ULL);
	val64 = SBL_PML_CFG_RX_MAC_MAC_OPERATIONAL_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);  /* flush */

	return 0;
}


/*
 * start
 */
void sbl_pml_mac_start(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: MAC start", port_num);

	/* tx mac start */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);
	val64 = SBL_PML_CFG_TX_MAC_MAC_OPERATIONAL_UPDATE(val64, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);  /* flush */

	/* rx mac start */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);
	val64 = SBL_PML_CFG_RX_MAC_MAC_OPERATIONAL_UPDATE(val64, 1ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);  /* flush */

	/* clear any errors */
	val64 = SBL_PML_ERR_FLG_MAC_TX_DP_ERR_SET(1ULL)|
		SBL_PML_ERR_FLG_MAC_RX_DP_ERR_SET(1ULL);
	sbl_write64(sbl, base|SBL_PML_ERR_CLR_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_ERR_CLR_OFFSET);  /* flush */

	sbl_link_info_set(sbl, port_num, SBL_LINK_INFO_MAC_OP);

}
EXPORT_SYMBOL(sbl_pml_mac_start);


/*
 * stop
 */
void sbl_pml_mac_stop(struct sbl_inst *sbl, int port_num)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: MAC stop", port_num);

	/* rx mac off */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);
	val64 = SBL_PML_CFG_RX_MAC_MAC_OPERATIONAL_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);  /* flush */

	/* tx mac off */
	val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);
	val64 = SBL_PML_CFG_TX_MAC_MAC_OPERATIONAL_UPDATE(val64, 0ULL);
	sbl_write64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);  /* flush */

	sbl_link_info_clear(sbl, port_num, SBL_LINK_INFO_MAC_OP);
}
EXPORT_SYMBOL(sbl_pml_mac_stop);


/*
 * get the current hw status
 */
void sbl_pml_mac_hw_status(struct sbl_inst *sbl, int port_num, bool *tx_op, bool *rx_op,
		u32 *ifg_mode, u32 *ifg_adjustment)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	sbl_dev_dbg(sbl->dev, "%d: MAC status", port_num);

	val64 = sbl_read64(sbl, base|SBL_PML_CFG_TX_MAC_OFFSET);
	if (tx_op)
		*tx_op = SBL_PML_CFG_TX_MAC_MAC_OPERATIONAL_GET(val64);
	if (ifg_mode)
		*ifg_mode = SBL_PML_CFG_TX_MAC_IFG_MODE_GET(val64);
	if (ifg_adjustment)
		*ifg_adjustment = SBL_PML_CFG_TX_MAC_IEEE_IFG_ADJUSTMENT_GET(val64);
	if (rx_op) {
		val64 = sbl_read64(sbl, base|SBL_PML_CFG_RX_MAC_OFFSET);
		*rx_op = SBL_PML_CFG_RX_MAC_MAC_OPERATIONAL_GET(val64);
	}
}


/*
 *  HPC IFG set
 */
void sbl_pml_mac_hpc_set(struct sbl_inst *sbl, int port_num)
{
	u32 base  = SBL_PML_BASE(port_num);
	u64 val64 = 0;

	sbl_dev_dbg(sbl->dev, "%d: MAC HPC set", port_num);

	val64 = sbl_read64(sbl, (base | SBL_PML_CFG_TX_MAC_OFFSET));
	val64 = SBL_PML_CFG_TX_MAC_IFG_MODE_UPDATE(val64, 0ULL); /* HPC */
	val64 = SBL_PML_CFG_TX_MAC_IEEE_IFG_ADJUSTMENT_UPDATE(val64, 3ULL); /* no adjustment */
	sbl_write64(sbl, (base | SBL_PML_CFG_TX_MAC_OFFSET), val64);
	sbl_read64(sbl, (base | SBL_PML_CFG_TX_MAC_OFFSET));  /* flush */
}


/*
 *  get mode
 */
static void sbl_pml_mac_get_mode(struct sbl_inst *sbl, int port_num, u64 *ifg_mode, u64 *ifg_adjustment)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: MAC get mode", port_num);

	/* start with what was requested */
	link->ifg_config = link->blattr.ifg_config;

	/* check BL options */
	if (link->blattr.options & SBL_OPT_ENABLE_IFG_CONFIG) {
		/* set using IFG config */
		switch (link->ifg_config) {
		case SBL_IFG_CONFIG_HPC:
			*ifg_mode       = 0;
			*ifg_adjustment = 3;
			break;
		case SBL_IFG_CONFIG_IEEE_200G:
			*ifg_mode       = 1;
			*ifg_adjustment = 0;
			break;
		case SBL_IFG_CONFIG_IEEE_100G:
			*ifg_mode       = 1;
			*ifg_adjustment = 1;
			break;
		case SBL_IFG_CONFIG_IEEE_50G:
		default:
			*ifg_mode       = 1;
			*ifg_adjustment = 2;
			break;
		}
	} else if (link->blattr.options & SBL_OPT_FABRIC_LINK) {
		/* set fabric link */
		*ifg_mode       = 0;
		*ifg_adjustment = 3;
	} else {
		/* set ether link */
		switch (link->link_mode) {
		case SBL_LINK_MODE_BS_200G:
			*ifg_mode       = 1;
			*ifg_adjustment = 0;
			break;
		case SBL_LINK_MODE_BJ_100G:
			*ifg_mode       = 1;
			*ifg_adjustment = 1;
			break;
		case SBL_LINK_MODE_CD_100G:
			*ifg_mode       = 1;
			*ifg_adjustment = 1;
			break;
		case SBL_LINK_MODE_CD_50G:
		default:
			*ifg_mode       = 1;
			*ifg_adjustment = 2;
			break;
		}
	}
}
