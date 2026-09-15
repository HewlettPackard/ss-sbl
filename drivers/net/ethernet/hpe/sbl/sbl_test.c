// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2021 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/device.h>

#include <linux/hpe/sbl/sbl.h>
#include <linux/hpe/sbl/sbl_kconfig.h>
#include <linux/hpe/sbl/sbl_test.h>

#include <uapi/ethernet/sbl_serdes_defaults.h>

#include "sbl_serdes.h"
#include "sbl_serdes_fn.h"
#include "sbl_pml_fn.h"
#include "sbl_internal.h"
#include "sbl_constants.h"

static bool sbl_test_crc_failure;

/**
 * sbl_test_core_intr() - test core interrupt access to the serdes
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 *
 * This function is used to test the serdes operations
 * and check its results
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_core_intr(struct sbl_inst *sbl, int port_num)
{
	int err;
	u16 result = 0;
	unsigned int flags = SBL_FLAG_DELAY_5US|SBL_FLAG_INTERVAL_1MS;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_pml_serdes_op(sbl,
			port_num, /* port num */
			0,        /* serdes_sel */
			0,        /* op */
			0,        /* data */
			&result,
			100,      /* timeout ms */
			flags);   /* flags */
	if (err)
		dev_info(sbl->dev, "op 0, err %d\n", err);
	else
		dev_info(sbl->dev, "op 0, result 0x%x\n", result);

	return err;
}
EXPORT_SYMBOL(sbl_test_core_intr);


static void sbl_blattr_init(struct sbl_base_link_attr *blattr,
			    int loopback_mode)
{
	/* simple PEC, fabric link config */
	memset(blattr, 0, sizeof(struct sbl_base_link_attr));

	blattr->magic                = SBL_LINK_ATTR_MAGIC;

	blattr->options              = SBL_OPT_FABRIC_LINK |
				       SBL_OPT_DFE_SAVE_PARAMS |
				       SBL_OPT_USE_SAVED_PARAMS |
				       SBL_OPT_ENABLE_PCAL;

	blattr->start_timeout        = SBL_LINK_START_TIMEOUT_PEC;

	blattr->config_target        = SBL_BASE_LINK_CONFIG_PEC;
	blattr->pec.an_mode          = SBL_AN_MODE_OFF;
	blattr->pec.an_retry_timeout = SBL_LINK_DFLT_AN_RETRY_TIMEOUT;
	blattr->pec.an_max_retry     = SBL_LINK_DFLT_AN_MAX_RETRY;

	blattr->lpd_timeout          = SBL_DFLT_LPD_TIMEOUT;        /* not enabled in options */
	blattr->lpd_poll_interval    = SBL_DFLT_LPD_POLL_INTERVAL;  /* not enabled in options */

	blattr->link_mode            = SBL_LINK_MODE_BS_200G;
	blattr->loopback_mode        = loopback_mode;
	blattr->link_partner         = SBL_LINK_PARTNER_SWITCH;
	blattr->tuning_pattern       = SBL_TUNING_PATTERN_CORE;

	blattr->dfe_pre_delay        = SBL_DFLT_DFE_PRE_DELAY_PEC;
	blattr->dfe_timeout          = SBL_DFLT_DFE_TIMEOUT_PEC;
	blattr->dfe_poll_interval    = SBL_DFLT_DFE_POLL_INTERVAL;

	blattr->nrz_min_eye_height   = SBL_DFLT_NRZ_PEC_MIN_EYE_HEIGHT;
	blattr->nrz_max_eye_height   = SBL_DFLT_NRZ_PEC_MAX_EYE_HEIGHT;
	blattr->pam4_min_eye_height  = SBL_DFLT_PAM4_PEC_MIN_EYE_HEIGHT;
	blattr->pam4_max_eye_height  = SBL_DFLT_PAM4_PEC_MAX_EYE_HEIGHT;

	blattr->fec_mode             = SBL_RS_MODE_ON;
	blattr->enable_autodegrade   = 0;
	blattr->llr_mode             = SBL_LLR_MODE_ON;
	blattr->ifg_config           = SBL_IFG_CONFIG_HPC;
}

/**
 * sbl_test_link_up() - test link up
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @loopback_mode: selected enum value of loopback mode
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_link_up(struct sbl_inst *sbl, int port_num, int loopback_mode)
{
	struct sbl_media_attr mattr;
	struct sbl_base_link_attr blattr;
	int blstate;
	int blerr;
	int sstate;
	int serr;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	/* configure media */
	mattr.magic = SBL_MEDIA_ATTR_MAGIC;
	mattr.media = SBL_LINK_MEDIA_ELECTRICAL;
	mattr.len   = 2;

	err = sbl_media_config(sbl, port_num, &mattr);
	if (err) {
		dev_err(sbl->dev, "%d: media config failed [%d]\n", port_num, err);
		goto out;
	}

	/* configure base-link */
	sbl_blattr_init(&blattr, loopback_mode);

	err = sbl_base_link_config(sbl, port_num, &blattr);
	if (err) {
		dev_err(sbl->dev, "%d: base link config failed [%d]\n", port_num, err);
		goto out;
	}

	/* start the base link */
	err = sbl_base_link_start(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: base link start failed [%d]\n", port_num, err);

out:
	sbl_base_link_get_status(sbl, port_num, &blstate, &blerr, &sstate, &serr, NULL, NULL);
	dev_info(sbl->dev, "%d: base link %s (%d), serdes %s (%d)\n", port_num,
			sbl_link_state_str(blstate), blerr,
			sbl_serdes_state_str(sstate), serr);

	return err;
}
EXPORT_SYMBOL(sbl_test_link_up);

/**
 * sbl_test_link_reup() - Test to link reup
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 * @loopback_mode: selected enum value of loopback mode
 *
 * This function is used to test base link
 * operation of start/stop.
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_link_reup(struct sbl_inst *sbl, int port_num, int loopback_mode)
{
	struct sbl_base_link_attr blattr;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_blattr_init(&blattr, loopback_mode);

	err = sbl_base_link_config(sbl, port_num, &blattr);
	if (err)
		dev_err(sbl->dev, "%d: base link config failed [%d]\n", port_num, err);

	err = sbl_base_link_start(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: base link start failed [%d]\n", port_num, err);

	err = sbl_base_link_stop(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: base link stop failed [%d]\n", port_num, err);

	err = sbl_base_link_start(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: base link start failed [%d]\n", port_num, err);

	return err;
}
EXPORT_SYMBOL(sbl_test_link_reup);

/**
 * sbl_test_link_down() - Test to link down
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_link_down(struct sbl_inst *sbl, int port_num)
{
	int blstate;
	int blerr;
	int sstate;
	int serr;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	err = sbl_base_link_stop(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: base link stop failed [%d]\n", port_num, err);

	sbl_base_link_get_status(sbl, port_num, &blstate, &blerr, &sstate, &serr, NULL, NULL);
	dev_info(sbl->dev, "%d: base link %s (%d), serdes %s (%d)\n", port_num,
			sbl_link_state_str(blstate), blerr,
			sbl_serdes_state_str(sstate), serr);

	return err;
}
EXPORT_SYMBOL(sbl_test_link_down);

/**
 * sbl_test_link_reset() - Test to reset link
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_link_reset(struct sbl_inst *sbl, int port_num)
{
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	err = sbl_base_link_reset(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: base link reset failed [%d]\n", port_num, err);

	return err;
}
EXPORT_SYMBOL(sbl_test_link_reset);

/**
 * sbl_test_serdes_stop() - Test to stop serdes
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 *
 * Stopping the serdes provides a way to test link error pathways
 * can be used to simulate link failure
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_serdes_stop(struct sbl_inst *sbl, int port_num)
{
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	err = sbl_spico_reset(sbl, port_num);
	if (err)
		dev_err(sbl->dev, "%d: sbl serdes stop failed [%d]\n", port_num, err);

	return err;

}
EXPORT_SYMBOL(sbl_test_serdes_stop);

/**
 * sbl_test_pcs_tx_rf() - set pcs to transmit remote fault
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 *
 * Return: 0 on success, error on failure
 */
int sbl_test_pcs_tx_rf(struct sbl_inst *sbl, int port_num)
{
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s starting\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_pml_pcs_set_tx_rf(sbl, port_num);

	return 0;

}
EXPORT_SYMBOL(sbl_test_pcs_tx_rf);

/**
 * sbl_test_scratch() - Test to reload serdes firmware
 * @sbl: A slingshot base link device instance
 * @port_num: port number
 *
 * Return: 0 on success,  error on failure
 */
int sbl_test_scratch(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link;
	int err = 0;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	dev_dbg(sbl->dev, "%s toggling reload serdes fw option\n", __func__);

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	link = sbl->link + port_num;

	if (link->blattr.options & SBL_OPT_RELOAD_FW_ON_TIMEOUT)
		link->blattr.options &= ~SBL_OPT_RELOAD_FW_ON_TIMEOUT;
	else
		link->blattr.options |= SBL_OPT_RELOAD_FW_ON_TIMEOUT;

	return err;
}
EXPORT_SYMBOL(sbl_test_scratch);


/* Updates return value of SerDes FW CRC check.
 * If sbl_test_crc_failure set by sbl_test_inject_serdes_fw_crc_failure is 1,
 * CRC returns failure.
 */
void sbl_test_manipulate_serdes_fw_crc_result(u16 *crc_result)
{
	if (sbl_test_crc_failure)
		*crc_result = SPICO_RESULT_SERDES_CRC_FAIL;
}

/**
 * sbl_test_inject_serdes_fw_crc_failure() - Test to inject crc failure for serdes fw
 * @set: if set then inject crc
 */
void sbl_test_inject_serdes_fw_crc_failure(bool set)
{
	sbl_test_crc_failure = set;
}
EXPORT_SYMBOL(sbl_test_inject_serdes_fw_crc_failure);
