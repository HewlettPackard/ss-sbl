# SPDX-License-Identifier: GPL-2.0

#
# Slingshot base link module makefile
#
.PHONY: all default modules clean stage_install stage_uninstall

# Rosetta config.mak
-include ../../config.mak

ifdef SBL_EXTERNAL_BUILD
KCPPFLAGS      += -DSBL_EXTERNAL_BUILD=1
endif

# One of the following must be defined:
# PLATFORM_ROSETTA_HW PLATFORM_CASSINI_HW PLATFORM_CASSINI_EMU or
#  PLATFORM_CASSINI_SIM
ifdef PLATFORM_ROSETTA_HW
KCPPFLAGS      += -DSBL_PLATFORM_ROS_HW=1
else ifdef PLATFORM_CASSINI_HW
KCPPFLAGS      += -DSBL_PLATFORM_CAS_HW=1
else ifdef PLATFORM_CASSINI_EMU
KCPPFLAGS      += -DSBL_PLATFORM_CAS_EMU=1
else ifdef PLATFORM_CASSINI_SIM
KCPPFLAGS      += -DSBL_PLATFORM_CAS_SIM=1
else
$(error Platform not defined! Define one of PLATFORM_ROSETTA_HW PLATFORM_CASSINI_HW PLATFORM_CASSINI_EMU or PLATFORM_CASSINI_SIM)
endif
KCPPFLAGS      += -Werror

INSTALL	       := install -p
TOUCH          := touch
PWD            := $(shell pwd)
KDIR           ?= /lib/modules/$(shell uname -r)/build
STAGING_DIR    ?= /tmp/staging_dir

# Rosetta-specific defines
ifdef PLATFORM_ROSETTA_HW
KCPPFLAGS      += -I$(STAGING_DIR)/usr/include
KCPPFLAGS      += -I$(INSTALL_DIR)/usr/include
# work around broken Debian knl build system
KNL_REL        := $(KERNELRELEASE)
SBL_NAME = sbl
else # Cassini
KCPPFLAGS      += -I$(PWD)/../cassini-headers/install/include -I$(PWD)/include
KNL_REL        := $(KERNELRELEASE)
SBL_NAME = cxi-sbl
endif

KNL_BUILD_ARGS += SBL_NAME=$(SBL_NAME)
KNL_BUILD_ARGS += KCPPFLAGS="$(KCPPFLAGS)"
KNL_BUILD_ARGS += INSTALL_MOD_PATH=$(STAGING_DIR)

MAKE_KNL       := $(MAKE) -C $(KDIR) M=$(PWD) $(KNL_BUILD_ARGS)

default: all

ifdef PLATFORM_ROSETTA_HW
all: stage_install
else
all: modules
endif

modules clean:
	$(MAKE_KNL)  $@

setup:
	./contrib/install_git_hooks.sh

stage_install: setup modules
	$(INSTALL) -d $(STAGING_DIR)
	$(INSTALL) -d $(STAGING_DIR)/lib/modules/$(KNL_REL)
	$(TOUCH) $(STAGING_DIR)/lib/modules/$(KNL_REL)/modules.order
	$(TOUCH) $(STAGING_DIR)/lib/modules/$(KNL_REL)/modules.builtin
	$(MAKE_KNL) modules_install
	$(INSTALL) -m 644 Module.symvers $(STAGING_DIR)/lib/modules/$(KNL_REL)/module.symvers.sbl
	$(INSTALL) -d $(STAGING_DIR)/usr/include/linux
	$(INSTALL) -m 644 sbl.h $(STAGING_DIR)/usr/include/linux
	$(INSTALL) -m 644 sbl_fec.h $(STAGING_DIR)/usr/include/linux
	$(INSTALL) -m 644 sbl_test.h $(STAGING_DIR)/usr/include/linux
	$(INSTALL) -m 644 sbl_an.h $(STAGING_DIR)/usr/include/linux
	$(INSTALL) -d $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 sbl_sbm_serdes_iface.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl_kconfig.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl_serdes_defaults.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl_serdes.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl_iface_constants.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl_counters.h $(STAGING_DIR)/usr/include/uapi
	$(INSTALL) -m 644 uapi/sbl_cassini.h $(STAGING_DIR)/usr/include/uapi

stage_uninstall:
	$(RM) $(STAGING_DIR)/usr/include/linux/sbl.h
	$(RM) $(STAGING_DIR)/usr/include/linux/sbl_test.h
	$(RM) $(STAGING_DIR)/usr/include/linux/sbl_an.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl_sbm_serdes_iface.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl_kconfig.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl.h
	$(RM) $(STAGING_DIR)/usr/include/linux/sbl_fec.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl_serdes_defaults.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl_serdes.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl_iface_constants.h
	$(RM) $(STAGING_DIR)/usr/include/uapi/sbl_counters.h

# 'make rpm' target for Cassini hosts
.PHONY: dist rpm

ifdef PLATFORM_CASSINI_HW
PLATFORM       := PLATFORM_CASSINI_HW
else ifdef PLATFORM_CASSINI_EMU
PLATFORM       := PLATFORM_CASSINI_EMU
else ifdef PLATFORM_CASSINI_SIM
PLATFORM       := PLATFORM_CASSINI_SIM
endif

PACKAGE=cray-slingshot-base-link
#
# this VERSION variable must match 'Version:' in cray-slingshot-base-link.spec
# for `make rpm` to function properly
#
# TODO: find a better way than manually synching the version among two files.
#	see NETCASSINI-5025
#
VERSION=0.9
DISTFILES = $(shell git ls-files 2>/dev/null || find . -type f)

dist: $(DISTFILES)
	tar czf $(PACKAGE)-$(VERSION).tar.gz --transform 's/^/$(PACKAGE)-$(VERSION)\//' $(DISTFILES)

$(PACKAGE)-$(VERSION).tar.gz: dist

rpm: $(PACKAGE)-$(VERSION).tar.gz
	$(if $(PLATFORM_ROSETTA_HW),$(error RPM build only supported for Cassini platforms))
	BUILD_METADATA='0' rpmbuild --define 'platform $(PLATFORM)' -ta $<

# last line
