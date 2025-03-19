// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2022 Hewlett Packard Enterprise Development LP */


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>

#include "uapi/sbl_kconfig.h"
#include "sbl.h"
#include "sbl_internal.h"


void sbl_debug_clear_config(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	atomic_set(&link->debug_config, 0);
}
EXPORT_SYMBOL(sbl_debug_clear_config);


void sbl_debug_update_config(struct sbl_inst *sbl, int port_num, u32 clear_flags, u32 set_flags)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 old;
	u32 new;

	do {
		old = atomic_read(&link->debug_config);
		new = (old & ~clear_flags) | set_flags;
	} while (atomic_cmpxchg(&link->debug_config, old, new) != old);
}
EXPORT_SYMBOL(sbl_debug_update_config);


int sbl_debug_get_config(struct sbl_inst *sbl, int port_num, u32 *config)
{
	struct sbl_link *link = sbl->link + port_num;

	if (!config)
		return -EINVAL;

	*config = atomic_read(&link->debug_config);

	return 0;
}
EXPORT_SYMBOL(sbl_debug_get_config);


bool sbl_debug_option(struct sbl_inst *sbl, int port_num, u32 flags)
{
	struct sbl_link *link = sbl->link + port_num;
	bool flag_set;

	flag_set = atomic_read(&link->debug_config) & flags;

	return flag_set;
}


#ifdef CONFIG_SYSFS
int sbl_debug_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 debug_config = atomic_read(&link->debug_config);
	int s = 0;

	if (!debug_config)
		return 0;

	s += snprintf(buf+s, size-s, "base link debug: ");
	if (debug_config & SBL_DEBUG_TRACE_LINK_DOWN)
		s += snprintf(buf+s, size-s, "async-down ");
	if (debug_config & SBL_DEBUG_IGNORE_HISER)
		s += snprintf(buf+s, size-s, "ignore-hiser ");
	if (debug_config & SBL_DEBUG_INHIBIT_CLEANUP)
		s += snprintf(buf+s, size-s, "inhibit-cleanup ");
	if (debug_config & SBL_DEBUG_INHIBIT_SPLL_RESET)
		s += snprintf(buf+s, size-s, "inhibit-spll-reset ");

	if (debug_config & SBL_DEBUG_BAD_PARAM_1)
		s += snprintf(buf+s, size-s, "bad-param-1 ");
	if (debug_config & SBL_DEBUG_BAD_PARAM_2)
		s += snprintf(buf+s, size-s, "bad-param-2 ");
	if (debug_config & SBL_DEBUG_INHIBIT_RELOAD_FW)
		s += snprintf(buf+s, size-s, "inhibit-reload-fw ");
	if (debug_config & SBL_DEBUG_FORCE_RELOAD_FW)
		s += snprintf(buf+s, size-s, "force-reload-fw ");

	if (debug_config & SBL_DEBUG_FORCE_MAX_EFFORT)
		s += snprintf(buf+s, size-s, "max-effort ");
	if (debug_config & SBL_DEBUG_FORCE_MED_EFFORT)
		s += snprintf(buf+s, size-s, "med-effort ");
	if (debug_config & SBL_DEBUG_FORCE_MIN_EFFORT)
		s += snprintf(buf+s, size-s, "min-effort ");
	if (debug_config & SBL_DEBUG_INHIBIT_USE_SAVED_TP)
		s += snprintf(buf+s, size-s, "inhibit-use-saved-tp ");

	if (debug_config & SBL_DEBUG_FORCE_PRECODING_ON)
		s += snprintf(buf+s, size-s, "precoding-on ");
	if (debug_config & SBL_DEBUG_FORCE_PRECODING_OFF)
		s += snprintf(buf+s, size-s, "precoding-off ");
	if (debug_config & SBL_DEBUG_ALLOW_MEDIA_BAD_MODE)
		s += snprintf(buf+s, size-s, "allow-media-bad-mode ");
	if (debug_config & SBL_DEBUG_ALLOW_MEDIA_BAD_LEN)
		s += snprintf(buf+s, size-s, "allow-media-bad-len ");

	if (debug_config & SBL_DEBUG_INHIBIT_PCAL)
		s += snprintf(buf+s, size-s, "inhibit-pcal ");
	if (debug_config & SBL_DEBUG_INHIBIT_RELOAD_SBM_FW)
		s += snprintf(buf+s, size-s, "inhibit-reload-sbm-fw ");
	if (debug_config & SBL_DEBUG_FORCE_RELOAD_SBM_FW)
		s += snprintf(buf+s, size-s, "force-reload-sbm-fw ");
	if (debug_config & SBL_DEBUG_DISABLE_AN_NEXT_PAGES)
		s += snprintf(buf+s, size-s, "disable-an-next-pages ");

	if (debug_config & SBL_DEBUG_KEEP_SERDES_UP)
		s += snprintf(buf+s, size-s, "keep-serdes-up ");
	if (debug_config & SBL_DEBUG_SERDES_MAP_DELAY)
		s += snprintf(buf+s, size-s, "serdes-map-delay ");
	if (debug_config & SBL_DEBUG_FORCE_RELOAD_SERDES_FW)
		s += snprintf(buf+s, size-s, "force-reload-serdes-fw ");
	if (debug_config & SBL_DEBUG_ALLOW_LOOP_TIME_FAIL)
		s += snprintf(buf+s, size-s, "allow-loop-time-fail ");
	if (debug_config & SBL_DEBUG_IGNORE_ALIGN)
		s += snprintf(buf+s, size-s, "ignore-align ");
	if (debug_config & SBL_DEBUG_TRACE_PML_INT)
		s += snprintf(buf+s, size-s, "pml-interrupt ");
	if (debug_config & SBL_DEBUG_REMOTE_FAULT_RECOVERY)
		s += snprintf(buf+s, size-s, "remote-fault-recovery ");

	if (debug_config & SBL_DEBUG_DEV0)
		s += snprintf(buf+s, size-s, "dev0 ");
	if (debug_config & SBL_DEBUG_TEST)
		s += snprintf(buf+s, size-s, "test ");

	s += snprintf(buf+s, size-s, "\n");

	return s;
}
#endif
