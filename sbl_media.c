// SPDX-License-Identifier: GPL-2.0

/*
 * sbl_media.c
 *
 * Copyright 2019, 2021-2024 Hewlett Packard Enterprise Development LP
 *
 *
 *
 */

//#define DEBUG     1

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sbl/sbl_pml.h>


#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_internal.h"


static bool sbl_media_check_len_supported(struct sbl_inst *sbl, int port_num, u64 len);


int sbl_media_config(struct sbl_inst *sbl, int port_num,
		struct sbl_media_attr *mattr)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "media config\n");

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	if (!mattr)
		return -EINVAL;

	if (mattr->magic != SBL_MEDIA_ATTR_MAGIC)
		return -EINVAL;

	link = sbl->link + port_num;

	spin_lock(&link->lock);

	if (!(link->blstate & (SBL_BASE_LINK_STATUS_UNCONFIGURED|SBL_BASE_LINK_STATUS_DOWN))) {

		sbl_dev_err(sbl->dev, "%d: wrong state (%s) to configure media\n", port_num,
				sbl_link_state_str(link->blstate));
		spin_unlock(&link->lock);
		return -EUCLEAN;
	}

	memcpy(&link->mattr, mattr, sizeof(struct sbl_media_attr));
	link->mconfigured = true;

	if (link->blconfigured)
		link->blstate = SBL_BASE_LINK_STATUS_DOWN;

	/* the media might have changed so invalidate the llr loop time */
	link->llr_loop_time = 0;

	spin_unlock(&link->lock);

	return 0;
}
EXPORT_SYMBOL(sbl_media_config);


int sbl_media_unconfig(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err = 0;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "media unconfig\n");

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	spin_lock(&link->lock);

	if (!(link->blstate & (SBL_BASE_LINK_STATUS_UNCONFIGURED |
			SBL_BASE_LINK_STATUS_DOWN |
			SBL_BASE_LINK_STATUS_STOPPING))) {

		sbl_dev_err(sbl->dev, "%d: wrong state (%s) to unconfigure media\n", port_num,
				sbl_link_state_str(link->blstate));
		spin_unlock(&link->lock);
		return -EUCLEAN;
	}

	memset(&link->mattr, 0, sizeof(struct sbl_media_attr));
	link->mconfigured = false;

	link->blstate = SBL_BASE_LINK_STATUS_UNCONFIGURED;

	spin_unlock(&link->lock);

	return err;
}
EXPORT_SYMBOL(sbl_media_unconfig);


bool sbl_media_check_mode_supported(struct sbl_inst *sbl, int port_num, u32 link_mode)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 info_mode_bit = 0;

	if (!link->mattr.info) {
		/*
		 * We have no info about media
		 * assume its OK so we can carry on
		 * if its not we will fail later anyway
		 */
		sbl_dev_warn(sbl->dev, "%d: no media info - assuming link mode OK",
			port_num);
		return true;
	}

	switch (link_mode) {
	case SBL_LINK_MODE_BS_200G:
		info_mode_bit = SBL_MEDIA_INFO_SUPPORTS_BS_200G;
		break;
	case SBL_LINK_MODE_BJ_100G:
		info_mode_bit = SBL_MEDIA_INFO_SUPPORTS_BJ_100G;
		break;
	case SBL_LINK_MODE_CD_100G:
		info_mode_bit = SBL_MEDIA_INFO_SUPPORTS_CD_100G;
		break;
	case SBL_LINK_MODE_CD_50G:
		info_mode_bit = SBL_MEDIA_INFO_SUPPORTS_CD_50G;
		break;
	default:
		/* do nothing here - below will catch invalid link mode */
		break;
	}

	if (link->mattr.info & info_mode_bit)
		return true;

	if ((link->blattr.options & SBL_OPT_ALLOW_MEDIA_BAD_MODE) ||
		sbl_debug_option(sbl, port_num, SBL_DEBUG_ALLOW_MEDIA_BAD_MODE)) {
		sbl_dev_warn(sbl->dev, "%d: link mode (%s) not supported by media - ignored",
			port_num, sbl_link_mode_str(link_mode));
		return true;
	} else {
		sbl_dev_err(sbl->dev, "%d: link mode (%s) not supported by media",
			port_num, sbl_link_mode_str(link_mode));
		return false;
	}
}

bool sbl_media_check_headshell_reset_supported(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	if (!link->mattr.info) {
		/* We have no info about media */
		sbl_dev_dbg(sbl->dev, "%d: no media info available to check headshell reset supported bit", port_num);
		return false;
	}

	if (link->mattr.info & SBL_MEDIA_INFO_SUPPORTS_HEADSHELL_RESET)
		return true;

	return false;
}
EXPORT_SYMBOL(sbl_media_check_headshell_reset_supported);

static bool sbl_media_check_len_supported(struct sbl_inst *sbl, int port_num, u64 len)
{
	struct sbl_link *link = sbl->link + port_num;

	switch (len) {
	case SBL_LINK_LEN_BACKPLANE:
	case SBL_LINK_LEN_000_300:
	case SBL_LINK_LEN_000_400:
	case SBL_LINK_LEN_000_750:
	case SBL_LINK_LEN_000_800:
	case SBL_LINK_LEN_001_000:
	case SBL_LINK_LEN_001_100:
	case SBL_LINK_LEN_001_150:
	case SBL_LINK_LEN_001_200:
	case SBL_LINK_LEN_001_400:
	case SBL_LINK_LEN_001_420:
	case SBL_LINK_LEN_001_500:
	case SBL_LINK_LEN_001_600:
	case SBL_LINK_LEN_001_640:
	case SBL_LINK_LEN_001_700:
	case SBL_LINK_LEN_001_800:
	case SBL_LINK_LEN_001_900:
	case SBL_LINK_LEN_001_910:
	case SBL_LINK_LEN_002_000:
	case SBL_LINK_LEN_002_100:
	case SBL_LINK_LEN_002_130:
	case SBL_LINK_LEN_002_200:
	case SBL_LINK_LEN_002_300:
	case SBL_LINK_LEN_002_390:
	case SBL_LINK_LEN_002_400:
	case SBL_LINK_LEN_002_500:
	case SBL_LINK_LEN_002_600:
	case SBL_LINK_LEN_002_620:
	case SBL_LINK_LEN_002_700:
	case SBL_LINK_LEN_002_800:
	case SBL_LINK_LEN_002_900:
	case SBL_LINK_LEN_002_990:
	case SBL_LINK_LEN_003_000:
	case SBL_LINK_LEN_004_000:
	case SBL_LINK_LEN_005_000:
	case SBL_LINK_LEN_006_000:
	case SBL_LINK_LEN_007_000:
	case SBL_LINK_LEN_008_000:
	case SBL_LINK_LEN_010_000:
	case SBL_LINK_LEN_014_000:
	case SBL_LINK_LEN_015_000:
	case SBL_LINK_LEN_019_000:
	case SBL_LINK_LEN_025_000:
	case SBL_LINK_LEN_030_000:
	case SBL_LINK_LEN_035_000:
	case SBL_LINK_LEN_050_000:
	case SBL_LINK_LEN_075_000:
	case SBL_LINK_LEN_100_000:
		return true;
	default:
		if ((link->blattr.options & SBL_OPT_ALLOW_MEDIA_BAD_LEN) ||
				sbl_debug_option(sbl, port_num, SBL_DEBUG_ALLOW_MEDIA_BAD_LEN)) {
			sbl_dev_warn(sbl->dev, "%d: unsupported media len (%lld) - ignored",
					port_num, len);
			return true;
		} else {
			sbl_dev_err(sbl->dev, "%d: unsupported media len (%lld)",
					port_num, len);
			return false;
		}
	}
}


/*
 * return valid length in cm or 0
 */
u32 sbl_media_get_len_cm(struct sbl_inst *sbl, int port_num, u64 len)
{
	if (sbl_media_check_len_supported(sbl, port_num, len)) {
		switch (len) {
		case SBL_LINK_LEN_BACKPLANE: return 25;
		case SBL_LINK_LEN_000_300:   return 30;
		case SBL_LINK_LEN_000_400:   return 40;
		case SBL_LINK_LEN_000_750:   return 75;
		case SBL_LINK_LEN_000_800:   return 80;
		case SBL_LINK_LEN_001_000:   return 100;
		case SBL_LINK_LEN_001_100:   return 110;
		case SBL_LINK_LEN_001_150:   return 115;
		case SBL_LINK_LEN_001_200:   return 120;
		case SBL_LINK_LEN_001_400:   return 140;
		case SBL_LINK_LEN_001_420:   return 142;
		case SBL_LINK_LEN_001_500:   return 150;
		case SBL_LINK_LEN_001_600:   return 160;
		case SBL_LINK_LEN_001_640:   return 164;
		case SBL_LINK_LEN_001_700:   return 170;
		case SBL_LINK_LEN_001_800:   return 180;
		case SBL_LINK_LEN_001_900:   return 190;
		case SBL_LINK_LEN_001_910:   return 191;
		case SBL_LINK_LEN_002_000:   return 200;
		case SBL_LINK_LEN_002_100:   return 210;
		case SBL_LINK_LEN_002_130:   return 213;
		case SBL_LINK_LEN_002_200:   return 220;
		case SBL_LINK_LEN_002_300:   return 230;
		case SBL_LINK_LEN_002_390:   return 239;
		case SBL_LINK_LEN_002_400:   return 240;
		case SBL_LINK_LEN_002_500:   return 250;
		case SBL_LINK_LEN_002_600:   return 260;
		case SBL_LINK_LEN_002_620:   return 262;
		case SBL_LINK_LEN_002_700:   return 270;
		case SBL_LINK_LEN_002_800:   return 280;
		case SBL_LINK_LEN_002_900:   return 290;
		case SBL_LINK_LEN_002_990:   return 299;
		case SBL_LINK_LEN_003_000:   return 300;
		case SBL_LINK_LEN_004_000:   return 400;
		case SBL_LINK_LEN_005_000:   return 500;
		case SBL_LINK_LEN_006_000:   return 600;
		case SBL_LINK_LEN_007_000:   return 700;
		case SBL_LINK_LEN_008_000:   return 800;
		case SBL_LINK_LEN_010_000:   return 1000;
		case SBL_LINK_LEN_014_000:   return 1400;
		case SBL_LINK_LEN_015_000:   return 1500;
		case SBL_LINK_LEN_019_000:   return 1900;
		case SBL_LINK_LEN_025_000:   return 2500;
		case SBL_LINK_LEN_030_000:   return 3000;
		case SBL_LINK_LEN_035_000:   return 3500;
		case SBL_LINK_LEN_050_000:   return 5000;
		case SBL_LINK_LEN_075_000:   return 7500;
		case SBL_LINK_LEN_100_000:   return 10000;
		}
	}

	return 0;
}


/*
 * calc loop time based on cable length
 */
int sbl_media_calc_loop_time(struct sbl_inst *sbl, int port_num, u64 *calc_loop_time)
{
	struct sbl_link *link;
	u32 len;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	if (!calc_loop_time)
		return -EINVAL;

	link = sbl->link + port_num;
	*calc_loop_time = 0;

	if ((link->blattr.loopback_mode == SBL_LOOPBACK_MODE_OFF) ||
		(link->blattr.loopback_mode == SBL_LOOPBACK_MODE_INVALID)) {
		len = sbl_media_get_len_cm(sbl, port_num, link->mattr.len);
		if (!len)
			return -EBADRQC;
	} else
		len = 0;

	sbl_dev_dbg(sbl->dev, "%d: len = %d, lb = %s",
		port_num, len, sbl_loopback_mode_str(link->blattr.loopback_mode));

	if (link->mattr.media == SBL_LINK_MEDIA_ELECTRICAL)
		*calc_loop_time = 2 * len * SBL_MEDIA_NS_PER_M / 100;
	else
		*calc_loop_time = (2 * len * SBL_MEDIA_OPTICAL_NS_PER_M / 100) +
			(4 * SBL_MEDIA_OPTICAL_TRANCEIVER_DELAY);
	*calc_loop_time += 2 * (SBL_ASIC_TX_DELAY + SBL_ASIC_RX_DELAY);

	sbl_dev_dbg(sbl->dev, "%d: calc loop time %lld cm %lld ns",
		port_num, link->mattr.len, *calc_loop_time);

	return 0;
}


/*
 * return loop time in ns or zero on failure
 *
 *   return measured time is available otherwise calculate
 *   an approximate value using the cable length.
 *   Return zero if no valid loop time can be returned
 */
u64 sbl_media_get_loop_time_ns(struct sbl_inst *sbl, int port_num,
		bool measurement_only)
{
	struct sbl_link *link = sbl->link + port_num;
	u64 calc_loop_time;
	int err;

	if (!link->mconfigured)
		return 0;

	if (link->llr_loop_time)
		return link->llr_loop_time;

	if (measurement_only) {
		sbl_dev_warn(sbl->dev, "%d: measured loop time not found",
				port_num);
		return 0;
	}

	if (!sbl_media_check_len_supported(sbl, port_num, link->mattr.len))
		return 0;

	err = sbl_media_calc_loop_time(sbl, port_num, &calc_loop_time);
	if (err)
		return 0;

	return calc_loop_time;
}
EXPORT_SYMBOL(sbl_media_get_loop_time_ns);


int sbl_media_validate_config(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	if (!(link->mconfigured && link->blconfigured))
		return -EUCLEAN;

	if ((link->mattr.media == SBL_LINK_MEDIA_ELECTRICAL) &&
			(link->blattr.config_target == SBL_BASE_LINK_CONFIG_PEC))
		return 0;

	if ((link->mattr.media == SBL_LINK_MEDIA_OPTICAL) &&
			(link->blattr.config_target == SBL_BASE_LINK_CONFIG_AOC))
		return 0;

	return -EMEDIUMTYPE;
}


#ifdef CONFIG_SYSFS
int sbl_media_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);
	if (link->mconfigured) {
		s += snprintf(buf+s, size-s, "media: %s, %s, (%s), info ",
			      sbl_link_media_str(link->mattr.media),
			      sbl_link_len_str(link->mattr.len),
			      sbl_link_vendor_str(link->mattr.vendor));
		if (link->mattr.info) {
			if (link->mattr.info & SBL_MEDIA_INFO_SUPPORTS_BS_200G)
				s += snprintf(buf+s, size-s, "200 ");
			if (link->mattr.info & SBL_MEDIA_INFO_SUPPORTS_BJ_100G)
				s += snprintf(buf+s, size-s, "100bj ");
			if (link->mattr.info & SBL_MEDIA_INFO_SUPPORTS_CD_100G)
				s += snprintf(buf+s, size-s, "100cd ");
			if (link->mattr.info & SBL_MEDIA_INFO_SUPPORTS_CD_50G)
				s += snprintf(buf+s, size-s, "50 ");
			if (link->mattr.info & SBL_MEDIA_INFO_ANALOG)
				s += snprintf(buf+s, size-s, "analog ");
			if (link->mattr.info & SBL_MEDIA_INFO_DIGITAL)
				s += snprintf(buf+s, size-s, "digital ");
		} else
			s += snprintf(buf+s, size-s, "none");
		s += snprintf(buf+s, size-s, "\n");
	}
	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_media_sysfs_sprint);

#ifndef CONFIG_SBL_PLATFORM_ROS_HW
int sbl_media_type_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;

	spin_lock(&link->lock);

	if (link->mconfigured)
		s += snprintf(buf+s, size-s, "%s", sbl_link_media_str(link->mattr.media));
	else
		s += snprintf(buf+s, size-s, "NA");

	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_media_type_sysfs_sprint);
#endif
#endif
