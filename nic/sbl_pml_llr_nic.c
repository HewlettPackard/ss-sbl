// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

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
#include "sbl_an.h"

int sbl_frame_size(struct sbl_inst *sbl, int port_num)
{
	u64 bytes_per_frame = 9216;

	return bytes_per_frame;
}

void sbl_llr_max_data_get(struct sbl_inst *sbl, int port_num,
			 u64 *cap_data_max, u64 *cap_seq_max)
{
	*cap_data_max = sbl_llr_edge_cap_data_max_get();
	*cap_seq_max  = sbl_llr_edge_cap_seq_max_get();
}
