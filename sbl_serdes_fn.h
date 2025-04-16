/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2019-2022, 2025 Hewlett Packard Enterprise Development LP */
#ifndef SBL_SERDES_FN_H
#define SBL_SERDES_FN_H

#include "sbl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get SBM firmware version from the given SBUS ring.
 */
void sbl_sbm_get_fw_vers(struct sbl_inst *sbl, int sbus_ring, uint *fw_rev,
		uint *fw_build);

/*
 * Get serdes firmware version for the specified port and serdes lane.
 */
void sbl_serdes_get_fw_vers(struct sbl_inst *sbl, int port_num, int serdes,
	uint *fw_rev, uint *fw_build);

/*
 * Sbus Master firmware flash - all rings
 */
int sbl_sbm_firmware_flash(struct sbl_inst *sbl);


/*
 * Sbus Master firmware flash
 */
int sbl_sbm_firmware_flash_ring(struct sbl_inst *sbl, int first_ring,
				int last_ring, bool force);

/*
 * SerDes firmware flash
 */
int sbl_serdes_firmware_flash(struct sbl_inst *sbl, int port_num, bool force);

/*
 * SerDes firmware flash with auto-recovery attempt
 */
int sbl_serdes_firmware_flash_safe(struct sbl_inst *sbl, int port_num,
				   bool force);

/*
 * Check if CRC for target serdes lane is valid
 */
int sbl_validate_serdes_fw_crc(struct sbl_inst *sbl, int port_num, int serdes);

/*
 * Check if SerDes desired_rev matches flashed version
 */
int sbl_validate_serdes_fw_vers(struct sbl_inst *sbl, int port_num, int serdes,
				int fw_rev, int fw_build);

/*
 * Check if SerDes desired_rev matches flashed version
 */
int sbl_validate_sbm_fw_vers(struct sbl_inst *sbl, u32 sbus_ring,
			     int fw_rev, int fw_build);

/*
 * Upload target firmware image to single location
 */
int sbl_serdes_firm_upload(struct sbl_inst *sbl, int port_num, size_t fw_size,
			   const u8 *fw_data);


/*
 * Upload target firmware image to multiple locations
 */
int sbl_sbm_firm_upload(struct sbl_inst *sbl, int sbus_ring,
			size_t fw_size, const u8 *fw_data);

/*
 * Ensures most tuning parameters for a given port are within range.
 */
int sbl_check_serdes_tuning_params(struct sbl_inst *sbl, int port_num);

/*
 * Gets a given SerDes' tuning params
 */
int sbl_save_serdes_tuning_params(struct sbl_inst *sbl, int port_num);

/*
 * Set a given SerDes tuning params
 */
int sbl_apply_serdes_tuning_params(struct sbl_inst *sbl, int port, int serdes);

/*
 * Perfrom dfe tune on a particular serdes
 */
int sbl_serdes_dfe_tune_start(struct sbl_inst *sbl, int port_num, int serdes,
			      bool is_retune);

/*
 * Wait for a particular serdes to complete its DFE tune
 */
int sbl_serdes_dfe_tune_wait(struct sbl_inst *sbl, int port_num);

/*
 * Perfrom dfe tune on a port
 */
int sbl_port_dfe_tune_start(struct sbl_inst *sbl, int port_num, bool is_retune);

/*
 * Wait for a port to complete its DFE tune
 */
int sbl_port_dfe_tune_wait(struct sbl_inst *sbl, int port_num);

/*
 * Read and validate eye heights for a port
 */
int sbl_port_check_eyes(struct sbl_inst *sbl, int port_num);


/*
 * Return maps of serdes status for a port
 */
int sbl_port_get_serdes_state_maps(struct sbl_inst *sbl, int port_num,
		u8 *active_map, u8 *not_idle_map, u8 *ok_eye_map);

/*
 * Updates the state in the sbl struct from the tgt values
 */
void sbl_update_internal_state(struct sbl_inst *sbl, int port_num);

/*
 * DFE tune a port
 */
int sbl_port_dfe_tune(struct sbl_inst *sbl, int port_num, bool is_retune);

/*
 * Get the 6 eye heights
 */
int sbl_get_eye_heights(struct sbl_inst *sbl, int port_num, int serdes,
			int *eye_heights);

/*
 * Issues a soft SBus reset
 */
int sbl_serdes_soft_reset(struct sbl_inst *sbl, int port_num, int serdes);

/*
 * Runs SerDes initization
 */
int sbl_serdes_init(struct sbl_inst *sbl, int port_num, int serdes,
		    int encoding, int divisor, int width);

/*
 * Applies tx/rx polarity inversion and other config
 */
int sbl_serdes_polarity_ctrl(struct sbl_inst *sbl, int port_num, int serdes,
			     int encoding, bool an);

/*
 * Enables/Disabled SerDes Tx/Rx
 */
int sbl_set_tx_rx_enable(struct sbl_inst *sbl, int port_num, int serdes,
			 bool tx_en, bool rx_en, bool txo_en);

/*
 * Sets the CTLE parameters
 */
int sbl_set_gs(struct sbl_inst *sbl, int port_num, int serdes, int gs1,
	       int gs2);

/*
 * Sets the TX data select
 */
int sbl_set_tx_data_sel(struct sbl_inst *sbl, int port_num, int serdes,
			int data_sel);

/*
 * Sets the TX data select
 */
int sbl_set_tx_eq(struct sbl_inst *sbl, int port_num, int serdes,
		  int atten, int pre, int post, int pre2, int pre3);

/*
 * Sets the Rx compare mode and data, qualification, and performs an
 *	error reset
 */
int sbl_set_prbs_rx_mode(struct sbl_inst *sbl, int port_num, int serdes);

/*
 * Set SBM clock divider
 */
int sbl_apply_sbus_divider(struct sbl_inst *sbl, int value);

/*
 * Begin continuous adaptive tuning on all enabled lanes for this port
 */
int sbl_port_start_pcal(struct sbl_inst *sbl, int port_num);

/*
 * Stop continuous adaptive tuning on all enabled lanes for this port
 */
int sbl_port_stop_pcal(struct sbl_inst *sbl, int port_num);

/*
 * Setup to perform a SerDes mini-tune
 */
int sbl_serdes_minitune_setup(struct sbl_inst *sbl, int port_num);

/*
 * Block until a SerDes mini-tune completes, fails or times out
 */
int sbl_serdes_minitune_block(struct sbl_inst *sbl, int port_num);

/*
 * Configure the SerDes lanes for a given port
 */
int sbl_serdes_config(struct sbl_inst *sbl, int port_num, bool allow_an);

/*
 * Tune the SerDes lanes for a given port
 */
int sbl_serdes_tuning(struct sbl_inst *sbl, int port_num);

/*
 * Reset SPICO micro
 */
int sbl_spico_reset(struct sbl_inst *sbl, int port_num);

/*
 * Reset the PLLs on a serdes
 */
int sbl_reset_serdes_plls(struct sbl_inst *sbl, int port_num);

/*
 * Log port eye heights
 */
void sbl_log_port_eye_heights(struct sbl_inst *sbl, int port_num);

#ifdef __cplusplus
}
#endif
#endif // SBL_SERDES_FN_H
