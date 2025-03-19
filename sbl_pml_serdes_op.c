// SPDX-License-Identifier: GPL-2.0
/* Copyright 2019-2023 Hewlett Packard Enterprise Development LP */

//#define DEBUG     1

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sbl/sbl_pml.h>


#include "uapi/sbl_kconfig.h"
#include "sbl.h"
#include "sbl_serdes.h"
#include "sbl_internal.h"



static inline bool sbl_pml_serdes_op_busy(struct sbl_inst *sbl, int port_num);



/*
 * Perform a serdes "operation"
 *
 *   Essentially write the op code and its data to a register
 *   then poll for completion bit
 *
 *   We use a mutex as we don't know how long the operation might take so we
 *   might need to sleep. Timing is very approximate as its not very critical.
 *
 *   Currently we lock this to enforce a single operation - we can probably relax this
 *   now its in the sbl
 */
int sbl_pml_serdes_op(struct sbl_inst *sbl, int port_num, u64 serdes_sel,
		u64 op, u64 data, u16 *result, int timeout, unsigned int flags)
{
	struct sbl_link *link;
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;
	int delay;
	int poll_interval;
	unsigned long last_jiffy;
	int err;

	err = sbl_validate_instance(sbl);
	if (err)
		return err;

	err = sbl_validate_port_num(sbl, port_num);
	if (err)
		return err;

	sbl_dev_dbg(sbl->dev, "serdes op, p%ds%lld, %lld, %lld, %d 0x%x\n",
		port_num, serdes_sel, op, data, timeout, flags);

	if (!result)
		return -EINVAL;

	/* cannot wait infinitely */
	if (!timeout)
		return -EINVAL;

	/* a polling interval is mandatory */
	poll_interval = sbl_flags_get_poll_interval_from_flags(flags);
	if (!poll_interval)
		return -EINVAL;

	delay = sbl_flags_get_delay_from_flags(flags);

	link = sbl->link + port_num;
	err = mutex_lock_interruptible(&link->serdes_mtx);
	if (err)
		return -ERESTARTSYS;

	if (sbl_pml_serdes_op_busy(sbl, port_num)) {
		mutex_unlock(&link->serdes_mtx);
		return -EBUSY;
	}

	/* start the operation  */
	val64 = SBL_PML_SERDES_CORE_INTERRUPT_SET(serdes_sel, 1ULL, op, data);
	sbl_write64(sbl, base|SBL_PML_SERDES_CORE_INTERRUPT_OFFSET, val64);
	sbl_read64(sbl, base|SBL_PML_SERDES_CORE_INTERRUPT_OFFSET);  /* flush */

	if (delay)
		udelay(delay);

	/* poll for completion or timeout */
	last_jiffy = jiffies + msecs_to_jiffies(timeout) + 1;
	while (sbl_pml_serdes_op_busy(sbl, port_num)) {
		if (time_is_before_jiffies(last_jiffy)) {
			mutex_unlock(&link->serdes_mtx);
			return -ETIMEDOUT;
		}
		msleep(poll_interval);
	}

	/* get the result */
	val64 = sbl_read64(sbl, base|SBL_PML_SERDES_CORE_INTERRUPT_OFFSET);
	*result = SBL_PML_SERDES_CORE_INTERRUPT_CORE_INTERRUPT_DATA_GET(val64);

	mutex_unlock(&link->serdes_mtx);
	return 0;
}
EXPORT_SYMBOL(sbl_pml_serdes_op);


static inline bool sbl_pml_serdes_op_busy(struct sbl_inst *sbl, int port_num)
{
	u64 val64;

	val64 = sbl_read64(sbl, SBL_PML_BASE(port_num)|
			SBL_PML_SERDES_CORE_INTERRUPT_OFFSET);
	return SBL_PML_SERDES_CORE_INTERRUPT_DO_CORE_INTERRUPT_GET(val64);
}


/*
 * serdes core interrupt access timings
 */
int sbl_pml_serdes_op_timing(struct sbl_inst *sbl, int port_num, u64 capture,
		u64 clear, u64 set)
{
	u32 base = SBL_PML_BASE(port_num);
	u64 val64;

	val64 = SBL_PML_CFG_SERDES_CORE_INTERRUPT_CAPTURE_INTERRUPT_DATA_DELAY_SET(capture) |
		SBL_PML_CFG_SERDES_CORE_INTERRUPT_CLEAR_INTERRUPT_DELAY_SET(clear) |
		SBL_PML_CFG_SERDES_CORE_INTERRUPT_SET_INTERRUPT_DELAY_SET(set);

	sbl_write64(sbl, base|SBL_PML_CFG_SERDES_CORE_INTERRUPT_OFFSET,
			val64);
	return 0;
}
EXPORT_SYMBOL(sbl_pml_serdes_op_timing);
