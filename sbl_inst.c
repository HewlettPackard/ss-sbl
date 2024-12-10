// SPDX-License-Identifier: GPL-2.0

/*
 *
 * Copyright 2019-2024 Hewlett Packard Enterprise Development LP
 *
 * Slingshot link manager driver for the first generation adapter
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>

#include "sbl_kconfig.h"
#include "sbl.h"
#include "uapi/sbl_iface_constants.h"
#include "sbl_constants.h"
#include "sbl_serdes.h"
#include "sbl_config_list.h"
#include "sbl_serdes_map.h"
#include "sbl_pml.h"
#include "sbl_internal.h"
#ifdef CONFIG_SBL_PLATFORM_CAS
#include "uapi/sbl_cassini.h"
#endif
#include "sbl_fec.h"

static atomic_t sbl_inst_id = ATOMIC_INIT(-1);
#ifndef CONFIG_SBL_PLATFORM_ROS_HW
struct sbl_switch_info cas_brazos_switch_info = SBL_CASSINI_BRAZOS_SW_INFO_INITIALIZER;
struct sbl_switch_info cas_nic0_switch_info = SBL_CASSINI_NIC0_SW_INFO_INITIALIZER;
struct sbl_switch_info cas_nic1_switch_info = SBL_CASSINI_NIC1_SW_INFO_INITIALIZER;
#endif
static struct sbl_link *sbl_create_link_db(struct sbl_switch_info *switch_info);
static int sbl_setup_ops(struct sbl_inst *sbl, const struct sbl_ops *ops);
static int sbl_setup_serdes_configs(struct sbl_inst *sbl);
static int sbl_fec_init(struct sbl_inst *sbl);

void sbl_get_version(int *major, int *minor, int *inc)
{
	if (major)
		*major = SBL_VERSION_MAJOR;
	if (minor)
		*minor = SBL_VERSION_MINOR;
	if (inc)
		*inc = SBL_VERSION_INC;
}
EXPORT_SYMBOL(sbl_get_version);

int sbl_get_num_sbus_rings(struct sbl_inst *sbl)
{
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	return sbl->switch_info->num_sbus_rings;
}
EXPORT_SYMBOL(sbl_get_num_sbus_rings);

struct sbl_inst *sbl_new_instance(void *accessor, void *pci_accessor,
				  const struct sbl_ops *ops,
				  struct sbl_init_attr *init_attr)
{
	struct sbl_inst *sbl;
	struct device *dev = sbl_get_device();
	int err;
	int i;

	if (!accessor || !pci_accessor || !ops || !init_attr) {
		err = -EINVAL;
		goto out;
	}
	if (init_attr->magic != SBL_INIT_ATTR_MAGIC) {
		err = -EINVAL;
		goto out;
	}

	sbl = kzalloc(sizeof(struct sbl_inst), GFP_KERNEL);
	if (!sbl) {
		err = -ENOMEM;
		goto out;
	}

	sbl->magic = SBL_MAGIC;
	sbl->id = atomic_inc_return(&sbl_inst_id);
	sbl->dev = dev;
	sbl->accessor = accessor;
	sbl->pci_accessor = pci_accessor;
	sbl->sbus_op_flags = SBL_DFLT_SBUS_OP_FLAGS_SLOW;

	sbl->workq = alloc_workqueue("%s", WQ_MEM_RECLAIM | WQ_UNBOUND, 0, "sbl-fec");

	/* Initialize the hardware-specific map */
#ifdef CONFIG_SBL_PLATFORM_ROS_HW
	sbl->switch_info = sbl_get_switch_info(NULL);
	if (sbl->switch_info == NULL) {
		sbl_dev_err(sbl->dev, "Unable to get sbl_switch_info\n");
		err = -ENOMSG;
		goto out_free;
	}
#else
	switch (init_attr->uc_platform) {
	case SBL_UC_PLATFORM_SAWTOOTH:
		if (init_attr->uc_nic == 0) {
			sbl->switch_info = &cas_nic0_switch_info;
		} else if (init_attr->uc_nic == 1) {
			sbl->switch_info = &cas_nic1_switch_info;
		} else {
			sbl_dev_err(sbl->dev, "Bad NIC index (%d)!\n",
				init_attr->uc_nic);
			err = -EINVAL;
			goto out_free;
		}
		break;
	case SBL_UC_PLATFORM_BRAZOS:
		sbl->switch_info = &cas_brazos_switch_info;
		break;
	case SBL_UC_PLATFORM_UNDEFINED:
		sbl_dev_err(sbl->dev, "Undefined uC platform!\n");
		err = -EINVAL;
		goto out_free;
	default:
		sbl_dev_err(sbl->dev, "Unknown uC platform (%d)!\n", init_attr->uc_platform);
		err = -EINVAL;
		goto out_free;
	}

#endif
	sbl_dev_info(dev, "new instance (%d): %d ports x %d serdes\n", sbl->id,
		 sbl->switch_info->num_ports, sbl->switch_info->num_serdes);

	sbl->sbus_ring_mtx =
		kcalloc(sbl->switch_info->num_sbus_rings, sizeof(struct mutex), GFP_KERNEL);
	if (!sbl->sbus_ring_mtx) {
		err = -ENOMEM;
		goto out_free;
	}

	sbl->sbm_fw_mtx =
		kzalloc(sbl->switch_info->num_sbus_rings*sizeof(struct mutex), GFP_KERNEL);
	if (!sbl->sbm_fw_mtx) {
		err = -ENOMEM;
		goto out_free_sbus_mtx;
	}

	sbl->reload_sbm_fw =
		kzalloc(sbl->switch_info->num_sbus_rings*sizeof(bool), GFP_KERNEL);
	if (!sbl->reload_sbm_fw) {
		err = -ENOMEM;
		goto out_free_sbm_fw_mtx;
	}

	sbl->sbm_fw_reload_count =
		kzalloc(sbl->switch_info->num_sbus_rings*sizeof(atomic_t), GFP_KERNEL);
	if (!sbl->sbm_fw_reload_count) {
		err = -ENOMEM;
		goto out_free_reload_sbm;
	}

	for (i = 0; i < sbl->switch_info->num_sbus_rings; ++i) {
		mutex_init(&sbl->sbus_ring_mtx[i]);
		mutex_init(&sbl->sbm_fw_mtx[i]);
		sbl->reload_sbm_fw[i] = false;
		atomic_set(&sbl->sbm_fw_reload_count[i], 0);
	}

	/* setup the op table */
	err = sbl_setup_ops(sbl, ops);
	if (err) {
		sbl_dev_err(sbl->dev, "op table setup failed [%d]\n", err);
		goto out_free_sbm_fw_reload_count;
	}

	/* setup serdes lock, configuration list and add default */
	err = sbl_setup_serdes_configs(sbl);
	if (err) {
		sbl_dev_err(sbl->dev, "serdes setup failed [%d]\n", err);
		goto out_free_sbm_fw_reload_count;
	}

	/* create link database */
	sbl->link = sbl_create_link_db(sbl->switch_info);
	if (IS_ERR(sbl->link)) {
		err = PTR_ERR(sbl->link);
		sbl_dev_err(sbl->dev, "link db creation failed [%d]\n", err);
		goto out_free_configs;
	}

	sbl_fec_init(sbl);

	for (i = 0; i < sbl->switch_info->num_ports; ++i) {
		/* ensure no valid saved tuning params */
		sbl_serdes_invalidate_tuning_params(sbl, i);

		/* set default values for PML */
		sbl_pml_set_defaults(sbl, i);
	}

	return sbl;

out_free_configs:
	sbl_serdes_clear_all_configs(sbl, true /* clear default */);
out_free_sbm_fw_reload_count:
	kfree(sbl->sbm_fw_reload_count);
out_free_reload_sbm:
	kfree(sbl->reload_sbm_fw);
out_free_sbm_fw_mtx:
	kfree(sbl->sbm_fw_mtx);
out_free_sbus_mtx:
	kfree(sbl->sbus_ring_mtx);
out_free:
	kfree(sbl);
out:
	return ERR_PTR(err);
}
EXPORT_SYMBOL(sbl_new_instance);

static int sbl_fec_init(struct sbl_inst *sbl)
{
	int i;
	struct sbl_link *link;

	for (i = 0; i < CONFIG_SBL_NUM_PORTS; ++i) {
		link = sbl->link + i;

		link->fec_data = kzalloc(sizeof(struct fec_data), GFP_KERNEL);

		if (!link->fec_data)
			return -ENOMEM;

		link->fec_data->fec_prmts = kzalloc(sizeof(struct sbl_fec), GFP_KERNEL);

		if (!link->fec_data->fec_prmts)
			return -ENOMEM;

		link->fec_data->fec_prmts->fec_curr_cnts = &link->fec_data->fec_prmts->fec_cntrs[0];
		link->fec_data->fec_prmts->fec_prev_cnts = &link->fec_data->fec_prmts->fec_cntrs[1];
		link->fec_data->fec_prmts->fec_rates = kzalloc(sizeof(struct sbl_pcs_fec_cntrs), GFP_KERNEL);
		spin_lock_init(&link->fec_data->fec_prmts->fec_cnt_lock);

		link->fec_data->fec_prmts->fec_ucw_thresh = 0;
		link->fec_data->fec_prmts->fec_ucw_up_thresh_adj = 100;
		link->fec_data->fec_prmts->fec_ucw_down_thresh_adj = 100;
		link->fec_data->fec_prmts->fec_ucw_hwm = 0;
		link->fec_data->fec_prmts->fec_ccw_thresh = 0;
		link->fec_data->fec_prmts->fec_ccw_up_thresh_adj = 100;
		link->fec_data->fec_prmts->fec_ccw_down_thresh_adj = 100;
		link->fec_data->fec_prmts->fec_stp_ccw_thresh = 0;
		link->fec_data->fec_prmts->fec_stp_ccw_up_thresh_adj = 100;
		link->fec_data->fec_prmts->fec_ccw_hwm = 0;
		link->fec_data->fec_prmts->fecl_warn = 0;

		spin_lock_init(&link->fec_data->fec_prmts->fec_cw_lock);
		timer_setup(&link->fec_data->fec_timer, sbl_fec_timer, 0);
		link->fec_data->sbl		= sbl;
		link->fec_data->port_num = i;

		INIT_WORK(&link->fec_data->fec_timer_work, sbl_fec_timer_work);
	}

	return 0;
}

#ifndef CONFIG_SBL_PLATFORM_ROS_HW
void sbl_set_eth_name(struct sbl_inst *sbl, const char *name)
{
	if (!sbl || !name)
		return;

	strscpy(sbl->iattr.eth_if_name, name, sizeof(sbl->iattr.eth_if_name));

	sbl_dev_info(sbl->dev, "%s eth if name changed to %s", sbl->iattr.inst_name, sbl->iattr.eth_if_name);
}
EXPORT_SYMBOL(sbl_set_eth_name);
#endif


int sbl_delete_instance(struct sbl_inst *sbl)
{
	struct sbl_link *link;
	int err;
	int i;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	for (i = 0; i < sbl->switch_info->num_ports; ++i) {
		link = sbl->link + i;
		sbl_link_counters_term(link);
		if (link->pml_recovery.started)
			sbl_pml_recovery_cancel(sbl, i);
	}
	for (i = 0; i < CONFIG_SBL_NUM_PORTS; ++i) {
		link = sbl->link + i;

		kfree(link->fec_data->fec_prmts->fec_rates);
		link->fec_data->fec_prmts->fec_rates = NULL;
		kfree(link->fec_data->fec_prmts);
		link->fec_data->fec_prmts = NULL;
		cancel_work_sync(&link->fec_data->fec_timer_work);
		kfree(link->fec_data);
		link->fec_data = NULL;
	}

	kfree(sbl->link);
	sbl_serdes_clear_all_configs(sbl, true /* clear default */);
	kfree(sbl->sbm_fw_reload_count);
	kfree(sbl->reload_sbm_fw);
	kfree(sbl->sbm_fw_mtx);
	kfree(sbl->sbus_ring_mtx);
	flush_workqueue(sbl->workq);
	destroy_workqueue(sbl->workq);
	sbl->magic = 0;   /* paranoia */
	kfree(sbl);

	return 0;
}
EXPORT_SYMBOL(sbl_delete_instance);


/*
 * All operations must be supplied or SBL cannot function
 */
#define SBL_SETUP_OP_TBL_ENTRY(e)  \
	if (ops->e) \
		sbl->ops.e = ops->e; \
	else { \
		sbl_dev_err(sbl->dev, "missing ops tbl entry " #e); \
		error = true; \
	}

static int sbl_setup_ops(struct sbl_inst *sbl, const struct sbl_ops *ops)
{
	bool error = false;

	if (!ops) {
		sbl_dev_err(sbl->dev, "missing ops table\n");
		return -EINVAL;
	}

	SBL_SETUP_OP_TBL_ENTRY(sbl_read32);
	SBL_SETUP_OP_TBL_ENTRY(sbl_read64);
	SBL_SETUP_OP_TBL_ENTRY(sbl_write32);
	SBL_SETUP_OP_TBL_ENTRY(sbl_write64);
	SBL_SETUP_OP_TBL_ENTRY(sbl_sbus_op);
	SBL_SETUP_OP_TBL_ENTRY(sbl_sbus_op_reset);
	SBL_SETUP_OP_TBL_ENTRY(sbl_is_fabric_link);
	SBL_SETUP_OP_TBL_ENTRY(sbl_get_max_frame_size);
	SBL_SETUP_OP_TBL_ENTRY(sbl_pml_install_intr_handler);
	SBL_SETUP_OP_TBL_ENTRY(sbl_pml_enable_intr_handler);
	SBL_SETUP_OP_TBL_ENTRY(sbl_pml_disable_intr_handler);
	SBL_SETUP_OP_TBL_ENTRY(sbl_pml_remove_intr_handler);
	SBL_SETUP_OP_TBL_ENTRY(sbl_async_alert);

	if (error)
		return -ENOENT;
	else
		return 0;
}


static struct sbl_link *sbl_create_link_db(struct sbl_switch_info *switch_info)
{
	int i;
	int j;
	int err;
	struct sbl_link *link;

	link = kcalloc(switch_info->num_ports, sizeof(struct sbl_link), GFP_KERNEL);
	if (!link)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < switch_info->num_ports; ++i) {
		err = sbl_link_counters_init(link + i);
		if (err)
			goto out_free_sbl_link_counters;

		link[i].num = i;
		link[i].mconfigured = false;
		link[i].blconfigured = false;
		atomic_set(&link[i].debug_config, 0);
		/* TODO: change blattr items to INVALID */
		link[i].blattr.pec.an_mode   = SBL_AN_MODE_OFF;
		link[i].blattr.link_mode     = SBL_LINK_MODE_BS_200G;
		link[i].blattr.loopback_mode = SBL_LOOPBACK_MODE_OFF;
		link[i].blattr.llr_mode      = SBL_LLR_MODE_OFF;
		link[i].blattr.ifg_config    = SBL_IFG_CONFIG_HPC;
		link[i].sstate = SBL_SERDES_STATUS_DOWN;
		link[i].serr = 0;
		link[i].blstate = SBL_BASE_LINK_STATUS_UNCONFIGURED;
		link[i].blerr = 0;
		link[i].link_info = 0;
		link[i].link_mode     = SBL_LINK_MODE_INVALID;
		link[i].ifg_config    = SBL_IFG_CONFIG_INVALID;
		link[i].loopback_mode = SBL_LOOPBACK_MODE_INVALID;
		link[i].llr_mode      = SBL_LLR_MODE_INVALID;
		link[i].llr_loop_time = 0;
		link[i].pcs_config = false;
		link[i].intr_err_flgs = 0ULL;
		link[i].an_rx_count = 0;
		link[i].an_tx_count = 0;
		link[i].an_timeout_active = false;
		link[i].lp_subtype = SBL_LP_SUBTYPE_INVALID;
		link[i].tuning_params.tp_state_hash0 = 0;
		link[i].tuning_params.tp_state_hash1 = 0;
		link[i].last_start_jiffy = jiffies;
		link[i].start_cancelled = false;
		link[i].dfe_tune_count = 0;
		link[i].optical_delay_active = false;
		link[i].dfe_predelay_active = false;
		link[i].pcal_running = false;
		link[i].tune_param_oob_count = 0;
		link[i].reload_serdes_fw = false;
		link[i].pcs_recovery_flag = false;
		link[i].pml_recovery.started = false;
		link[i].pml_recovery.rl_window_start = 0;
		link[i].fec_discard_time = 0;
		link[i].fec_discard_type = SBL_FEC_DISCARD_TYPE_INVALID;

		spin_lock_init(&link[i].lock);
		spin_lock_init(&link[i].timeout_lock);
		spin_lock_init(&link[i].pcs_recovery_lock);
		spin_lock_init(&link[i].is_degraded_lock);
		spin_lock_init(&link[i].fec_discard_lock);
		mutex_init(&link[i].busy_mtx);
		mutex_init(&link[i].serdes_mtx);
		mutex_init(&link[i].tuning_params_mtx);
	}

	return link;

out_free_sbl_link_counters:
	for (j = 0; j < i; ++j)
		sbl_link_counters_term(link + j);
	kfree(link);
	return ERR_PTR(-ENOMEM);
}


static int sbl_setup_serdes_configs(struct sbl_inst *sbl)
{
	struct sbl_serdes_config default_config;

	spin_lock_init(&sbl->serdes_config_lock);
	INIT_LIST_HEAD(&sbl->serdes_config_list);

	default_config = (struct sbl_serdes_config)SBL_SERDES_CONFIG_INITIALIZER;

	return sbl_serdes_add_config(sbl, default_config.tp_state_mask0,
					default_config.tp_state_mask1,
					default_config.tp_state_match0,
					default_config.tp_state_match1,
					default_config.port_mask,
					default_config.serdes_mask,
					&default_config.vals, true);
}


/*
 * initialise the instance
 *
 *   Most initialisation is done during configuration
 */
int sbl_initialise_instance(struct sbl_inst *sbl, struct sbl_instance_attr *attr)
{
	int err;

	if (!attr)
		return -EINVAL;

	if (attr->magic != SBL_INSTANCE_ATTR_MAGIC)
		return -EINVAL;

	memcpy(&sbl->iattr, attr, sizeof(struct sbl_instance_attr));

	/*
	 * load the required firmware
	 *
	 *    force load all ports if serdes firmware validation fails
	 */
	err = sbl_serdes_load(sbl, SBL_ALL_PORTS, false);
	if (err) {
		sbl_dev_err(sbl->dev, "initial serdes fw load failed [%d]", err);
		return err;
	}
	sbl_dev_err(sbl->dev, "serdes fw loaded");

	return 0;

}
EXPORT_SYMBOL(sbl_initialise_instance);


int sbl_restore_instance(struct sbl_inst *sbl)
{
	sbl_dev_err(sbl->dev, "Restoring an instance not supported (yet)\n");
	return -EOPNOTSUPP;
}
EXPORT_SYMBOL(sbl_restore_instance);
