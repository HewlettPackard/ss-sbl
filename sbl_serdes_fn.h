/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2019-2022 Hewlett Packard Enterprise Development LP */
#ifndef SBL_SERDES_FN_H
#define SBL_SERDES_FN_H

#include "sbl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get SBM firmware version from the given SBUS ring.
 *
 * @param sbl[in] Slingshot base link instance to get the serdes firmware version for.
 * @param sbus_ring[in] SBUS ring to get the SBUS Master Firmware for.
 * @param fw_rev[out] serdes firmware revision.
 * @param fw_build[out] serdes firmware build.
 */
void sbl_sbm_get_fw_vers(struct sbl_inst *sbl, int sbus_ring, uint *fw_rev,
		uint *fw_build);

/**
 * @brief Get serdes firmware version for the specified port and serdes lane.
 *
 * @param sbl[in] Slingshot base link instance to get the serdes firmware version for.
 * @param port_num[in] port to get the serdes firmware version for.
 * @param serdes[in] serdes lane within the port to get the firmware version for.
 * @param fw_rev[out] serdes firmware revision.
 * @param fw_build[out] serdes firmware build.
 */
void sbl_serdes_get_fw_vers(struct sbl_inst *sbl, int port_num, int serdes,
	uint *fw_rev, uint *fw_build);

/**
 * @brief Sbus Master firmware flash - all rings
 *
 * @param sbl sbl_inst pointer containing target config and state
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_sbm_firmware_flash(struct sbl_inst *sbl);


/**
 * @brief Sbus Master firmware flash
 *
 * Flashes all rings from first_ring through last_ring, inclusive
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param first_ring first sbus ring to flash
 * @param last_ring last sbus ring to flash
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_sbm_firmware_flash_ring(struct sbl_inst *sbl, int first_ring,
				int last_ring, bool force);

/**
 * @brief SerDes firmware flash
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param force reset and flash even if current fw is at the correct
 *        rev and build number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_firmware_flash(struct sbl_inst *sbl, int port_num, bool force);

/**
 * @brief SerDes firmware flash with auto-recovery attempt
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param force reset and flash even if current fw is at the correct
 *        rev and build number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_firmware_flash_safe(struct sbl_inst *sbl, int port_num,
				   bool force);

/**
 * @brief Check if CRC for target serdes lane is valid
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 *
 * @return 0 on success, -1 on failure
 */
int sbl_validate_serdes_fw_crc(struct sbl_inst *sbl, int port_num, int serdes);

/**
 * @brief Check if SerDes desired_rev matches flashed version
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param fw_rev desired firmware revision - flash if not found
 * @param fw_build desired firmware build - flash if not found
 *
 * @return 0 on success, -1 on failure
 */
int sbl_validate_serdes_fw_vers(struct sbl_inst *sbl, int port_num, int serdes,
				int fw_rev, int fw_build);

/**
 * @brief Check if SerDes desired_rev matches flashed version
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param fw_rev desired firmware revision - flash if not found
 * @param fw_build desired firmware build - flash if not found
 *
 * @return 0 on success, -1 on failure
 */
int sbl_validate_sbm_fw_vers(struct sbl_inst *sbl, u32 sbus_ring,
			     int fw_rev, int fw_build);

/**
 * @brief Upload target firmware image to single location
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param fw_size byte size of firmware image
 * @param fw_data byte array of size fw_size
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_firm_upload(struct sbl_inst *sbl, int port_num, size_t fw_size,
			   const u8 *fw_data);


/**
 * @brief Upload target firmware image to multiple locations
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param sbus_ring target sbus ring
 * @param fw_size byte size of firmware image
 * @param fw_data byte array of size fw_size
 *
 * @return 0 on success, -1 on failure
 */
int sbl_sbm_firm_upload(struct sbl_inst *sbl, int sbus_ring,
			size_t fw_size, const u8 *fw_data);

/**
 * @brief Ensures most tuning parameters for a given port are within range.
 *
 * This function allows for at most SBL_MAX_OOB_SERDES_PARAMS parameters to
 *  be outside of the configured range and still pass the check.
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 if all lanes pass the check, negative errno on failure
 *	 -EDQUOT: Too many parameters outside valid window
 */
int sbl_check_serdes_tuning_params(struct sbl_inst *sbl, int port_num);

/**
 * @brief Save a given SerDes' tuning params
 *
 * Since the AAPL code does not provide this type of convenience function,
 *  we piece it together here.
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success, -1 on failure
 */
int sbl_save_serdes_tuning_params(struct sbl_inst *sbl, int port_num);

/**
 * @brief Set a given SerDes tuning params
 *
 * Since the AAPL code does not provide this type of convenience function,
 *  we piece it together here.
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 *
 * @return 0 on success, -1 on failure
 */
int sbl_apply_serdes_tuning_params(struct sbl_inst *sbl, int port, int serdes);

/**
 * @brief Perfrom dfe tune on a particular serdes
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param is_retune true if reapplying saved tuning params
 *                   This causes a minimal-effort DFE tune
 *
 * @return 0 on success, negative errno on failure
 */
int sbl_serdes_dfe_tune_start(struct sbl_inst *sbl, int port_num, int serdes,
			      bool is_retune);

/**
 * @brief Wait for a particular serdes to complete its DFE tune
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on dfe complete, negative errno on failure
 *	 -EIO: interrupt failed
 *	 -ENOMSG: loss of signal during DFE
 *	 -ETIMEDOUT: DFE still in progress after retry_limit exceeded
 */
int sbl_serdes_dfe_tune_wait(struct sbl_inst *sbl, int port_num);

/**
 * @brief Perfrom dfe tune on a port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param is_retune true if reapplying saved tuning params
 *                   This causes a minimal-effort DFE tune
 *
 * @return 0 on success, negative errno on failure
 */
int sbl_port_dfe_tune_start(struct sbl_inst *sbl, int port_num, bool is_retune);

/**
 * @brief Wait for a port to complete its DFE tune
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes_mask bitmask of serdes lanes to wait for
 * @param tune_mask bitmask returned with bits set for successful tunes
 *
 * @return 0 on dfe complete, negative errno on failure
 *	 -EIO: interrupt failed
 *	 -ENOMSG: loss of signal during DFE
 *	 -ETIMEDOUT: DFE still in progress after retry_limit exceeded
 */
int sbl_port_dfe_tune_wait(struct sbl_inst *sbl, int port_num);

/**
 * @brief Read and validate eye heights for a port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 if all eyes pass the check, negative errno on failure
 *	 -EIO: Failed to read eye heights
 *	 -ECHRNG: Eye height failed - too low
 *	 -ENOMSG: Eye height failed - too high
 */
int sbl_port_check_eyes(struct sbl_inst *sbl, int port_num);


/**
 * @brief Return maps of serdes status for a port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param active_map returned map of serdes in use for the mode
 * @param not_idle_map returned map of serdes not reporting electrical idle
 * @param ok_eye_map returned map of serdes with all eyes good
 *
 * @return 0 if all maps are valid, negative errno on failure
 *	 -EIO: Failed to read eye heights
 *	 -ENODATA: bad link mode configured
 *	 -EUCLEAN: not configured
 */
int sbl_port_get_serdes_state_maps(struct sbl_inst *sbl, int port_num,
		u8 *active_map, u8 *not_idle_map, u8 *ok_eye_map);

/**
 * @brief Updates the state in the sbl struct from the tgt values
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return none
 */
void sbl_update_internal_state(struct sbl_inst *sbl, int port_num);

/**
 * @brief DFE tune a port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param is_retune true if reapplying saved tuning params
 *                   This causes a minimal-effort DFE tune
 *
 * @return 0 on dfe complete, negative errno on failure
 */
int sbl_port_dfe_tune(struct sbl_inst *sbl, int port_num, bool is_retune);

/**
 * @brief Get the 6 eye heights
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param eye_heights pointer to int[6] array
 *
 * @return 0 on success, -1 on failure
 */
int sbl_get_eye_heights(struct sbl_inst *sbl, int port_num, int serdes,
			int *eye_heights);

/**
 * @brief Issues a soft SBus reset
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_soft_reset(struct sbl_inst *sbl, int port_num, int serdes);

/**
 * @brief Runs SerDes initization
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param encoding NRZ or PAM4
 * @param divisor With width, describes target speed
 * @param width With divisor, describes target speed
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_init(struct sbl_inst *sbl, int port_num, int serdes,
		    int encoding, int divisor, int width);

/**
 * @brief Applies tx/rx polarity inversion and other config
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param encoding NRZ or PAM4
 * @param an indicates swizzle and graycode should be disabled
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_polarity_ctrl(struct sbl_inst *sbl, int port_num, int serdes,
			     int encoding, bool an);

/**
 * @brief Enables/Disabled SerDes Tx/Rx
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param tx_en New Tx state
 * @param rx_en New Rx state
 * @param txo_en New Tx output state
 *
 * @return 0 on success, -1 on failure
 */
int sbl_set_tx_rx_enable(struct sbl_inst *sbl, int port_num, int serdes,
			 bool tx_en, bool rx_en, bool txo_en);

/**
 * @brief Sets the CTLE parameters
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param gs1 value for gainshape1
 * @param gs2 value for gainshape2
 *
 * @return 0 on success, -1 on failure
 */
int sbl_set_gs(struct sbl_inst *sbl, int port_num, int serdes, int gs1,
	       int gs2);

/**
 * @brief Sets the TX data select
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param data_sel ROSETTA_TX_DS_CORE or ROSETTA_TX_DS_PRBS
 *
 * @return 0 on success, -1 on failure
 */
int sbl_set_tx_data_sel(struct sbl_inst *sbl, int port_num, int serdes,
			int data_sel);

/**
 * @brief Sets the TX data select
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 * @param atten value for atten
 * @param pre value for pre
 * @param post value for post
 * @param pre2 value for pre2
 * @param pre3 value for pre3
 *
 * @return 0 on success, -1 on failure
 */
int sbl_set_tx_eq(struct sbl_inst *sbl, int port_num, int serdes,
		  int atten, int pre, int post, int pre2, int pre3);

/**
 * @brief Sets the Rx compare mode and data, qualification, and performs an
 *	error reset
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param serdes target serdes lane
 *
 * @return 0 on success, -1 on failure
 */
int sbl_set_prbs_rx_mode(struct sbl_inst *sbl, int port_num, int serdes);

/**
 * @brief Set SBM clock divider
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @divider value for divider
 *
 * @return 0 on success, -1 on failure
 */
int sbl_apply_sbus_divider(struct sbl_inst *sbl, int value);

/**
 * @brief Begin continuous adaptive tuning on all enabled lanes for this port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_port_start_pcal(struct sbl_inst *sbl, int port_num);

/**
 * @brief Stop continuous adaptive tuning on all enabled lanes for this port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_port_stop_pcal(struct sbl_inst *sbl, int port_num);

/**
 * @brief Setup to perform a SerDes mini-tune
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_minitune_setup(struct sbl_inst *sbl, int port_num);

/**
 * @brief Block until a SerDes mini-tune completes, fails or times out
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_minitune_block(struct sbl_inst *sbl, int port_num);

/**
 * @brief Configure the SerDes lanes for a given port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param allow_an target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_config(struct sbl_inst *sbl, int port_num, bool allow_an);

/**
 * @brief Tune the SerDes lanes for a given port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_tuning(struct sbl_inst *sbl, int port_num);

/**
 * @brief Reset SPICO micro
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_spico_reset(struct sbl_inst *sbl, int port_num);

/**
 * @brief Reset the PLLs on a serdes
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_reset_serdes_plls(struct sbl_inst *sbl, int port_num);

/**
 * @brief Log port eye heights
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 */
void sbl_log_port_eye_heights(struct sbl_inst *sbl, int port_num);

#ifdef __cplusplus
}
#endif
#endif // SBL_SERDES_FN_H
