/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019,2021-2024 Hewlett Packard Enterprise Development LP. All rights reserved. */

#ifndef SBL_IFACE_H
#define SBL_IFACE_H

#ifdef __KERNEL__
#include "sbl.h"
#include "sbl_internal.h"
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#endif /* __KERNEL__ */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__

#define SBL_INST struct sbl_inst
#define SBL_ERR(dev, format, args...)    sbl_dev_err(dev, format, ## args)
#define SBL_WARN(dev, format, args...)   sbl_dev_warn(dev, format, ## args)
#define SBL_INFO(dev, format, args...)   sbl_dev_info(dev, format, ## args)
#define SBL_TRACE1(dev, format, args...) dev_dbg(dev, format, ## args)
#define SBL_TRACE2(dev, format, args...) DEV_TRACE2(dev, format, ## args)
#define SBL_TRACE3(dev, format, args...) DEV_TRACE3(dev, format, ## args)

inline int sbl_iface_sbus_op(SBL_INST * sbl, int ring, u32 req_data,
			     u8 data_addr, u8 rx_addr, u8 command,
			     u32 *rsp_data, u8 *result_code, u8 *overrun,
			     int timeout, u32 flags)
{
	return sbl_sbus_op(sbl, ring, req_data, data_addr, rx_addr, command,
			   rsp_data, result_code, overrun, timeout, flags);
}

inline int sbl_iface_sbus_op_reset(SBL_INST *sbl, int ring)
{
	return sbl_sbus_op_reset(sbl, ring);
}

inline int sbl_iface_pml_serdes_op(SBL_INST *sbl, int port_num,
				   u64 serdes_sel, u64 op, u64 data,
				   u16 *result, int timeout, u32 flags)
{
	return sbl_pml_serdes_op(sbl, port_num, serdes_sel, op, data, result,
				 timeout, flags);
}

inline int sbl_iface_get_sbus_op_timeout_ms(SBL_INST *sbl)
{
	return sbl->iattr.sbus_op_timeout_ms;
}

inline int sbl_iface_get_sbus_int_timeout(SBL_INST *sbl)
{
	return sbl->iattr.sbus_int_timeout;
}

inline int sbl_iface_get_sbus_int_poll_interval(SBL_INST *sbl)
{
	return sbl->iattr.sbus_int_poll_interval;
}

inline int sbl_iface_get_serdes_op_timeout_ms(SBL_INST *sbl)
{
	return sbl->iattr.serdes_op_timeout_ms;
}

inline int sbl_iface_get_sbus_op_flags(SBL_INST *sbl)
{
	return sbl->sbus_op_flags;
}

inline int sbl_iface_get_serdes_op_flags(SBL_INST *sbl)
{
	return sbl->iattr.serdes_op_flags;
}

#else
#include "hms_sbl_iface.h"
#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif
#endif // SBL_IFACE_H
