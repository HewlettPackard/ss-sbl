# SPDX-License-Identifier: GPL-2.0

#
# Slingshot base link module makefile
#
.PHONY: all default modules clean stage_install stage_uninstall

ifeq ($(PLATFORM_ROSETTA_HW),1)
include ../../../../../../../config.mak
endif

ifdef SBL_EXTERNAL_BUILD
KCPPFLAGS      += -DSBL_EXTERNAL_BUILD=1
endif

ifdef PLATFORM_ROSETTA_HW
KCPPFLAGS      += -DSBL_PLATFORM_ROS_HW=1
else ifdef PLATFORM_CASSINI_HW
KCPPFLAGS      += -DSBL_PLATFORM_CAS_HW=1
else ifdef PLATFORM_CASSINI_EMU
KCPPFLAGS      += -DSBL_PLATFORM_CAS_HW=1
export PLATFORM_CASSINI_HW=1
else ifdef PLATFORM_CASSINI_SIM
KCPPFLAGS      += -DSBL_PLATFORM_CAS_HW=1
export PLATFORM_CASSINI_HW=1
else
$(error Platform not defined! Define PLATFORM_ROSETTA_HW or PLATFORM_CASSINI_HW)
endif
KCPPFLAGS      += -Werror

INSTALL        := install -p
TOUCH          := touch
PWD            := $(shell pwd)
ifeq ($(PLATFORM_CASSINI_HW),1)
KDIR           ?= /lib/modules/$(shell uname -r)/build
endif
STAGING_DIR    ?= /tmp/staging_dir
PKGCONFIG_DIR = $(STAGING_DIR)/usr/lib/pkgconfig

SBL_DRIVER_PATH := .
SBL_UAPI_PATH   := include

HDR_INSTALL_PATH = hpe/sbl
HDR_DIR = $(PWD)/../../../../../include/linux/$(HDR_INSTALL_PATH)
UAPI_HDR_DIR = $(PWD)/../../../../../include/uapi/ethernet

INSTALL_LINUX_HDRS = \
	$(HDR_DIR)/sbl.h \
	$(HDR_DIR)/sbl_fec.h \
	$(HDR_DIR)/sbl_test.h \
	$(HDR_DIR)/sbl_an.h \
	$(HDR_DIR)/sbl_kconfig.h

UNINSTALL_LINUX_HDRS = \
	sbl.h \
	sbl_fec.h \
	sbl_test.h \
	sbl_an.h \
	sbl_kconfig.h \
	sbl_platform.h

ifeq ($(PLATFORM_CASSINI_HW),1)
SBL_PLATFORM_HDR := nic/sbl_platform.h
else
SBL_PLATFORM_HDR := sw/sbl_platform.h
endif

UAPI_ETHERNET_HDRS = \
	sbl_sbm_serdes.h \
	$(UAPI_HDR_DIR)/sbl-abi.h \
	$(UAPI_HDR_DIR)/sbl_serdes_defaults.h \
	$(UAPI_HDR_DIR)/sbl_serdes.h \
	$(UAPI_HDR_DIR)/sbl_sbm_constants.h \
	$(UAPI_HDR_DIR)/sbl_counters.h \
	$(UAPI_HDR_DIR)/sbl_cassini.h \
	$(UAPI_HDR_DIR)/sbl_serdes_map.h \
	sbl_serdes_map_autogen.h \
	$(HDR_DIR)/sbl_kconfig.h

UNINSTALL_UAPI_ETHERNET_HDRS = \
	sbl_sbm_serdes.h \
	sbl-abi.h \
	sbl_serdes_defaults.h \
	sbl_serdes.h \
	sbl_sbm_constants.h \
	sbl_counters.h \
	sbl_cassini.h \
	sbl_serdes_map.h \
	sbl_serdes_map_autogen.h \
	sbl_kconfig.h

ifdef PLATFORM_ROSETTA_HW
KCPPFLAGS      += -I$(STAGING_DIR)/usr/include
KCPPFLAGS      += -I$(INSTALL_DIR)/usr/include
KNL_REL        := $(KERNELRELEASE)
SBL_NAME = sbl
else # Cassini
KCPPFLAGS      += -I$(PWD)/../../../../../../cassini-headers/install/include -I$(PWD)/../../../../../include
KNL_REL        := $(KERNELRELEASE)
ifeq ($(strip $(KNL_REL)),)
ifneq ($(wildcard $(KDIR)/include/config/kernel.release),)
KNL_REL        := $(strip $(shell cat $(KDIR)/include/config/kernel.release))
endif
ifeq ($(strip $(KNL_REL)),)
KNL_REL        := $(shell uname -r)
endif
endif
SBL_NAME = cxi-sbl
endif

KNL_BUILD_ARGS += SBL_NAME=$(SBL_NAME)
KNL_BUILD_ARGS += KCPPFLAGS="$(KCPPFLAGS)"
KNL_BUILD_ARGS += SBL_DRIVER_PATH=$(SBL_DRIVER_PATH)
KNL_BUILD_ARGS += SBL_UAPI_PATH=$(SBL_UAPI_PATH)
KNL_BUILD_ARGS += INSTALL_MOD_PATH=$(STAGING_DIR)

MAKE_KNL       := $(MAKE) -C $(KDIR) M=$(PWD) $(KNL_BUILD_ARGS)

default: all

ifdef PLATFORM_ROSETTA_HW
all: stage_install
else
all: modules
endif

sbldrv.pc: ../../../../../pkgconfig/sbldrv.pc.in
	sed "s|@prefix@|$(STAGING_DIR)|g" $< > $@

platform_cp:
	cp $(SBL_PLATFORM_HDR) $(PWD)/../../../../../include/linux/hpe/sbl/sbl_platform.h

modules: platform_cp
	$(MAKE_KNL) modules

clean:
	if [ -d "$(KDIR)" ];then \
		$(MAKE_KNL) clean; \
	fi; \
	rm -f sbldrv.pc \
	rm -f modules.livepatch \
	rm -f $(PWD)/../../../../../include/linux/hpe/sbl/sbl_platform.h

stage_install: modules sbldrv.pc
	$(INSTALL) -d $(STAGING_DIR)
	$(INSTALL) -d $(STAGING_DIR)/lib/modules/$(KNL_REL)
	$(TOUCH) $(STAGING_DIR)/lib/modules/$(KNL_REL)/modules.order
	$(TOUCH) $(STAGING_DIR)/lib/modules/$(KNL_REL)/modules.builtin
	$(MAKE_KNL) modules_install
	$(INSTALL) -m 644 Module.symvers $(STAGING_DIR)/lib/modules/$(KNL_REL)/module.symvers.sbl
	$(INSTALL) -d $(STAGING_DIR)/usr/include/linux/hpe/sbl
	for hdr in $(INSTALL_LINUX_HDRS); do \
	$(INSTALL) -m 644 $$hdr $(STAGING_DIR)/usr/include/linux/hpe/sbl; \
	done
	$(INSTALL) -m 644 $(SBL_PLATFORM_HDR) $(STAGING_DIR)/usr/include/linux/hpe/sbl/sbl_platform.h
	$(INSTALL) -d $(STAGING_DIR)/usr/include/uapi/ethernet
	for hdr in $(UAPI_ETHERNET_HDRS); do \
	$(INSTALL) -m 644 $$hdr $(STAGING_DIR)/usr/include/uapi/ethernet; \
	done
	$(INSTALL) -D -m 644 sbldrv.pc $(PKGCONFIG_DIR)/sbldrv.pc

stage_uninstall:
	if [ -d "$(STAGING_DIR)" ]; then \
		for hdr in $(UNINSTALL_LINUX_HDRS); do \
			$(RM) $(STAGING_DIR)/usr/include/linux/hpe/sbl/$$hdr; \
		done; \
		for hdr in $(UNINSTALL_UAPI_ETHERNET_HDRS); do \
			$(RM) $(STAGING_DIR)/usr/include/uapi/ethernet/$$hdr; \
		done; \
		$(RM) $(PKGCONFIG_DIR)/sbldrv.pc; \
		if [ -d "$(STAGING_DIR)/lib/modules" ]; then \
			find "$(STAGING_DIR)/lib/modules" -type f \
			\( -name "$(SBL_NAME).ko*" -o -name "modules.*" -o -name "module.symvers.sbl" \) \
			-delete; \
			find "$(STAGING_DIR)/lib/modules" -mindepth 1 -type d -empty -delete; \
		fi; \
		rmdir $(STAGING_DIR)/usr/include/uapi/ethernet; \
		rmdir $(STAGING_DIR)/usr/include/uapi; \
		rmdir $(STAGING_DIR)/usr/include/linux/hpe/sbl; \
		rmdir $(STAGING_DIR)/usr/include/linux/hpe; \
		rmdir $(STAGING_DIR)/usr/include/linux; \
		rmdir $(STAGING_DIR)/usr/include; \
		rmdir $(STAGING_DIR)/usr/lib/pkgconfig; \
		rmdir $(STAGING_DIR)/usr/lib; \
		rmdir $(STAGING_DIR)/usr; \
		rmdir $(STAGING_DIR)/lib/modules; \
		rmdir $(STAGING_DIR)/lib; \
		rmdir $(STAGING_DIR); \
	fi

# last line
