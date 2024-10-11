/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2020,2022-2023 Hewlett Packard Enterprise Development LP. All rights reserved. */

#ifndef SBL_CONFIG_LIST_H
#define SBL_CONFIG_LIST_H

#include "uapi/sbl_serdes.h"
#include "uapi/sbl_serdes_defaults.h"


/*
 * A SerDes configuration list entry specifies a set of config values which
 * are selected to get the SerDes to tune reliably for a given setup. Some default
 * entries are statically defined - additional entries can be added and removed from the
 * list dynamically
 *
 * To add a static serdes configuration
 * 1. create an initializer as below
 * 2. create an static structure in sbl_init.c
 * 3. Add the structure into sbl_setup_serdes_configs()
 *
 */
struct sbl_serdes_config {
	u64 tp_state_mask0;         /* State lookup mask */
	u64 tp_state_mask1;         /* State lookup mask */
	u64 tp_state_match0;        /* match values after masking */
	u64 tp_state_match1;        /* match values after masking */
	u64 port_mask;              /* applicable ports */
	u8  serdes_mask;            /* applicable serdes */
	struct sbl_sc_values vals;  /* Configuration values */

	struct list_head list;
	bool is_default;            /* Default configuration */
	u32 tag;                    /* tag for debugging */
};


/*
 * Default serdes config initializer
 */
#define SBL_SERDES_CONFIG_INITIALIZER					\
{									\
	/*                   --mlmlmtlmtplblp           */              \
	.tp_state_mask0	 = 0x0000000000000000,				\
	/*                   mvmvmvmvmvmvmvmv           */              \
	.tp_state_mask1	 = 0x0000000000000000,				\
	.tp_state_match0 = 0x0000000000000000,				\
	.tp_state_match1 = 0x0000000000000000,				\
	.port_mask	 = 0xffffffffffffffff,				\
	.serdes_mask	 = 0xf,						\
	.vals = {							\
		.magic		= SBL_SERDES_CONFIG_MAGIC,		\
		.atten		= SBL_DFLT_PORT_CONFIG_ATTEN,		\
		.pre		= SBL_DFLT_PORT_CONFIG_PRE,		\
		.post		= SBL_DFLT_PORT_CONFIG_POST,		\
		.pre2		= SBL_DFLT_PORT_CONFIG_PRE2,		\
		.pre3		= SBL_DFLT_PORT_CONFIG_PRE3,		\
		.gs1		= SBL_DFLT_PORT_CONFIG_GS1_OPTICAL,	\
		.gs2		= SBL_DFLT_PORT_CONFIG_GS2_OPTICAL,	\
		.num_intr	= SBL_DFLT_PORT_CONFIG_NUM_INTR,	\
		.intr_val	= SBL_DFLT_PORT_CONFIG_INTR_VAL,	\
		.data_val	= SBL_DFLT_PORT_CONFIG_DATA_VAL,	\
	},								\
}

#endif // SBL_CONFIG_LIST_H
