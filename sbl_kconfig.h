/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2019-2020,2022-2023 Hewlett Packard Enterprise Development LP. All rights reserved. */
/*
 *
 * Kconfig fixup for external modules
 *
 *   It is not possible to use the normal Kconfig infrastructure for
 *   externally built modules
 *
 *   When building out-of-tree we define an SBL_EXTERNAL_BUILD=1 environment
 *   variable and we use this to setup the configuration
 *
 *   We must also define one of the following target platforms:
 *    PLATFORM_ROSETTA_HW=1
 *    PLATFORM_CASSINI_HW=1
 *    PLATFORM_CASSINI_EMU=1
 *    PLATFORM_CASSINI_SIM=1
 *
 *   If we ever need to build in-tree then constructing a Kconfig
 *   file should be simple
 *
 */

#ifndef _SBL_KCONFIG_H_
#define _SBL_KCONFIG_H_

#ifdef SBL_EXTERNAL_BUILD

#define CONFIG_SBL				 y

/*
 * currently unused config options we don't want to forget about
 */
#undef  CONFIG_SBL_FAST_AUTONEG
#undef  CONFIG_SBL_MAC_PCS_EMU

/*
 * Rosetta hardware platform
 */
#ifdef SBL_PLATFORM_ROS_HW

#define CONFIG_SBL_PLATFORM_ROS		y
#define CONFIG_SBL_PLATFORM_ROS_HW	y
#define CONFIG_SBL_BUILD_NAME		"rosetta"
#define CONFIG_SBL_NUM_PORTS            64
#define CONFIG_SBL_NUM_SBUS_RINGS       2

/*
 * Cassini hardware platform
 */
#elif defined(SBL_PLATFORM_CAS_HW)

#define CONFIG_SBL_PLATFORM_CAS		y
#define CONFIG_SBL_PLATFORM_CAS_HW	y
#define CONFIG_SBL_BUILD_NAME		"cassini"
#define CONFIG_SBL_NUM_PORTS            1
#define CONFIG_SBL_NUM_SBUS_RINGS       1  /* 2 rings, but only one per NIC */

/*
 * Cassini emulation platform
 */
#elif defined(SBL_PLATFORM_CAS_EMU)

#define CONFIG_SBL_PLATFORM_CAS		y
#define CONFIG_SBL_PLATFORM_CAS_EMU	y
#define CONFIG_SBL_BUILD_NAME		"cassini-emulation"
#define CONFIG_SBL_NUM_PORTS            1
#define CONFIG_SBL_NUM_SBUS_RINGS       1  /* 2 rings, but only one per NIC */

/*
 * Cassini netsim platform
 */
#elif defined(SBL_PLATFORM_CAS_SIM)

#define CONFIG_SBL_PLATFORM_CAS		y
#define CONFIG_SBL_PLATFORM_CAS_SIM	y
#define CONFIG_SBL_BUILD_NAME		"cassini-netsim"
#define CONFIG_SBL_NUM_PORTS            1
#define CONFIG_SBL_NUM_SBUS_RINGS       1 /* 2 rings, but only one per NIC */

#else /* SBL_PLATFORM_* */
/*
 * undefined platform not possible, cant build
 */
#error  "Target platform not defined"
#endif /* SBL_PLATFORM_* */

#else /* SBL_EXTERNAL_BUILD */
/*
 * undefined platform not possible, cant build
 */
#error  "Non-external builds of SBL are not supported!"

#endif /* SBL_EXTERNAL_BUILD */

#undef SBL_PLATFORM_ROS_HW
#undef SBL_PLATFORM_CAS_HW
#undef SBL_PLATFORM_CAS_EMU
#undef SBL_PLATFORM_CAS_SIM

#endif /* _SBL_KCONFIG_H_ */
