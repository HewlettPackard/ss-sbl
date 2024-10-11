/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright 2020 Hewlett Packard Enterprise Development LP */
#ifndef _CASSINI_SBL_UAPI_H_
#define _CASSINI_SBL_UAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cassini platform options provided by the microcontroller
 *
 */
enum cass_uc_platform
{
	SBL_UC_PLATFORM_UNDEFINED = 0,
	SBL_UC_PLATFORM_UNKNOWN,
	SBL_UC_PLATFORM_SAWTOOTH,
	SBL_UC_PLATFORM_BRAZOS,
	SBL_UC_PLATFORM_WASHINGTON,
	SBL_UC_PLATFORM_KENNEBEC,
	SBL_UC_PLATFORM_PANGANI,
};

#ifdef __cplusplus
}
#endif

#endif  /* _CASSINI_SBL_UAPI_H_ */
