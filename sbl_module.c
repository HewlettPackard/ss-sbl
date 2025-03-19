// SPDX-License-Identifier: GPL-2.0

/* Copyright 2019-2024 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "uapi/sbl_kconfig.h"
#include "sbl.h"
#include <linux/of.h>
#include "sbl_internal.h"
#include "sbl_serdes_map.h"


#define DEVICE_NAME         "sbl"

static struct class  *sbl_cls;
static struct device *sbl_dev;
static dev_t          sbl_devt;
struct sbl_switch_info *sbl_switch_info;

#define tostr(x) #x
#define version_str(a, b, c) tostr(a.b.c)
#define SBL_VERSION_STR version_str(SBL_VERSION_MAJOR,	\
				    SBL_VERSION_MINOR,	\
				    SBL_VERSION_INC)

static int update_sbl_ports_info_from_dt(struct device_node *starting_node,
					struct sbl_switch_info *sbl_ports_info)
{
	struct device_node *port, *lane;
	int val, ret, port_index, lane_index;
	char port_buffer[10], lane_buffer[10];

	if (!starting_node || !sbl_ports_info) {
		pr_err("%s : starting_node is %s sbl_ports_info is %s\n", module_name(THIS_MODULE),
								starting_node ? "Non Null" : "Null",
								sbl_ports_info ? "Non Null" : "Null");
		goto out;
	}

	ret = of_property_read_u32(starting_node, "num_ports", &val);
	if (!ret) {
		sbl_ports_info->num_ports = val;
	} else {
		pr_err("%s : Unable to get num_ports\n", module_name(THIS_MODULE));
		goto out;
	}

	ret = of_property_read_u32(starting_node, "num_serdes", &val);
	if (!ret) {
		sbl_ports_info->num_serdes = val;
	} else {
		pr_err("%s : Unable to get num_serdes\n", module_name(THIS_MODULE));
		goto out;
	}

	ret = of_property_read_u32(starting_node, "num_sbus_rings", &val);
	if (!ret) {
		sbl_ports_info->num_sbus_rings = val;
	} else {
		pr_err("%s : Unable to get num_sbus_rings\n", module_name(THIS_MODULE));
		goto out;
	}

	for (port_index = 0; port_index < sbl_ports_info->num_ports; port_index++) {
		snprintf(port_buffer, 10, "port%d", port_index);
		port = of_find_node_by_name(starting_node, port_buffer);
		if (port) {
			ret = of_property_read_u32(port, "rx_an_swizzle", &val);
			if (!ret) {
				sbl_ports_info->ports[port_index].rx_an_swizzle = val;
			} else {
				pr_err("%s : Unable to get port%d rx_an_swizzle\n", module_name(THIS_MODULE), port_index);
				goto out_1;
			}

			ret = of_property_read_u32(port, "tx_an_swizzle", &val);
			if (!ret) {
				sbl_ports_info->ports[port_index].tx_an_swizzle = val;
			} else {
				pr_err("%s : Unable to get port%d tx_an_swizzle\n", module_name(THIS_MODULE), port_index);
				goto out_1;
			}

			for (lane_index = 0; lane_index < sbl_ports_info->num_serdes; lane_index++) {
				snprintf(lane_buffer, 10, "lane%d", lane_index);
				lane = of_find_node_by_name(port, lane_buffer);
				if (lane) {
					ret = of_property_read_u32(lane, "device", &val);
					if (!ret) {
						sbl_ports_info->ports[port_index].serdes[lane_index].device = val;
					} else {
						pr_err("%s : Unable to get port%d lane%d device\n", module_name(THIS_MODULE),
								      port_index, lane_index);
						goto out_2;
					}

					ret = of_property_read_u32(lane, "sbus_ring", &val);
					if (!ret) {
						sbl_ports_info->ports[port_index].serdes[lane_index].sbus_ring = val;
					} else {
						pr_err("%s : Unable to get port%d lane%d sbus_ring\n",
							module_name(THIS_MODULE), port_index, lane_index);
						goto out_2;
					}

					ret = of_property_read_u32(lane, "rx_addr", &val);
					if (!ret) {
						sbl_ports_info->ports[port_index].serdes[lane_index].rx_addr = val;
					} else {
						pr_err("%s : Unable to get port%d lane%d rx_addr\n",
								module_name(THIS_MODULE), port_index, lane_index);
						goto out_2;
					}

					ret = of_property_read_u32(lane, "tx_lane_source", &val);
					if (!ret) {
						sbl_ports_info->ports[port_index].serdes[lane_index].tx_lane_source = val;
					} else {
						pr_err("%s : Unable to get port%d lane%d tx_lane_source\n",
								module_name(THIS_MODULE), port_index, lane_index);
						goto out_2;
					}

					ret = of_property_read_u32(lane, "rx_lane_source", &val);
					if (!ret) {
						sbl_ports_info->ports[port_index].serdes[lane_index].rx_lane_source = val;
					} else {
						pr_err("%s : Unable to get port%d lane%d rx_lane_source\n",
								module_name(THIS_MODULE), port_index, lane_index);
						goto out_2;
					}

					ret = of_property_read_bool(lane, "txinv");
					sbl_ports_info->ports[port_index].serdes[lane_index].txinv = ret;

					ret = of_property_read_bool(lane, "rxinv");
					sbl_ports_info->ports[port_index].serdes[lane_index].rxinv = ret;
					of_node_put(lane);
				} else {
					pr_err("%s : Unable to get port%d lane%d by of_find_node_by_name\n",
								module_name(THIS_MODULE), port_index, lane_index);
					goto out_1;
				}
			}
			of_node_put(port);
		} else {
			pr_err("%s : Unable to get port%d by of_find_node_by_name\n",
					module_name(THIS_MODULE), port_index);
			goto out;
		}
	}
	return 0;
out_2:
	of_node_put(lane);
out_1:
	of_node_put(port);
out:
	return -1;
}

static int update_sbl_switch_info_from_dt(void)
{
	struct device_node *switch_node, *node;
	int  val, switch_index;
	int ret, num_of_rosetta;
	char switch_buffer[20];
	struct sbl_switch_info *rosetta_switch_info = NULL;

	node = of_find_node_by_name(NULL, "rosettas");
	if (node) {
		ret = of_property_read_u32(node, "num_rosetta", &val);
		if (!ret) {
			num_of_rosetta = val;
		} else {
			pr_err("%s : Unable to get num_of_rosetta\n", module_name(THIS_MODULE));
			goto out_1;
		}
	} else {
		pr_debug("%s : Not a switch node\n", module_name(THIS_MODULE));
		goto out;
	}

	rosetta_switch_info = kzalloc(num_of_rosetta * sizeof(struct sbl_switch_info), GFP_KERNEL);
	if (!rosetta_switch_info)
		goto out_1;

	for (switch_index = 0; switch_index < num_of_rosetta; switch_index++) {
		snprintf(switch_buffer, 20, "rosetta%d", switch_index);

		switch_node = of_find_node_by_name(node, switch_buffer);
		if (switch_node) {
			ret = update_sbl_ports_info_from_dt(switch_node, &rosetta_switch_info[switch_index]);
			if (ret) {
				/* Error already logged */
				goto out_2;
			}
			of_node_put(switch_node);
		} else {
			pr_err("%s : Unable to get switch node by of_find_node_by_name\n",
					module_name(THIS_MODULE));
			goto out_1;
		}
	}

	of_node_put(node);

	sbl_switch_info = rosetta_switch_info;

	return 0;
out_2:
	of_node_put(switch_node);
out_1:
	of_node_put(node);
out:
	kfree(rosetta_switch_info);
	rosetta_switch_info = NULL;

	return -1;
}

/*
 * module init function
 *
 *   The char device is only used for fw loading and printing to the console
 *   For now multiple instances all share the same device
 */
//#define __init
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

	update_sbl_switch_info_from_dt();

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

struct sbl_switch_info *sbl_get_switch_info(int *size)
{
	if (size != NULL) {
		if (sbl_switch_info != NULL)
			*size = sizeof(struct sbl_switch_info);
		else
			*size = 0;
	}
	return sbl_switch_info;
}
EXPORT_SYMBOL(sbl_get_switch_info);

/*
 * module exit function
 */
//#define __exit
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

#if defined(CONFIG_SBL_PLATFORM_ROS_HW)
static unsigned long llr_fabric_cap_data_max = 0x800; /* HW reset value */
module_param_cb(llr_fabric_cap_data_max, &llr_cap_ops, &llr_fabric_cap_data_max, 0664);
MODULE_PARM_DESC(llr_fabric_cap_data_max, "LLR data capacity max for fabric link");
u64 sbl_llr_fabric_cap_data_max_get(void)
{
	return llr_fabric_cap_data_max;
}

static unsigned long llr_fabric_cap_seq_max  = 0x800; /* HW reset value */
module_param_cb(llr_fabric_cap_seq_max, &llr_cap_ops, &llr_fabric_cap_seq_max, 0664);
MODULE_PARM_DESC(llr_fabric_cap_seq_max, "LLR sequence num capacity max for fabric link");
u64 sbl_llr_fabric_cap_seq_max_get(void)
{
	return llr_fabric_cap_seq_max;
}
#endif


MODULE_LICENSE("GPL");
MODULE_AUTHOR("HPE Slingshot <support@hpe.com>");
MODULE_DESCRIPTION("Slingshot Base Link module");
MODULE_VERSION(SBL_VERSION_STR);
