/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019,2022-2023 Hewlett Packard Enterprise Development LP. All rights reserved. */

#ifndef SBL_SBM_SERDES_IFACE_H
#define SBL_SBM_SERDES_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __KERNEL__
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif /*  __KERNEL__ */

/**
 * @brief Returns true if the sbus_addr is a CM4 SerDes
 *
 * @param sbus_addr address describing a serdes ring and rxaddr
 *
 * @return true if match, else false
 */
bool is_cm4_serdes_addr(u32 sbus_addr);

/**
 * @brief Returns true if the sbus_addr is a PCIe SerDes
 *
 * @param sbus_addr address describing a serdes ring and rxaddr
 *
 * @return true if match, else false
 */
bool is_pcie_serdes_addr(u32 sbus_addr);

/**
 * @brief Returns true if the sbus_addr is a Sbus Master Controller Reciever
 *         Memory
 *
 * @param sbus_addr address describing a serdes ring and rxaddr
 *
 * @return true if match, else false
 */
bool is_sbm_crm_addr(u32 sbus_addr);

/**
 * @brief Returns true if the sbus_addr is a Spico processor
 *
 * @param sbus_addr address describing a serdes ring and rxaddr
 *
 * @return true if match, else false
 */
bool is_sbm_spico_addr(u32 sbus_addr);

/**
 * @brief Issue an interrupt to read SerDes memory
 *
 * @param port target port number
 * @param serdes target serdes lane
 * @param addr memory address to read
 * @param data location to store read result
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_mem_rd(void *sbl, u32 port, u32 serdes, u32 addr,
		      u16 *data);

/**
 * @brief Issue an interrupt to write SerDes memory
 *
 * @param port target port number
 * @param serdes target serdes lane
 * @param addr memory address to write
 * @param data write data
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_mem_wr(void *sbl, u32 port, u32 serdes, u32 addr,
		      u16 data);

/**
 * @brief Issue an interrupt to read-modify-write SerDes memory
 *
 * @param port target port number
 * @param serdes target serdes lane
 * @param addr memory address to write
 * @param data write data
 * @param mask write mask
 *
 * @return 0 on success, -1 on failure
 */
int sbl_serdes_mem_rmw(void *sbl, u32 port, u32 serdes, u32 addr,
		       u16 data, u16 mask);

/**
 * @brief Convenience function to issue an SBus op and check the result_code
 *	 and overrun against the original command
 *
 * @param sbus_ring target sbus ring
 * @param reg_addr target sbus register address
 * @param command 8-bit sbus command field
 * @param sbus_data target sbus data
 * @param result location to store result of command (for reads)
 *
 * @return 0 on success, -1 on failure
 */
int sbl_sbus_wr(void *sbl, u32 sbus_addr, u8 reg_addr,
		u32 sbus_data);
int sbl_sbus_rd(void *sbl, u32 sbus_addr, u8 reg_addr,
		u32 *result);
int sbl_sbus_op_aux(void *sbl, u32 sbus_addr, u8 reg_addr,
		    u8 command, u32 sbus_data, u32 *result);

/**
 * @brief upload rom to target sbus address
 *
 * @param sbus_ring target sbus ring
 * @param reg_addr target sbus register address
 * @param fw_size byte size of firmware image
 * @param fw_data byte array of size fw_size
 *
 * @return 0 on success, -1 on failure
 */
int sbl_spico_burst_upload(void *sbl, u32 sbus, u32 reg,
			   size_t fw_size, const u8 *fw_data);

/**
 * @brief Write an interrupt request to a target SBM Spico
 *
 * @param sbus_addr address describing a serdes ring and rxaddr
 * @param code interrupt command
 * @param data interrupt data
 * @param result location to store result of interrupt
 *
 * @return 0 on success, -1 on failure
 */
int sbl_sbm_spico_int(void *sbl, u32 sbus_addr, int code,
		       int data, u32 *result);

/**
 * @brief Write an interrupt request to a set of SerDes Spicos
 *
 * @param config_entry address list describing SerDes lanes
 * @param interrupt command
 * @param data interrupt data
 * @param result pointer to store result in.
 * @param result_action ignore the interrupt result, store it to the result
 *		      pointer, or validate it matches code
 *
 * @return 0 if all results are the same, else -1
 */
int sbl_serdes_spico_int(void *sbl, u32 port, u32 serdes,
			 int code, int data, u16 *result, u8 result_action);
/**
 * @param sbus_addr address describing a serdes ring and rxaddr
 */
int sbl_serdes_spico_int2(void *sbl, u32 sbus_addr, int code,
			  int data, u16 *result, u8 result_action);

#ifdef __cplusplus
}
#endif
#endif // SBL_SBM_SERDES_IFACE_H
