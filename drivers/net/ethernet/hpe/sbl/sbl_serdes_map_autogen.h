/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2019-2024 Hewlett Packard Enterprise Development LP */
#define SBL_CASSINI_BRAZOS_SW_INFO_INITIALIZER \
{ \
	.num_ports = CONFIG_SBL_NUM_PORTS, \
	.num_serdes = SBL_SERDES_LANES_PER_PORT, \
	.num_sbus_rings = CONFIG_SBL_NUM_SBUS_RINGS, \
	.ports = { \
	{ /* 0x00 */ \
		.rx_an_swizzle = 3, \
		.tx_an_swizzle = 1, \
		.serdes = { \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x28, .tx_lane_source = 1, .txinv = true, .rx_lane_source = 2, .rxinv = false }, \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x29, .tx_lane_source = 0, .txinv = true, .rx_lane_source = 3, .rxinv = false }, \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x2A, .tx_lane_source = 2, .txinv = true, .rx_lane_source = 1, .rxinv = false }, \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x2B, .tx_lane_source = 3, .txinv = true, .rx_lane_source = 0, .rxinv = false }, \
			} \
		} \
	} \
}
#define SBL_CASSINI_NIC0_SW_INFO_INITIALIZER \
{ \
	.num_ports = CONFIG_SBL_NUM_PORTS, \
	.num_serdes = SBL_SERDES_LANES_PER_PORT, \
	.num_sbus_rings = CONFIG_SBL_NUM_SBUS_RINGS, \
	.ports = { \
	{ /* 0x00 */ \
		.rx_an_swizzle = 3, \
		.tx_an_swizzle = 3, \
		.serdes = { \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x28, .tx_lane_source = 3, .txinv = true, .rx_lane_source = 3, .rxinv = true }, \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x29, .tx_lane_source = 2, .txinv = true, .rx_lane_source = 2, .rxinv = true }, \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x2A, .tx_lane_source = 1, .txinv = true, .rx_lane_source = 1, .rxinv = true }, \
			{ .device = 0, .sbus_ring = 0, .rx_addr = 0x2B, .tx_lane_source = 0, .txinv = true, .rx_lane_source = 0, .rxinv = true }, \
			} \
		} \
	} \
}
#define SBL_CASSINI_NIC1_SW_INFO_INITIALIZER \
{ \
	.num_ports = CONFIG_SBL_NUM_PORTS, \
	.num_serdes = SBL_SERDES_LANES_PER_PORT, \
	.num_sbus_rings = CONFIG_SBL_NUM_SBUS_RINGS, \
	.ports = { \
	{ /* 0x00 */ \
		.rx_an_swizzle = 0, \
		.tx_an_swizzle = 0, \
		.serdes = { \
			{ .device = 0, .sbus_ring = 1, .rx_addr = 0x28, .tx_lane_source = 0, .txinv = true, .rx_lane_source = 0, .rxinv = true }, \
			{ .device = 0, .sbus_ring = 1, .rx_addr = 0x29, .tx_lane_source = 1, .txinv = true, .rx_lane_source = 1, .rxinv = true }, \
			{ .device = 0, .sbus_ring = 1, .rx_addr = 0x2A, .tx_lane_source = 2, .txinv = true, .rx_lane_source = 2, .rxinv = true }, \
			{ .device = 0, .sbus_ring = 1, .rx_addr = 0x2B, .tx_lane_source = 3, .txinv = true, .rx_lane_source = 3, .rxinv = true }, \
			} \
		} \
	} \
}
