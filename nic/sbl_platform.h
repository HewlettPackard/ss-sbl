/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#define SBUS_RING_MTX(sbl, ring) ((sbl)->sbus_ring_mtx)
#define SBM_FW_MTX(sbl, ring) ((sbl)->sbm_fw_mtx)

#define sbl_dev_err(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_err((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_err((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_dbg(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_dbg((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_dbg((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_warn(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_warn((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_warn((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_info(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_info((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_info((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_err_ratelimited(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_err_ratelimited((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_err_ratelimited((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
#define sbl_dev_dbg_ratelimited(_dev, _fmt, ...) \
do { \
	if (!sbl) \
		dev_dbg_ratelimited((_dev), (_fmt), ##__VA_ARGS__); \
	else \
		dev_dbg_ratelimited((_dev), "%s[%s]: " _fmt, sbl->iattr.inst_name, sbl->iattr.eth_if_name, ##__VA_ARGS__); \
} while (0)
/*
 * Configuration passing into SBL init
 */
struct sbl_init_attr {
	u32 magic;
	u32 uc_nic;
	u32 uc_platform;
	bool is_hw;
};
