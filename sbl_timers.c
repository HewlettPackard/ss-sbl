// SPDX-License-Identifier: GPL-2.0
/* Copyright 2019-2024 Hewlett Packard Enterprise Development LP */


//#define DEBUG     1

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <sbl/sbl_pml.h>


#include "sbl_kconfig.h"
#include "sbl.h"
#include "sbl_pml.h"
#include "sbl_link.h"
#include "sbl_internal.h"


/*
 * start timeout is the limit for sbl_base_link_start() to bring the link up
 * this includes all stages of link bringup
 */
void sbl_link_init_start_timeout(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->start_timeout = link->blattr.start_timeout;
	link->an_timeout_active = false;
	link->last_start_jiffy = jiffies + msecs_to_jiffies(1000*link->start_timeout);
	link->start_time_begin = ktime_get();
	link->total_tune_time.tv_sec = 0;
	link->total_tune_time.tv_nsec = 0;
	spin_unlock(&link->timeout_lock);
}


/*
 * change the start timeout
 */
void sbl_link_update_start_timeout(struct sbl_inst *sbl, int port_num, unsigned int timeout_ms)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->start_timeout = DIV_ROUND_UP(timeout_ms, 1000);
	link->last_start_jiffy = jiffies + msecs_to_jiffies(timeout_ms);
	spin_unlock(&link->timeout_lock);

	sbl_dev_dbg(sbl->dev, "%d: update start timeout to %d s, %d ms\n",
			port_num, link->start_timeout, timeout_ms);
}


/*
 * test if we have timed out - returns true if we have
 */
bool sbl_start_timeout(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	unsigned long last_start_jiffy;
	bool timed_out;

	spin_lock(&link->timeout_lock);
	last_start_jiffy = link->last_start_jiffy;
	timed_out = time_is_before_jiffies(last_start_jiffy);
	spin_unlock(&link->timeout_lock);

	if (timed_out)
		sbl_dev_dbg(sbl->dev, "%d: %s timed out\n", port_num, __func__);

	return timed_out;
}


u32 sbl_get_start_timeout(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u32 start_timeout;

	spin_lock(&link->timeout_lock);
	start_timeout = link->start_timeout;
	spin_unlock(&link->timeout_lock);

	return start_timeout;
}


/*
 * update start timeout to ensure there is a minimum  amout of time remaining
 */
void sbl_start_timeout_ensure_remaining(struct sbl_inst *sbl, int port_num, unsigned int remaining_s)
{
	struct sbl_link *link = sbl->link + port_num;
	unsigned long now;
	unsigned long left;
	unsigned long required = msecs_to_jiffies(1000*remaining_s);

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);

	/* Don't mess with timeouts setup by autoneg */
	if (link->an_timeout_active) {
		spin_unlock(&link->timeout_lock);
		return;
	}

	now = jiffies;
	if (time_before(now, link->last_start_jiffy)) {
		left = link->last_start_jiffy - now;
		if (left < required) {
			sbl_dev_info(sbl->dev, "%d: extending timeout by %ld\n", port_num,
					(required-left)/HZ);
			link->start_timeout += required/HZ;
			link->last_start_jiffy += required;
		} else {
			sbl_dev_dbg(sbl->dev, "%d: timeout OK - now %ld, last %ld, left %ld, req %ld\n",
					port_num, now, link->last_start_jiffy, left, required);
		}
	} else {
		/* we should timeout but extend instead */
		sbl_dev_info(sbl->dev, "%d: extending timeout by %ld\n", port_num,
				required/HZ);
		link->start_timeout += required/HZ;
		link->last_start_jiffy += required;
	}

	spin_unlock(&link->timeout_lock);
}


int sbl_link_start_elapsed(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct timespec64 elapsed;

	spin_lock(&link->timeout_lock);
	elapsed = ktime_to_timespec64(ktime_sub(ktime_get(), link->start_time_begin));
	spin_unlock(&link->timeout_lock);

	return elapsed.tv_sec;
}


void sbl_link_start_record_timespec(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->start_time = ktime_to_timespec64(ktime_sub(ktime_get(), link->start_time_begin));
	spin_unlock(&link->timeout_lock);
}


/*
 * up time is the time to tune serdes and bring-up pml
 */
void sbl_link_up_begin(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->up_time_begin = ktime_get();
	spin_unlock(&link->timeout_lock);
}


void sbl_link_up_record_timespec(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->up_time = ktime_to_timespec64(ktime_sub(ktime_get(), link->up_time_begin));
	spin_unlock(&link->timeout_lock);
}


/*
 * tune time is the time serdes spent actually tuning
 *
 */
void sbl_link_tune_begin(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->tune_time_begin = ktime_get();
	spin_unlock(&link->timeout_lock);
}


int sbl_link_tune_elapsed(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	struct timespec64 elapsed;

	spin_lock(&link->timeout_lock);
	elapsed = ktime_to_timespec64(ktime_sub(ktime_get(), link->tune_time_begin));
	spin_unlock(&link->timeout_lock);

	return elapsed.tv_sec;
}


void sbl_link_tune_zero_total_timespec(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->tune_time.tv_sec = 0;
	link->tune_time.tv_nsec = 0;
	spin_unlock(&link->timeout_lock);
}


void sbl_link_tune_update_total_timespec(struct sbl_inst *sbl, int port_num)
{
	struct sbl_link *link = sbl->link + port_num;
	u64 ns;

	sbl_dev_dbg(sbl->dev, "%d: %s\n", port_num, __func__);

	spin_lock(&link->timeout_lock);
	link->tune_time = ktime_to_timespec64(ktime_sub(ktime_get(), link->tune_time_begin));

	link->total_tune_time.tv_sec += link->tune_time.tv_sec;
	ns = link->total_tune_time.tv_nsec + link->tune_time.tv_nsec;
	if (ns > 1000000000ULL) {
		link->total_tune_time.tv_sec += 1;
		ns -= 1000000000ULL;
	}
	link->total_tune_time.tv_nsec = ns;
	spin_unlock(&link->timeout_lock);
}


