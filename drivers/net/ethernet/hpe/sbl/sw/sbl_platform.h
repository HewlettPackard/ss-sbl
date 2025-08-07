/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#define SBUS_RING_MTX(sbl, ring) ((sbl)->sbus_ring_mtx + (ring))

#define SBM_FW_MTX(sbl, ring) ((sbl)->sbm_fw_mtx + (ring))

#define sbl_dev_err(_dev, _fmt, ...) dev_err((_dev), (_fmt), ##__VA_ARGS__)
#define sbl_dev_dbg(_dev, _fmt, ...) dev_dbg((_dev), (_fmt), ##__VA_ARGS__)
#define sbl_dev_warn(_dev, _fmt, ...) dev_warn((_dev), (_fmt), ##__VA_ARGS__)
#define sbl_dev_info(_dev, _fmt, ...) dev_info((_dev), (_fmt), ##__VA_ARGS__)
#define sbl_dev_err_ratelimited(_dev, _fmt, ...) dev_err_ratelimited((_dev), (_fmt), ##__VA_ARGS__)
#define sbl_dev_dbg_ratelimited(_dev, _fmt, ...) dev_dbg_ratelimited((_dev), (_fmt), ##__VA_ARGS__)
/*
 * Configuration passing into SBL init
 */
struct sbl_init_attr {
	u32 magic;
};

u64 sbl_llr_fabric_cap_data_max_get(void);
u64 sbl_llr_fabric_cap_seq_max_get(void);
