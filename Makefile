# SPDX-License-Identifier: GPL-2.0

.PHONY: all modules clean stage_install stage_uninstall dist rpm setup

SBL_DIR := drivers/net/ethernet/hpe/sbl

all modules clean stage_install stage_uninstall:
	$(MAKE) -C $(SBL_DIR) $@

setup:
	./contrib/install_git_hooks.sh

ifdef PLATFORM_CASSINI_HW
PLATFORM       := PLATFORM_CASSINI_HW
else ifdef PLATFORM_CASSINI_EMU
PLATFORM       := PLATFORM_CASSINI_HW
else ifdef PLATFORM_CASSINI_SIM
PLATFORM       := PLATFORM_CASSINI_HW
endif

PACKAGE=cray-slingshot-base-link
VERSION=0.9
DISTFILES = $(shell git ls-files 2>/dev/null || find . -type f)

dist: $(DISTFILES)
	@tar czf $(PACKAGE)-$(VERSION).tar.gz --transform 's/^/$(PACKAGE)-$(VERSION)\//' $(DISTFILES)

$(PACKAGE)-$(VERSION).tar.gz: dist

rpm: $(PACKAGE)-$(VERSION).tar.gz
	@$(if $(PLATFORM_ROSETTA_HW),$(error RPM build only supported for Cassini platforms))
	@BUILD_METADATA='0' rpmbuild --define 'platform $(PLATFORM)' -ta $<

default: all

# last line
