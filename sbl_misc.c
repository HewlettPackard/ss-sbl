// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2024 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <sbl/sbl_pml.h>


#include "sbl_kconfig.h"
#include "uapi/sbl.h"
#include "sbl.h"
#include "sbl_constants.h"
#include "sbl_internal.h"


/*
 * enum to string functions
 */
const char *sbl_link_state_str(enum sbl_base_link_status state)
{
	switch (state) {
	case SBL_BASE_LINK_STATUS_UNKNOWN:      return "unknown";
	case SBL_BASE_LINK_STATUS_RESETTING:    return "resetting";
	case SBL_BASE_LINK_STATUS_UNCONFIGURED: return "unconfigured";
	case SBL_BASE_LINK_STATUS_STARTING:     return "starting";
	case SBL_BASE_LINK_STATUS_UP:           return "up";
	case SBL_BASE_LINK_STATUS_STOPPING:     return "stopping";
	case SBL_BASE_LINK_STATUS_DOWN:         return "down";
	case SBL_BASE_LINK_STATUS_ERROR:        return "error";
	default:                                return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_link_state_str);


const char *sbl_link_len_str(enum sbl_link_len len)
{
	switch (len) {
	case SBL_LINK_LEN_INVALID:   return "invalid";
	case SBL_LINK_LEN_BACKPLANE: return "backplane";
	case SBL_LINK_LEN_000_300:   return "0.30m";
	case SBL_LINK_LEN_000_400:   return "0.40m";
	case SBL_LINK_LEN_000_750:   return "0.75m";
	case SBL_LINK_LEN_000_800:   return "0.80m";
	case SBL_LINK_LEN_001_000:   return "1.00m";
	case SBL_LINK_LEN_001_100:   return "1.10m";
	case SBL_LINK_LEN_001_150:   return "1.15m";
	case SBL_LINK_LEN_001_200:   return "1.20m";
	case SBL_LINK_LEN_001_400:   return "1.40m";
	case SBL_LINK_LEN_001_420:   return "1.42m";
	case SBL_LINK_LEN_001_500:   return "1.50m";
	case SBL_LINK_LEN_001_600:   return "1.60m";
	case SBL_LINK_LEN_001_640:   return "1.64m";
	case SBL_LINK_LEN_001_700:   return "1.70m";
	case SBL_LINK_LEN_001_800:   return "1.80m";
	case SBL_LINK_LEN_001_900:   return "1.90m";
	case SBL_LINK_LEN_001_910:   return "1.91m";
	case SBL_LINK_LEN_002_000:   return "2.00m";
	case SBL_LINK_LEN_002_100:   return "2.10m";
	case SBL_LINK_LEN_002_130:   return "2.13m";
	case SBL_LINK_LEN_002_200:   return "2.20m";
	case SBL_LINK_LEN_002_300:   return "2.30m";
	case SBL_LINK_LEN_002_390:   return "2.39m";
	case SBL_LINK_LEN_002_400:   return "2.40m";
	case SBL_LINK_LEN_002_500:   return "2.50m";
	case SBL_LINK_LEN_002_600:   return "2.60m";
	case SBL_LINK_LEN_002_620:   return "2.62m";
	case SBL_LINK_LEN_002_700:   return "2.70m";
	case SBL_LINK_LEN_002_800:   return "2.80m";
	case SBL_LINK_LEN_002_900:   return "2.90m";
	case SBL_LINK_LEN_002_990:   return "2.99m";
	case SBL_LINK_LEN_003_000:   return "3.00m";
	case SBL_LINK_LEN_004_000:   return "4.00m";
	case SBL_LINK_LEN_005_000:   return "5.00m";
	case SBL_LINK_LEN_006_000:   return "6.00m";
	case SBL_LINK_LEN_007_000:   return "7.00m";
	case SBL_LINK_LEN_008_000:   return "8.00m";
	case SBL_LINK_LEN_010_000:   return "10.00m";
	case SBL_LINK_LEN_014_000:   return "14.00m";
	case SBL_LINK_LEN_015_000:   return "15.00m";
	case SBL_LINK_LEN_019_000:   return "19.00m";
	case SBL_LINK_LEN_025_000:   return "25.00m";
	case SBL_LINK_LEN_030_000:   return "30.00m";
	case SBL_LINK_LEN_035_000:   return "35.00m";
	case SBL_LINK_LEN_050_000:   return "50.00m";
	case SBL_LINK_LEN_075_000:   return "75.00m";
	case SBL_LINK_LEN_100_000:   return "100.00m";
	default:                     return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_link_len_str);


const char *sbl_link_vendor_str(enum sbl_link_vendor vendor)
{
	switch (vendor) {
	case SBL_LINK_VENDOR_INVALID:        return "invalid";
	case SBL_LINK_VENDOR_TE:             return "TE Connectivity";
	case SBL_LINK_VENDOR_LEONI:          return "Leoni";
	case SBL_LINK_VENDOR_MOLEX:          return "Molex";
	case SBL_LINK_VENDOR_HISENSE:        return "Hisense";
	case SBL_LINK_VENDOR_DUST_PHOTONICS: return "Dust Photonics";
	case SBL_LINK_VENDOR_FINISAR:        return "Finisar";
	case SBL_LINK_VENDOR_LUXSHARE:       return "Luxshare";
	case SBL_LINK_VENDOR_FIT:            return "FIT";
	case SBL_LINK_VENDOR_FT:             return "FT";
	case SBL_LINK_VENDOR_MELLANOX:       return "Mellanox";
	case SBL_LINK_VENDOR_HITACHI:        return "Hitachi";
	case SBL_LINK_VENDOR_HPE:            return "HPE";
	case SBL_LINK_VENDOR_CLOUD_LIGHT:    return "Cloud Light";
	default:                             return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_link_vendor_str);


const char *sbl_link_media_str(enum sbl_link_media media)
{
	switch (media) {
	case SBL_LINK_MEDIA_INVALID:         return "invalid";
	case SBL_LINK_MEDIA_UNKNOWN:         return "unknown";
	case SBL_LINK_MEDIA_ELECTRICAL:      return "electrical";
	case SBL_LINK_MEDIA_OPTICAL:         return "optical";
	default:                             return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_link_media_str);


const char *sbl_an_mode_str(enum sbl_an_mode mode)
{
	switch (mode) {
	case SBL_AN_MODE_INVALID: return "invalid";
	case SBL_AN_MODE_UNKNOWN: return "unknown";
	case SBL_AN_MODE_OFF:     return "off";
	case SBL_AN_MODE_ON:      return "on";
	case SBL_AN_MODE_FIXED:   return "fixed";
	default:                  return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_an_mode_str);


const char *sbl_link_mode_str(enum sbl_link_mode mode)
{
	switch (mode) {
	case SBL_LINK_MODE_INVALID: return "invalid";
	case SBL_LINK_MODE_BS_200G: return "BS_200G";
	case SBL_LINK_MODE_BJ_100G: return "BJ_100G";
	case SBL_LINK_MODE_CD_100G: return "CD_100G";
	case SBL_LINK_MODE_CD_50G:  return "CD_50G";
	default:                    return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_link_mode_str);


const char *sbl_rs_mode_str(enum sbl_rs_mode mode)
{
	switch (mode) {
	case SBL_RS_MODE_INVALID:        return "invalid";
	case SBL_RS_MODE_UNKNOWN:        return "unknown";
	case SBL_RS_MODE_OFF:            return "error correction OFF error checking OFF syndrome checking OFF marking OFF";
	case SBL_RS_MODE_OFF_SYN:        return "error correction OFF error checking OFF syndrome checking  ON marking OFF";
	case SBL_RS_MODE_OFF_CHK:        return "error correction OFF error checking  ON syndrome checking OFF marking OFF";
	case SBL_RS_MODE_ON:             return "error correction  ON error checking  ON syndrome checking OFF marking OFF";
	case SBL_RS_MODE_ON_SYN_MRK:     return "error correction  ON error checking OFF syndrome checking  ON marking  ON";
	case SBL_RS_MODE_ON_CHK_SYN_MRK: return "error correction  ON error checking  ON syndrome checking  ON marking  ON";
	default:                         return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_rs_mode_str);


const char *sbl_ifg_mode_str(enum sbl_ifg_mode mode)
{
	switch (mode) {
	case SBL_IFG_MODE_INVALID:  return "invalid";
	case SBL_IFG_MODE_UNKNOWN:  return "unknown";
	case SBL_IFG_MODE_HPC:      return "hpc";
	case SBL_IFG_MODE_IEEE:     return "ieee";
	default:                    return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_ifg_mode_str);


const char *sbl_ifg_config_str(enum sbl_ifg_config config)
{
	switch (config) {
	case SBL_IFG_CONFIG_INVALID:   return "invalid";
	case SBL_IFG_CONFIG_UNKNOWN:   return "unknown";
	case SBL_IFG_CONFIG_HPC:       return "hpc";
	case SBL_IFG_CONFIG_IEEE_200G: return "ieee 200G";
	case SBL_IFG_CONFIG_IEEE_100G: return "ieee 100G";
	case SBL_IFG_CONFIG_IEEE_50G:  return "ieee 50G";
	default:                       return "unrecognized";
}
}
EXPORT_SYMBOL(sbl_ifg_config_str);


const char *sbl_llr_mode_str(enum sbl_llr_mode mode)
{
	switch (mode) {
	case SBL_LLR_MODE_INVALID:  return "invalid";
	case SBL_LLR_MODE_UNKNOWN:  return "unknown";
	case SBL_LLR_MODE_OFF:      return "off";
	case SBL_LLR_MODE_MONITOR:  return "monitor";
	case SBL_LLR_MODE_ON:       return "on";
	case SBL_LLR_MODE_AUTO:     return "auto";
	default:                    return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_llr_mode_str);


const char *sbl_llr_down_behaviour_str(
		enum sbl_llr_down_behaviour behaviour)
{
	switch (behaviour) {
	case SBL_LLR_LINK_DOWN_INVALID:      return "invalid";
	case SBL_LLR_LINK_DOWN_UNKNOWN:      return "unknown";
	case SBL_LLR_LINK_DOWN_DISCARD:      return "discard";
	case SBL_LLR_LINK_DOWN_BLOCK:        return "block";
	case SBL_LLR_LINK_DOWN_BEST_EFFORT:  return "best-effort";
	default:                             return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_llr_down_behaviour_str);


const char *sbl_loopback_mode_str(enum sbl_loopback_mode state)
{
	switch (state) {
	case SBL_LOOPBACK_MODE_INVALID:  return "invalid";
	case SBL_LOOPBACK_MODE_LOCAL:    return "local";
	case SBL_LOOPBACK_MODE_REMOTE:   return "remote";
	case SBL_LOOPBACK_MODE_OFF:      return "off";
	default:                         return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_loopback_mode_str);


const char *sbl_serdes_effort_str(u32 effort)
{
	switch (effort) {
	case SPICO_INT_DATA_ICAL_EFFORT_1:   return "max";
	case SPICO_INT_DATA_ICAL_EFFORT_10:  return "med";
	case SPICO_INT_DATA_ICAL_EFFORT_0:   return "min";
	default:                             return "unrecognized";
	}
}


const char *sbl_serdes_state_str(enum sbl_serdes_status state)
{
	switch (state) {
	case SBL_SERDES_STATUS_UNKNOWN:             return "unknown";
	case SBL_SERDES_STATUS_AUTONEG:             return "autoneg-mode";
	case SBL_SERDES_STATUS_LPD_MT:              return "lpd-mt";
	case SBL_SERDES_STATUS_DOWN:                return "down";
	case SBL_SERDES_STATUS_TUNING:              return "tuning";
	case SBL_SERDES_STATUS_RUNNING:             return "running";
	case SBL_SERDES_STATUS_ERROR:               return "error";
	case SBL_SERDES_STATUS_RESETTING:           return "resetting";
	default:                                    return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_serdes_state_str);


const char *sbl_an_state_str(int state)
{
	switch (state) {
	case SBL_PML_AUTONEG_STATE_AUTONEG_OFF:    return "off";
	case SBL_PML_AUTONEG_STATE_AUTONEG_ENABLE: return "enable";
	case SBL_PML_AUTONEG_STATE_TX_DISABLE:     return "tx_disable";
	case SBL_PML_AUTONEG_STATE_ABILITY_DETECT: return "ability_detect";
	case SBL_PML_AUTONEG_STATE_ACK_DETECT:     return "ack_detect";
	case SBL_PML_AUTONEG_STATE_COMPLETE_ACK:   return "complete_ack";
	case SBL_PML_AUTONEG_STATE_NEXT_PAGE_WAIT: return "next_page_wait";
	case SBL_PML_AUTONEG_STATE_AN_GOOD_CHECK:  return "good_check";
	case SBL_PML_AUTONEG_STATE_AN_GOOD:        return "good";
	default:                                   return "unrecognized";
	}
}


const char *sbl_async_alert_str(enum sbl_async_alert_type alert_type)
{
	switch (alert_type) {
	case SBL_ASYNC_ALERT_INVALID:              return "invalid";
	case SBL_ASYNC_ALERT_LINK_DOWN:            return "link down";
	case SBL_ASYNC_ALERT_SERDES_FW_CORRUPTION: return "serdes fw corruption";
	case SBL_ASYNC_ALERT_TX_DEGRADE:           return "tx degrade";
	case SBL_ASYNC_ALERT_RX_DEGRADE:           return "rx degrade";
	case SBL_ASYNC_ALERT_TX_DEGRADE_FAILURE:   return "tx degrade failure";
	case SBL_ASYNC_ALERT_RX_DEGRADE_FAILURE:   return "rx degrade failure";
	case SBL_ASYNC_ALERT_SBM_FW_LOAD_FAILURE:  return "sbus master fw load failure";
	default:                                   return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_async_alert_str);


const char *sbl_fec_discard_str(enum sbl_fec_discard_type discard_type)
{
	switch (discard_type) {
	case SBL_FEC_DISCARD_TYPE_INVALID:         return "invalid";
	case SBL_FEC_DISCARD_TYPE_RX_DEGRADE:      return "rx lane degraded";
	case SBL_FEC_DISCARD_TYPE_PML_REC_START:   return "pml recovery started";
	case SBL_FEC_DISCARD_TYPE_PML_REC_END:     return "pml recovery ended";
	default:                                   return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_fec_discard_str);


const char *sbl_down_origin_str(enum sbl_link_down_origin down_origin)
{
	switch (down_origin) {
	case SBL_LINK_DOWN_ORIGIN_UNKNOWN:         return "unknown";
	case SBL_LINK_DOWN_ORIGIN_LINK_DOWN:       return "link down";
	case SBL_LINK_DOWN_ORIGIN_LOCAL_FAULT:     return "local fault";
	case SBL_LINK_DOWN_ORIGIN_REMOTE_FAULT:    return "remote fault";
	case SBL_LINK_DOWN_ORIGIN_ALIGN:           return "align";
	case SBL_LINK_DOWN_ORIGIN_HISER:           return "hiser";
	case SBL_LINK_DOWN_ORIGIN_LLR_MAX:         return "max llr replay";
	case SBL_LINK_DOWN_ORIGIN_DEGRADE_FAILURE: return "degrade failure";
	default:                                   return "unrecognized";
	}
}
EXPORT_SYMBOL(sbl_down_origin_str);

/*
 * look at flags and get the polling interval
 *
 *   return 0 if no flag is found
 */
int sbl_flags_get_poll_interval_from_flags(unsigned int flags)
{
	if (flags&SBL_FLAG_INTERVAL_1MS)
		return 1;
	else if (flags&SBL_FLAG_INTERVAL_10MS)
		return 10;
	else if (flags&SBL_FLAG_INTERVAL_100MS)
		return 100;
	else if (flags&SBL_FLAG_INTERVAL_1S)
		return 1000;
	else
		return 0;
}
EXPORT_SYMBOL(sbl_flags_get_poll_interval_from_flags);


/*
 * look at flags and get the polling delay
 *
 *   return 0 if no flag is found
 */
int sbl_flags_get_delay_from_flags(unsigned int flags)
{
	if (flags&SBL_FLAG_DELAY_3US)
		return 3;
	else if (flags&SBL_FLAG_DELAY_4US)
		return 4;
	else if (flags&SBL_FLAG_DELAY_5US)
		return 5;
	else if (flags&SBL_FLAG_DELAY_10US)
		return 10;
	else if (flags&SBL_FLAG_DELAY_20US)
		return 20;
	else if (flags&SBL_FLAG_DELAY_50US)
		return 50;
	else if (flags&SBL_FLAG_DELAY_100US)
		return 100;
	else
		return 0;
}
EXPORT_SYMBOL(sbl_flags_get_delay_from_flags);
