/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2020,2022-2023 Hewlett Packard Enterprise Development LP. All rights reserved. */

/*
 * sbl_serdes.h
 *
 *  Exported serdes interface definition
 */
#ifndef _SBL_SERDES_H_
#define _SBL_SERDES_H_


/**
 * @brief SerDes test function
 *
 * @param sbl sbl_inst pointer containing target config and state
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_test(struct sbl_inst *sbl);

/**
 * @brief SerDes initial load function
 *
 *  To be called after the Rosetta power cycle or the controller boots
 *  Applyies the SBus speedup and flashes all SBM/SerDes firmware images
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 * @param force force reset and flash even if current fw is at the correct
 *        rev and build number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_load(struct sbl_inst *sbl, int port_num, bool force);

/**
 * @brief SerDes link partner detection
 *
 * This blocking function will use the Serdes to determine if the
 * link partner is present ant trying to come up.
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_lp_detect(struct sbl_inst *sbl, int port_num);

/**
 * @brief Start the SerDes lanes for a given port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_start(struct sbl_inst *sbl, int port_num);

/**
 * @brief Stops the SerDes lanes for a given port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_stop(struct sbl_inst *sbl, int port_num);


/**
 * @brief Resets all SerDes lanes for a given port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_serdes_reset(struct sbl_inst *sbl, int port_num);

/**
 * @brief Sets up the serdes at the correct speed to perform autonegotiation
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_an_serdes_start(struct sbl_inst *sbl, int port_num);

/**
 * @brief Stops the SerDes lanes for a given port
 *
 * @param sbl sbl_inst pointer containing target config and state
 * @param port_num target port number
 *
 * @return 0 on success or negative errno on failure
 */
int sbl_an_serdes_stop(struct sbl_inst *sbl, int port_num);

#endif /* _SBL_SERDES_H_ */
