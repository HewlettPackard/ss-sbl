// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sbl/sbl_pml.h>
#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_serdes_map.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_an.h"
#include "sbl_serdes.h"
#include "sbl_serdes_fn.h"
#include "sbl_internal.h"
#include "sbl_fec.h"

/**
 * sbl_link_get_fec_thresholds() - Get fec thresholds
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @ucw_bad: Pointer to an int where output is stored(ucw threshold value)
 * @ccw_bad: Pointer to an int where output is stored(ccw threshold value)
 * @fecl_warn: Pointer to an int where output is stored(warn threshold value)
 *
 * Get threshold values based on link mode used.
 * @ucw_bad, @ucw_bad and @fecl_warn holds the values and
 * returns through provided pointers to the caller.
 *
 * Return the failure and warning thresholds for fec metrics
 * monitoring
 *
 * The ccw values used here corresponds to a BER of 1.e-4
 * The ucw values correspond to a little less than 1e-10
 * The single lane fec warning threshold is the ccw rate divided by
 * the number of fec lanes in use for the mode.
 *
 *
 * 200G LINK (BS) (CD)
 *   212500000000 bits/second over a 4-lane link @ 200G
 *   39062500 cw/s (5440 bits/codeword)
 *
 *   (Post)Pre-FEC BER
 *  21250000.000000 (un)corrected cw/s => 1e-04 BER (1 bad bit per codeword)
 *   2125000.000000 (un)corrected cw/s => 1e-05 BER
 *    212500.000000 (un)corrected cw/s => 1e-06 BER
 *     21250.000000 (un)corrected cw/s => 1e-07 BER
 *      2125.000000 (un)corrected cw/s => 1e-08 BER
 *       212.500000 (un)corrected cw/s => 1e-09 BER
 *        21.250000 (un)corrected cw/s => 1e-10 BER
 *         2.125000 (un)corrected cw/s => 1e-11 BER
 *         0.212500 (un)corrected cw/s => 1e-12 BER
 *         0.021250 (un)corrected cw/s => 1e-13 BER
 *         0.002125 (un)corrected cw/s => 1e-13 BER
 *
 * 100G LINK (BJ)
 *   103125000000 bits/second over a 4-lane link @ 100G
 *   19531250 cw/s (5280 bits/codeword)
 *
 *   (Post) Pre-FEC BER
 *   1031250.000000 (un)corrected cw/s => 1e-05 BER (1 bad bit per codeword)
 *    103125.000000 (un)corrected cw/s => 1e-06 BER
 *     10312.500000 (un)corrected cw/s => 1e-07 BER
 *      1031.250000 (un)corrected cw/s => 1e-08 BER
 *       103.125000 (un)corrected cw/s => 1e-09 BER
 *        10.312500 (un)corrected cw/s => 1e-10 BER
 *         1.031250 (un)corrected cw/s => 1e-11 BER
 *         0.103125 (un)corrected cw/s => 1e-12 BER
 *
 * Cassini uses the old
 *    sbl_link_get_fec_thresholds()
 * Rosetta uses the newer
 *    sbl_link_get_ucw_thresh_ieee()
 *    sbl_link_get_ucw_thresh_hpe()
 *    sbl_link_get_ccw_thresh_ieee()
 *    sbl_link_get_ccw_thresh_hpe()
 *
 * Return: 0 on success, negative error code on failure
 */
int sbl_link_get_fec_thresholds(struct sbl_inst *sbl, int port_num,
		int *ucw_bad, int *ccw_bad, int *fecl_warn)
{
	struct sbl_link *link = sbl->link + port_num;
	int ucw;
	int ccw;
	int warn;

	if (link->sstate != SBL_SERDES_STATUS_RUNNING)
		return -ENAVAIL;

	switch (link->link_mode) {
	case SBL_LINK_MODE_BS_200G:
		ucw = 21;
		ccw = 21250000;
		warn = ccw/8;
		break;
	case SBL_LINK_MODE_BJ_100G:
		ucw = 11;
		ccw = 10312500;
		warn = ccw/8;
		break;
	case SBL_LINK_MODE_CD_100G:
		ucw = 11;
		ccw = 10625000;
		warn = ccw/4;
		break;
	case SBL_LINK_MODE_CD_50G:
		ucw = 5;
		ccw = 5312500;
		warn = ccw/2;
		break;
	default:
		sbl_dev_err(sbl->dev, "%d: bad mode for fec thresh", port_num);
		return -ENAVAIL;
	}

	if (ucw_bad)
		*ucw_bad = ucw;
	if (ccw_bad)
		*ccw_bad = ccw;
	if (fecl_warn)
		*fecl_warn = warn;

	return 0;
}
EXPORT_SYMBOL(sbl_link_get_fec_thresholds);

#ifdef CONFIG_SYSFS
/**
 * sbl_base_link_llr_sysfs_sprint() - Report LLR on/off status into buffer
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @buf: Destination buffer to write the data
 * @size: Size of data to write
 *
 * This function checks the state info flags of LLR.
 * Based on this flag, it formats the string indicating
 * whether LLR is on/off and writes it into provided buffer
 *
 * Context: Process context, Acquires lock and release link->lock <spin_lock>
 *
 * Return: Number of characters written on success
 */
int sbl_base_link_llr_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);

	if (link->link_info & SBL_LINK_INFO_LLR_RUN)
		s += snprintf(buf+s, size-s, "on");
	else
		s += snprintf(buf+s, size-s, "off");

	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_base_link_llr_sysfs_sprint);

/**
 * sbl_base_link_loopback_sysfs_sprint() - Report loopback mode status into buffer
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @buf: Destination buffer to write the data
 * @size: Size of data to write
 *
 * This function gets the loopback mode status and
 * formats the string into the provided buffer
 *
 * Context: Process context, Acquires lock and release link->lock <spin_lock>
 *
 * Return: Number of characters written on success
 */
int sbl_base_link_loopback_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);

	s += snprintf(buf+s, size-s, sbl_loopback_mode_str(link->loopback_mode));

	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_base_link_loopback_sysfs_sprint);
#endif
