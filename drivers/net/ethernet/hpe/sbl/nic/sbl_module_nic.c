// SPDX-License-Identifier: GPL-2.0

/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/of.h>

#include <linux/hpe/sbl/sbl.h>
#include <linux/hpe/sbl/sbl_kconfig.h>

#include "sbl_internal.h"
#include "sbl_serdes_map.h"


#define DEVICE_NAME         "sbl"

static struct class  *sbl_cls;
static struct device *sbl_dev;
static dev_t          sbl_devt;

#define tostr(x) #x
#define version_str(a, b, c) tostr(a.b.c)
#define SBL_VERSION_STR version_str(SBL_VERSION_MAJOR,	\
				    SBL_VERSION_MINOR,	\
				    SBL_VERSION_INC)

/*
 * module init function
 *
 *   The char device is only used for fw loading and printing to the console
 *   For now multiple instances all share the same device
 */
static int __init sbl_init(void)
{
	int err;

	pr_info("%s : v" SBL_VERSION_STR " loading (" CONFIG_SBL_BUILD_NAME " build)\n", module_name(THIS_MODULE));

	/*
	 * create a device
	 */
	err = alloc_chrdev_region(&sbl_devt, 0, 1, DEVICE_NAME);
	if (err) {
		pr_err("%s : failed to get dev region [%d]\n",
				module_name(THIS_MODULE), err);
		goto out;
	}

	#if (defined(RHEL_MAJOR) && (RHEL_MAJOR >= 9) && defined(RHEL_MINOR) && (RHEL_MINOR >= 4)) || (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
		sbl_cls = class_create(DEVICE_NAME);
	#else
		sbl_cls = class_create(THIS_MODULE, DEVICE_NAME);
	#endif
	if (IS_ERR(sbl_cls)) {
		pr_err("%s : class create failed [%ld]\n",
				module_name(THIS_MODULE), PTR_ERR(sbl_cls));
		err = PTR_ERR(sbl_cls);
		goto out_region;
	}

	sbl_dev = device_create(sbl_cls, NULL, sbl_devt, NULL,
			module_name(THIS_MODULE));
	if (IS_ERR(sbl_dev)) {
		err = PTR_ERR(sbl_dev);
		pr_err("%s : failed to add dev [%d]\n",
				module_name(THIS_MODULE), err);
		goto out_class;
	}

	return 0;

out_class:
	class_destroy(sbl_cls);
out_region:
	unregister_chrdev_region(sbl_devt, 1);
out:
	return err;
}
module_init(sbl_init);


struct device *sbl_get_device(void)
{
	return sbl_dev;
}

/*
 * module exit function
 */
static void __exit sbl_exit(void)
{
	pr_info("%s : module unloading\n", module_name(THIS_MODULE));

	device_destroy(sbl_cls, sbl_devt);
	class_destroy(sbl_cls);
	unregister_chrdev_region(sbl_devt, 1);

}
module_exit(sbl_exit);

static int llr_cap_set(const char *val, const struct kernel_param *kp)
{
	int           err;
	unsigned long data;

	err = kstrtoul(val, 0, &data);
	if (err || (data > 0x800))
		return -EINVAL;

	return param_set_ulong(val, kp);
}
static const struct kernel_param_ops llr_cap_ops = {
	.set = llr_cap_set,
	.get = param_get_ulong,
};

static unsigned long llr_edge_cap_data_max = 0x320;
module_param_cb(llr_edge_cap_data_max, &llr_cap_ops, &llr_edge_cap_data_max, 0664);
MODULE_PARM_DESC(llr_edge_cap_data_max, "LLR data capacity max for edge link");
u64 sbl_llr_edge_cap_data_max_get(void)
{
	return llr_edge_cap_data_max;
}

static unsigned long llr_edge_cap_seq_max = 0x160;
module_param_cb(llr_edge_cap_seq_max, &llr_cap_ops, &llr_edge_cap_seq_max, 0664);
MODULE_PARM_DESC(llr_edge_cap_seq_max, "LLR sequence num capacity max for edge link");
u64 sbl_llr_edge_cap_seq_max_get(void)
{
	return llr_edge_cap_seq_max;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HPE Slingshot <support@hpe.com>");
MODULE_DESCRIPTION("Slingshot Base Link module");
MODULE_VERSION(SBL_VERSION_STR);
