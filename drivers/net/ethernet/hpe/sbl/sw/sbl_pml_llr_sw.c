// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <sbl/sbl_pml.h>

#include <linux/hpe/sbl/sbl.h>
#include <linux/hpe/sbl/sbl_an.h>
#include <linux/hpe/sbl/sbl_kconfig.h>
c
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_internal.h"


int sbl_frame_size(struct sbl_inst *sbl, int port_num)
{
	u64 bytes_per_frame = sbl_get_max_frame_size(sbl, port_num);

	return bytes_per_frame;
}

void sbl_llr_max_data_get(struct sbl_inst *sbl, int port_num,
		u64 *cap_data_max, u64 *cap_seq_max)
{
	struct sbl_link *link = sbl->link + port_num;

	switch (link->blattr.link_partner) {
	case SBL_LINK_PARTNER_SWITCH:
		sbl_dev_dbg(sbl->dev, "%d: LLR fabric link detected", port_num);
		*cap_data_max = sbl_llr_fabric_cap_data_max_get();
		*cap_seq_max  = sbl_llr_fabric_cap_seq_max_get();
		break;
	case SBL_LINK_PARTNER_NIC:
	case SBL_LINK_PARTNER_NIC_C2:
		sbl_dev_dbg(sbl->dev, "%d: LLR edge link detected", port_num);
		*cap_data_max = sbl_llr_edge_cap_data_max_get();
		*cap_seq_max  = sbl_llr_edge_cap_seq_max_get();
		break;
	default:
		sbl_dev_dbg(sbl->dev, "%d: LLR unknown link partner", port_num);
		*cap_data_max = sbl_llr_edge_cap_data_max_get();
		*cap_seq_max  = sbl_llr_edge_cap_seq_max_get();
	}
}
