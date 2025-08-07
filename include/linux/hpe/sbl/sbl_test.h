/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2019-2021 Hewlett Packard Enterprise Development LP */

#ifndef _SBL_TEST_H_
#define _SBL_TEST_H_

int sbl_test_core_intr(struct sbl_inst *sbl, int port_num);

int sbl_test_link_up(struct sbl_inst *sbl, int port_num, int loopback_mode);
int sbl_test_link_reup(struct sbl_inst *sbl, int port_num, int loopback_mode);
int sbl_test_link_down(struct sbl_inst *sbl, int port_num);
int sbl_test_link_reset(struct sbl_inst *sbl, int port_num);
int sbl_test_scratch(struct sbl_inst *sbl, int port_num);
int sbl_test_serdes_stop(struct sbl_inst *sbl, int port_num);
int sbl_test_pcs_tx_rf(struct sbl_inst *sbl, int port_num);
void sbl_test_manipulate_serdes_fw_crc_result(u16 *crc_result);
void sbl_test_inject_serdes_fw_crc_failure(bool set);

#endif /* _SBL_SERDES_H_ */
