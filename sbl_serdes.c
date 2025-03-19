// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2022, 2024 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "uapi/sbl_kconfig.h"
#include "sbl.h"
#include "sbl_constants.h"
#include "sbl_serdes.h"
#include "sbl_config_list.h"
#include "sbl_serdes_map.h"
#include "sbl_serdes_fn.h"
#include "sbl_internal.h"

static atomic_t sbl_next_serdes_config_tag = ATOMIC_INIT(-1);

static int sbl_serdes_stop_internal(struct sbl_inst *sbl, int port_num);
static int sbl_serdes_optical_lock_delay(struct sbl_inst *sbl, int port_num);


#if defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined(CONFIG_SBL_PLATFORM_CAS_SIM)
int sbl_serdes_load(struct sbl_inst *sbl, int port_num, bool force)
{
	return 0;
}
#else
int sbl_serdes_load(struct sbl_inst *sbl, int port_num, bool force)
{
	int err;

	if (port_num == SBL_ALL_PORTS) {
		err = sbl_apply_sbus_divider(sbl, SBL_SBUS_DIVIDER_DFLT);
		if (err) {
			sbl_dev_err(sbl->dev, "p%d: SBus speedup failed [%d]", port_num, err);
			return err;
		}
	}

	/* retrieve, flash and release SBM firmware */
	if (port_num == SBL_ALL_PORTS) {
		err = sbl_sbm_firmware_flash(sbl);
		if (err) {
			sbl_dev_err(sbl->dev, "p%d: failed to flash SBM fw [%d]", port_num, err);
			return err;
		}
	}

	/* retrieve and flash SerDes firmware */
	err = sbl_serdes_firmware_flash(sbl, port_num, force);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: failed to flash SerDes fw [%d]",
			port_num, err);
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_SBL_PLATFORM_CAS_EMU) || defined (CONFIG_SBL_PLATFORM_CAS_SIM) */
EXPORT_SYMBOL(sbl_serdes_load);


/*
 * link partner detection
 *
 * This function will attempt a serdes minitune. This fast tune will only
 * succeed if the lp is sending AM using the same mode as we are using.
 *
 */
int sbl_serdes_lp_detect(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;
	int tmp_err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	/* in loopback mode the link partner (ourself) is always present! */
	if (link->loopback_mode == SBL_LOOPBACK_MODE_LOCAL)
		return 0;

	if (link->sstate != SBL_SERDES_STATUS_DOWN) {
		sbl_dev_err(sbl->dev, "p%d: serdes_lpd wrong state to start (%s)",
			port_num, sbl_serdes_state_str(link->sstate));
		/* leave state unchanged */
		return -EUCLEAN;
	}

	link->sstate = SBL_SERDES_STATUS_LPD_MT;
	link->serr = 0;

	for (link->lpd_try_count = 0; true; ++link->lpd_try_count) {

		err = sbl_serdes_minitune_setup(sbl, port_num);
		if (err) {
			sbl_dev_err(sbl->dev, "p%d: serdes_lpd setup failed [%d]",
				port_num, err);
			goto out_err;
		}

		/*
		 * If the other end is there then we should block for no more
		 * than 5s
		 */
		err = sbl_serdes_minitune_block(sbl, port_num);
		switch (err) {
		case 0:
			sbl_dev_dbg(sbl->dev, "p%d: serdes_lpd done", port_num);
			link->lp_detected = true;
			goto out_done;

		case -ETIMEDOUT:
			sbl_dev_warn(sbl->dev, "p%d: serdes_lpd timed out", port_num);
			/*
			 * Occasionally serdes minitune will fail even though the link
			 * partner is actually there!
			 * Sometimes we can recover by resetting the serdes PLLs and/or
			 * reloading the serdes firmware
			 */
			if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_SPLL_RESET)) {
				tmp_err = sbl_reset_serdes_plls(sbl, port_num);
				if (tmp_err) {
					/* if this fails nothing we can do */
					sbl_dev_err(sbl->dev, "p%d: serdes_lpd pll reset failed [%d]",
						port_num, tmp_err);
					/* TODO maybe force fw reload here? */
				}
			}

			if (link->blattr.options & SBL_OPT_RELOAD_FW_ON_TIMEOUT)
				link->reload_serdes_fw = true;

			goto out_done;

		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "p%d: serdes_lpd cancelled", port_num);
			goto out_done;

		case -ETIME:
			sbl_dev_dbg(sbl->dev, "p%d: serdes_lpd nothing detected", port_num);
			/* just try again */
			break;

		default:
			sbl_dev_err(sbl->dev, "p%d: serdes_lpd block failed [%d]", port_num, err);
			goto out_err;
		}
	};

out_done:
	link->sstate = SBL_SERDES_STATUS_DOWN;
	link->serr = 0;
	return err;

out_err:
	/* serdes is broken and requires reset */
	link->sstate = SBL_SERDES_STATUS_ERROR;
	link->serr = err;
	return err;
}


int sbl_serdes_start(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;
	int tmp_err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "p%d: SerDes start", port_num);

	link = sbl->link + port_num;

	if (link->sstate != SBL_SERDES_STATUS_DOWN) {
		sbl_dev_err(sbl->dev, "p%d: SerDes start: wrong state (%s)",
			port_num, sbl_serdes_state_str(link->sstate));
		/* leave state unchanged */
		return -EUCLEAN;
	}

	/* configure serdes */
	err = sbl_serdes_config(sbl, port_num, false);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: SerDes start: serdes_config failed [%d]", port_num, err);
		if (err == -EBADE)
			link->reload_serdes_fw = true;
		goto out;
	}

	/*
	 * wait a while before starting to tune to let the optics lock if present
	 * (in local loopback mode however there are never any optics to wait for)
	 */
	if (link->blattr.loopback_mode != SBL_LOOPBACK_MODE_LOCAL) {
		if (link->mattr.media == SBL_LINK_MEDIA_OPTICAL) {
			err = sbl_serdes_optical_lock_delay(sbl, port_num);
			if (err)
				goto out;
		}
	}

	/* make sure we have time left for at lease 2 tuning cycles */
	sbl_start_timeout_ensure_remaining(sbl, port_num,
			2*(link->blattr.dfe_timeout + link->blattr.dfe_pre_delay));

	/* tune serdes */
	link->sstate = SBL_SERDES_STATUS_TUNING;

	err = sbl_serdes_tuning(sbl, port_num);
	if (err) {
		switch (err) {
		case -ECANCELED:
			sbl_dev_dbg(sbl->dev, "p%d: SerDes start: tuning cancelled", port_num);
			break;
		case -ETIMEDOUT:
			sbl_dev_dbg(sbl->dev, "p%d: SerDes start: tuning timed out", port_num);
			/*
			 * occasionally tuning can fail even thought the lp is good
			 * resetting the serdes rx plls or reloading the serdes fw can
			 * sometimes fix this
			 */
			if (!sbl_debug_option(sbl, port_num, SBL_DEBUG_INHIBIT_SPLL_RESET)) {
				tmp_err = sbl_reset_serdes_plls(sbl, port_num);
				if (tmp_err) {
					/* if this fails nothing we can do */
					sbl_dev_err(sbl->dev, "p%d: SerDes start: reset_serdes_plls failed [%d]",
							port_num, tmp_err);
					/* TODO maybe force fw reload here? */
				}
			}

			if (link->blattr.options & SBL_OPT_RELOAD_FW_ON_TIMEOUT)
				link->reload_serdes_fw = true;
			break;
		default:
			sbl_dev_err(sbl->dev, "p%d: SerDes start: serdes_tuning failed [%d]", port_num, err);
			break;
		}
	}

out:
	/* update status */
	if (err) {
		link->sstate = SBL_SERDES_STATUS_ERROR;
		link->serr = err;
	} else {
		sbl_dev_dbg(sbl->dev, "p%d: SerDes start: done", port_num);
		link->sstate = SBL_SERDES_STATUS_RUNNING;
		link->serr = 0;
	}

	return err;
}


int sbl_serdes_stop(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	if (!(link->sstate & (SBL_SERDES_STATUS_RUNNING|SBL_SERDES_STATUS_AUTONEG))) {
		sbl_dev_err(sbl->dev, "p%d: SerDes stop: wrong state (%s)",
			port_num, sbl_serdes_state_str(link->sstate));
		/* leave state unchanged */
		return -EUCLEAN;
	}

	if (sbl_debug_option(sbl, port_num, SBL_DEBUG_KEEP_SERDES_UP)) {
		sbl_dev_err(sbl->dev, "p%d: KEEP SERDES UP", port_num);
		return 0;
	}

	return sbl_serdes_stop_internal(sbl, port_num);
}


static int sbl_serdes_stop_internal(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	int err = 0;
	int serdes;

	sbl_dev_dbg(sbl->dev, "p%d: SerDes stop", port_num);

	err = sbl_port_stop_pcal(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: SerDes stop: port_pcal_stop failed [%d]",
				port_num, err);
		goto out;
	}

	for (serdes = 0; serdes < sbl->switch_info->num_serdes; ++serdes) {
		err = sbl_set_tx_rx_enable(sbl, port_num, serdes,
						false, false, false);
		if (err) {
			sbl_dev_err(sbl->dev,
				"p%d: SerDes stop: disable failed [%d]",
				port_num, err);
			goto out;
		}
	}

	err = sbl_spico_reset(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: SerDes stop: spico_reset failed [%d]",
			 port_num, err);
		goto out;
	}

out:
	if (err) {
		link->sstate = SBL_SERDES_STATUS_ERROR;
		/* Try and recover from errors with FW reload */
		link->reload_serdes_fw = true;
	} else {
		sbl_dev_dbg(sbl->dev, "p%d: SerDes stop: done", port_num);
		link->sstate = SBL_SERDES_STATUS_DOWN;
	}
	link->serr = err;

	return err;
}


int sbl_serdes_reset(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int fail_reset_serdes;
	int serdes;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "p%d: SerDes reset", port_num);

	link = sbl->link + port_num;
	link->sstate = SBL_SERDES_STATUS_RESETTING;

	/* optionally clear any saved tuning params */
	if (link->blattr.options & SBL_OPT_RESET_CLEAR_PARAMS) {
		link->tuning_params.tp_state_hash0 = 0;
		link->tuning_params.tp_state_hash1 = 0;
		for (serdes = 0; serdes < sbl->switch_info->num_serdes; ++serdes) {
			memset(link->tuning_params.params[serdes].ctle, 0,
					sizeof(u16)*NUM_CTLE_PARAMS);
			memset(link->tuning_params.params[serdes].ffe, 0,
					sizeof(u16)*NUM_FFE_PARAMS);
			memset(link->tuning_params.params[serdes].dfe, 0,
					sizeof(u16)*NUM_DFE_PARAMS);
			memset(link->tuning_params.params[serdes].rxvs, 0,
				   sizeof(u16)*NUM_RXVS_PARAMS);
			memset(link->tuning_params.params[serdes].rxvc, 0,
				   sizeof(u16)*NUM_RXVC_PARAMS);
			memset(link->tuning_params.params[serdes].rsdo, 0,
				   sizeof(u16)*NUM_RSDO_PARAMS);
			memset(link->tuning_params.params[serdes].rsdc, 0,
				   sizeof(u16)*NUM_RSDC_PARAMS);
			memset(link->tuning_params.params[serdes].rsto, 0,
				   sizeof(u16)*NUM_RSTO_PARAMS);
			memset(link->tuning_params.params[serdes].rstc, 0,
				   sizeof(u16)*NUM_RSTC_PARAMS);
			memset(link->tuning_params.params[serdes].eh, 0,
				   sizeof(u16)*NUM_EH_PARAMS);
			memset(link->tuning_params.params[serdes].gtp, 0,
				   sizeof(u16)*NUM_GTP_PARAMS);
			memset(link->tuning_params.params[serdes].dccd, 0,
				   sizeof(u16)*NUM_DCCD_PARAMS);
			memset(link->tuning_params.params[serdes].p4lv, 0,
				   sizeof(u16)*NUM_P4LV_PARAMS);
			memset(link->tuning_params.params[serdes].afec, 0,
				   sizeof(u16)*NUM_AFEC_PARAMS);
		}
	}

	/* Stop continuous tune */
	err = sbl_port_stop_pcal(sbl, port_num);
	if (err)
		sbl_dev_warn(sbl->dev, "p%d: SerDes reset: port_stop_pcal failed [%d]",
			 port_num, err);

	/* Previously sbl_spico_reset() is now changed to
	 * sbl_serdes_soft_reset() to avoid SBUS lockups - SSHOTPLAT-2222.
	 * Upon error, spico reset will reload serdes firmware.
	 * Note, reloading the firmware can be forced with a
	 * debug option.
	 */
	for (serdes = 0; serdes < sbl->switch_info->num_serdes; ++serdes) {
		err = sbl_serdes_soft_reset(sbl, port_num, serdes);
		if (err) {
			fail_reset_serdes = serdes;
			break;
		}
	}
	if (err)
		sbl_dev_err(sbl->dev, "p%ds%d: SerDes reset: serdes_soft_reset failed [%d]",
				port_num, fail_reset_serdes, err);
	else if (sbl_debug_option(sbl, port_num, SBL_DEBUG_FORCE_RELOAD_SERDES_FW))
		/* Skip out_success and continue to SPICO reload */
		sbl_dev_info(sbl->dev, "p%d: SPICO force serdes_firmware_flash_safe", port_num);
	else
		goto out_success;

	/* reload fw */
	err = sbl_serdes_firmware_flash_safe(sbl, port_num, true);
	if (err)
		sbl_dev_err(sbl->dev, "p%d: SerDes reset: serdes_firmware_flash_safe failed [%d]",
				port_num, err);
	else
		goto out_success;

	/*
	 * This is really bad!
	 * There is nothing we can do but say we are in error and
	 * be reset again
	 */
	sbl_dev_err(sbl->dev, "p%d: SerDes reset: failed [%d]", port_num, err);
	link->sstate = SBL_SERDES_STATUS_ERROR;
	link->serr = err;

	return err;

out_success:
	/* serdes should be fine */
	sbl_dev_dbg(sbl->dev, "p%d: SerDes reset: done", port_num);
	link->sstate = SBL_SERDES_STATUS_DOWN;
	link->serr = 0;

	return 0;
}


int sbl_an_serdes_start(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	if (link->mattr.media != SBL_LINK_MEDIA_ELECTRICAL) {
		sbl_dev_err(sbl->dev, "p%d: SerDes AN start - media not electrical",
			port_num);
		return -EINVAL;
	}

	if ((link->blattr.pec.an_mode != SBL_AN_MODE_ON) &&
	    (link->blattr.pec.an_mode != SBL_AN_MODE_FIXED)) {
		sbl_dev_err(sbl->dev, "p%d: SerDes AN start - bad mode specified (%s)",
			port_num, sbl_an_mode_str(link->blattr.pec.an_mode));
		return -EINVAL;
	}

	if (link->sstate != SBL_SERDES_STATUS_DOWN) {
		sbl_dev_err(sbl->dev, "p%d: SerDes AN start - wrong state (%s)",
			port_num, sbl_serdes_state_str(link->sstate));
		/* leave state unchanged */
		return -EUCLEAN;
	}

	err = sbl_serdes_stop_internal(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: SerDes AN start - stop internal failed [%d]",
			port_num, err);
		goto out_err;
	}

	err = sbl_serdes_config(sbl, port_num, true);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: SerDes AN start - config failed [%d]",
			port_num, err);
		goto out_err;
	}

	sbl_dev_dbg(sbl->dev, "p%d: SerDes AN started", port_num);
	link->sstate = SBL_SERDES_STATUS_AUTONEG;
	link->serr = 0;
	return 0;

out_err:
	link->sstate = SBL_SERDES_STATUS_ERROR;
	link->serr = err;
	return err;
}


int sbl_an_serdes_stop(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	if (link->sstate != SBL_SERDES_STATUS_AUTONEG) {
		sbl_dev_err(sbl->dev, "p%d: SerDes in wrong state (%s) for AN stop\n",
				port_num, sbl_serdes_state_str(link->sstate));
		/* leave state unchanged */
		return -EUCLEAN;
	}

	err = sbl_serdes_stop_internal(sbl, port_num);
	if (err) {
		sbl_dev_err(sbl->dev, "p%d: SerDes AN stop failed [%d]\n",
				port_num, err);
		link->sstate = SBL_SERDES_STATUS_ERROR;
	} else {
		sbl_dev_dbg(sbl->dev, "p%d: SerDes AN stopped", port_num);
		link->sstate = SBL_SERDES_STATUS_DOWN;
	}
	link->serr = err;
	return err;
}


/*
 * This delay is to give time for the optical transceivers to lock
 */
static int sbl_serdes_optical_lock_delay(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	unsigned long last_jiffy;
	int err = 0;

	sbl_dev_dbg(sbl->dev, "p%d: optical lock delay", port_num);

	if (link->blattr.config_target != SBL_BASE_LINK_CONFIG_AOC) {
		sbl_dev_err(sbl->dev, "p%d: optical lock delay - config not optical", port_num);
		return -EINVAL;
	}

	link->optical_delay_active = true;
	last_jiffy = jiffies + msecs_to_jiffies(link->blattr.aoc.optical_lock_delay);
	while (time_is_after_jiffies(last_jiffy)) {
		if (sbl_base_link_start_cancelled(sbl, port_num)) {
			err = -ECANCELED;
			goto out;
		}
		if (sbl_start_timeout(sbl, port_num)) {
			err = -ETIMEDOUT;
			goto out;
		}
		msleep(link->blattr.aoc.optical_lock_interval);
	}
out:
	link->optical_delay_active = false;
	return err;
}


/*
 * Saved tuning params
 *
 */
int sbl_serdes_get_tuning_params(struct sbl_inst *sbl, int port_num,
		struct sbl_tuning_params *tuning_params)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	/* don't return invalid parameters */
	mutex_lock(&link->tuning_params_mtx);
	if (link->tuning_params.magic != SBL_TUNING_PARAM_MAGIC) {
		mutex_unlock(&link->tuning_params_mtx);
		return -ENODATA;
	}

	sbl_dev_dbg(sbl->dev, "p%d: tp get - returning hash0 0x%llx hash1 0x%llx\n",
		port_num, link->tuning_params.tp_state_hash0,
		link->tuning_params.tp_state_hash1);

	memcpy(tuning_params, &link->tuning_params, sizeof(struct sbl_tuning_params));
	mutex_unlock(&link->tuning_params_mtx);

	return 0;
}
EXPORT_SYMBOL(sbl_serdes_get_tuning_params);


int sbl_serdes_set_tuning_params(struct sbl_inst *sbl, int port_num,
		struct sbl_tuning_params *tuning_params)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	if (tuning_params->magic != SBL_TUNING_PARAM_MAGIC)
		return -EINVAL;

	/* only support one verion for now */
	if (tuning_params->version != SBL_TUNING_PARAM_VERSION)
		return -EINVAL;

	mutex_lock(&link->tuning_params_mtx);
	memcpy(&link->tuning_params, tuning_params, sizeof(struct sbl_tuning_params));
	mutex_unlock(&link->tuning_params_mtx);

	sbl_dev_dbg(sbl->dev, "p%d: tp set - received hash0 0x%llx hash1 0x%llx\n",
		port_num, link->tuning_params.tp_state_hash0,
		link->tuning_params.tp_state_hash1);

	return 0;
}
EXPORT_SYMBOL(sbl_serdes_set_tuning_params);


int sbl_serdes_invalidate_tuning_params(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	mutex_lock(&link->tuning_params_mtx);
	link->tuning_params.magic = 0;               /* prevent transfer to usr-space */
	link->tuning_params.tp_state_hash0 = 0;      /* prevent use */
	link->tuning_params.tp_state_hash1 = 0;      /* prevent use */
	mutex_unlock(&link->tuning_params_mtx);

	return 0;
}
EXPORT_SYMBOL(sbl_serdes_invalidate_tuning_params);


int sbl_serdes_invalidate_all_tuning_params(struct sbl_inst *sbl)
{
	int i;

	for (i = 0; i < sbl->switch_info->num_ports; ++i)
		sbl_serdes_invalidate_tuning_params(sbl, i);

	return 0;
}
EXPORT_SYMBOL(sbl_serdes_invalidate_all_tuning_params);

/*
 * Serdes configuration addition/removal support
 *
 */
int sbl_serdes_add_config(struct sbl_inst *sbl,
		u64 tp_state_mask0, u64 tp_state_mask1,
		u64 tp_state_match0, u64 tp_state_match1,
		u64 port_mask, u8 serdes_mask, struct sbl_sc_values *vals,
		bool is_default)
{
	struct sbl_serdes_config *new_sc;
	struct sbl_serdes_config *sc;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	if (vals->magic != SBL_SERDES_CONFIG_MAGIC)
		return -EINVAL;

	sbl_dev_dbg(sbl->dev, "serdes add config");

	new_sc = kmalloc(sizeof(struct sbl_serdes_config), GFP_KERNEL);
	if (!new_sc)
		return -ENOMEM;

	new_sc->tp_state_mask0  = tp_state_mask0;
	new_sc->tp_state_mask1  = tp_state_mask1;
	new_sc->tp_state_match0 = tp_state_match0;
	new_sc->tp_state_match1 = tp_state_match1;
	new_sc->port_mask       = port_mask;
	new_sc->serdes_mask     = serdes_mask;
	new_sc->is_default      = is_default;
	new_sc->tag             = sbl_serdes_get_config_tag();
	memcpy(&new_sc->vals, vals, sizeof(struct sbl_sc_values));
	INIT_LIST_HEAD(&new_sc->list);

	spin_lock(&sbl->serdes_config_lock);

	/* check new entry is unique */
	list_for_each_entry(sc, &sbl->serdes_config_list, list) {
		if ((sc->port_mask == port_mask) &&
				(sc->serdes_mask == serdes_mask) &&
				(sc->tp_state_mask0 == tp_state_mask0) &&
				(sc->tp_state_mask1 == tp_state_mask1) &&
				(sc->tp_state_match0 == tp_state_match0) &&
				(sc->tp_state_match1 == tp_state_match1)) {
			spin_unlock(&sbl->serdes_config_lock);
			kfree(new_sc);
			return -EEXIST;
		}
	}

	/* add the new entry */
	list_add(&new_sc->list, &sbl->serdes_config_list);
	sbl_dev_dbg(sbl->dev, "added serdes config, tag %d\n", sc->tag);
	spin_unlock(&sbl->serdes_config_lock);
	return 0;
}
EXPORT_SYMBOL(sbl_serdes_add_config);


int sbl_serdes_del_config(struct sbl_inst *sbl,
		u64 tp_state_mask0, u64 tp_state_mask1,
		u64 tp_state_match0, u64 tp_state_match1,
		u64 port_mask, u8  serdes_mask)
{
	struct sbl_serdes_config *sc;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "serdes del config");

	spin_lock(&sbl->serdes_config_lock);

	/* find the entry and remove it */
	list_for_each_entry(sc, &sbl->serdes_config_list, list) {
		if ((sc->port_mask == port_mask) &&
				(sc->serdes_mask == serdes_mask) &&
				(sc->tp_state_mask0 == tp_state_mask0) &&
				(sc->tp_state_mask1 == tp_state_mask1) &&
				(sc->tp_state_match0 == tp_state_match0) &&
				(sc->tp_state_match1 == tp_state_match1)) {
			list_del(&sc->list);
			spin_unlock(&sbl->serdes_config_lock);
			sbl_dev_dbg(sbl->dev, "deleted serdes config, tag %d\n", sc->tag);
			kfree(sc);
			return 0;
		}
	}

	/* not found */
	spin_unlock(&sbl->serdes_config_lock);
	return -ENOENT;
}
EXPORT_SYMBOL(sbl_serdes_del_config);


int sbl_serdes_clear_all_configs(struct sbl_inst *sbl, bool clr_default)
{
	struct sbl_serdes_config *sc;
	struct sbl_serdes_config *tmp_sc;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "serdes clear all configs");

	/* find the entries and remove them */
	spin_lock(&sbl->serdes_config_lock);
	list_for_each_entry_safe(sc, tmp_sc, &sbl->serdes_config_list, list) {
		/* clear if not default or we want to clear the default */
		if (!sc->is_default || clr_default) {
			list_del(&sc->list);
			kfree(sc);
		}
	}
	spin_unlock(&sbl->serdes_config_lock);

	return 0;
}
EXPORT_SYMBOL(sbl_serdes_clear_all_configs);


void sbl_serdes_dump_configs(struct sbl_inst *sbl)
{
	struct sbl_serdes_config *sc;
	int count = 0;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return;

	spin_lock(&sbl->serdes_config_lock);
	list_for_each_entry_reverse(sc, &sbl->serdes_config_list, list) {
		sbl_dev_info(sbl->dev,
			"serdes config %d: dflt: %d, tag %d, mask0 0x%llx, match0 0x%llx, mask1 0x%llx, match1 0x%llx, ports 0x%llx, serdes 0x%x\n",
			count++, sc->is_default, sc->tag, sc->tp_state_mask0, sc->tp_state_match0, sc->tp_state_mask1, sc->tp_state_match1, sc->port_mask, sc->serdes_mask);
	}
	spin_unlock(&sbl->serdes_config_lock);
}
EXPORT_SYMBOL(sbl_serdes_dump_configs);


u32 sbl_serdes_get_config_tag(void)
{
	return atomic_inc_return(&sbl_next_serdes_config_tag);
}

/*
 * sysfs state output
 */
#ifdef CONFIG_SYSFS
int sbl_serdes_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	struct sbl_link *link = sbl->link + port_num;
	int s = 0;
	int i;
	int s_fw_reload_count[4] = {0};
	int s_spico_reset_count[4] = {0};
	int s_pll_reset_count[4] = {0};

	spin_lock(&link->lock);
	s += snprintf(buf+s, size-s, "serdes: %s",
			sbl_serdes_state_str(link->sstate));
	switch (link->sstate) {
	case SBL_SERDES_STATUS_ERROR:
		s += snprintf(buf+s, size-s, " [%d]", link->serr);
		break;
	case SBL_SERDES_STATUS_LPD_MT:
		s += snprintf(buf+s, size-s, " cnt %d", link->lpd_try_count);
		break;
	case SBL_SERDES_STATUS_RUNNING:
		if (link->loopback_mode != SBL_LOOPBACK_MODE_OFF) {
			s += snprintf(buf+s, size-s, ", loopback: %s",
					sbl_loopback_mode_str(link->loopback_mode));
		}
		s += snprintf(buf+s, size-s, ", precoding: %s",
				(link->precoding_enabled) ? "on" : "off");
		if (link->dfe_tune_count == SBL_DFE_USED_SAVED_PARAMS) {
			s += snprintf(buf+s, size-s, ", tune: used-saved-params");
		} else {
			s += snprintf(buf+s, size-s, ", tune: cnt %d, eff %s (%lld.%.3ld/%lld.%.3ld)",
					link->dfe_tune_count,
					sbl_serdes_effort_str(link->ical_effort),
					(long long)link->tune_time.tv_sec,
					link->tune_time.tv_nsec/1000000,
					(long long)link->total_tune_time.tv_sec,
					link->total_tune_time.tv_nsec/1000000);
		}
		break;
	case SBL_SERDES_STATUS_TUNING:
		if (link->dfe_tune_count == SBL_DFE_USED_SAVED_PARAMS) {
			s += snprintf(buf+s, size-s, ", loading-saved-params");
		} else {
			s += snprintf(buf+s, size-s, ", cnt %d, eff %s (%d/%d)",
					link->dfe_tune_count,
					sbl_serdes_effort_str(link->ical_effort),
					sbl_link_tune_elapsed(sbl, port_num),
					link->blattr.dfe_timeout);
		}
		break;
	default:
		break;
	}

	s += snprintf(buf+s, size-s, ", pcal: %s",
			(link->pcal_running) ? "on" : "off");
	if (link->dfe_predelay_active)
		s += snprintf(buf+s, size-s, ", pre-delay");
	if (link->optical_delay_active)
		s += snprintf(buf+s, size-s, ", optical-delay");
	s += snprintf(buf+s, size-s, ", fw_reload_skip_cnt: %d",
			sbl_link_counters_read(sbl, port_num, serdes_fw_reload_skip));
	s += snprintf(buf+s, size-s, "\n");

	s += snprintf(buf+s, size-s, "serdes-reload-counters[fw,spico,pll]:");
	sbl_link_counters_get(sbl, port_num, s_fw_reload_count,
							serdes0_fw_reload, 4);
	sbl_link_counters_get(sbl, port_num, s_spico_reset_count,
							serdes0_spico_reset, 4);
	sbl_link_counters_get(sbl, port_num, s_pll_reset_count,
							serdes0_pll_reset, 4);
	for (i = 0; i < sbl->switch_info->num_serdes; ++i) {
		s += snprintf(buf+s, size-s, " s%d:[%d,%d,%d]", i, s_fw_reload_count[i],
					s_spico_reset_count[i], s_pll_reset_count[i]);
	}
	s += snprintf(buf+s, size-s, "\n");
	spin_unlock(&link->lock);

	return s;
}
EXPORT_SYMBOL(sbl_serdes_sysfs_sprint);

/**
 * @brief Get the serdes firmware version information for the port as a string.
 * @details Multiple serdes firmware versions exist per port. Each serdes lane
 * firmware version is listed on a newline.
 * @param[in] sbl	Slingshot base link instance.
 * @param[in] port_num	Port number to get the serdes firmware version for.
 * @param[out] buf	Serdes firmware version string.
 * @param[in] size	Size of the buffer allocated for the Serdes firmware version string.
 *
 * @return On success the number of bytes written to the buffer is returned.
 * -EINVAL is returned when the sbl instance or port number is invalid and
 * -ENOMEM when the buf points to NULL or size is 0.
 */
int sbl_serdes_fw_sysfs_sprint(struct sbl_inst *sbl, int port_num, char *buf, size_t size)
{
	int serdes = 0;
	struct fw {
		uint rev;
		uint build;
	} fw[SBL_SERDES_LANES_PER_PORT];
	ssize_t s = 0;

	int err = sbl_validate_instance(sbl);

	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	if (buf == NULL || size == 0U)
		return -ENOMEM;

	memset(fw, 0, sizeof(fw));
	for (serdes = 0; serdes < SBL_SERDES_LANES_PER_PORT; ++serdes) {
		struct fw *pfw = fw + serdes;

		sbl_serdes_get_fw_vers(sbl, port_num, serdes,
			&pfw->rev, &pfw->build);

		s += snprintf(buf + s, size - s,
			"0x%4.4x_%4.4x\n", pfw->rev, pfw->build);
	}

	return s;
}
EXPORT_SYMBOL(sbl_serdes_fw_sysfs_sprint);

/**
 * @brief Get the sbm firmware version information for the sbus ring as a string.
 * @param[in] sbl		Slingshot base link instance.
 * @param[in] ring		SBUS ring number to get the serdes firmware version for.
 * @param[out] buf		SBUS ring firmware version string.
 * @param[in] size		Size of the buffer allocated to hold the fw version string.
 *
 * @return On success number of bytes written to the buffer. -EINVAL when the sbl
 * instance or ring number is invalid. -ENOMEM when the buf points to NULL or size
 * is 0.
 */
int sbl_sbm_fw_sysfs_sprint(struct sbl_inst *sbl, int ring, char *buf, size_t size)
{
	uint fw_rev, fw_build = 0;

	int err = sbl_validate_instance(sbl);

	if (err)
		return err;

	if (sbl_get_num_sbus_rings(sbl) < ring || ring < 0)
		return -EINVAL;

	if (buf == NULL || size == 0U)
		return -ENOMEM;

	sbl_sbm_get_fw_vers(sbl, ring, &fw_rev, &fw_build);
	return snprintf(buf, size, "0x%4.4x_%4.4x\n", fw_rev, fw_build);
}
EXPORT_SYMBOL(sbl_sbm_fw_sysfs_sprint);

#endif
