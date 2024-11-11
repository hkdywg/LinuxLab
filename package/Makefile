##
## Makefile
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

PACKAGE_BUILD_TOPDIR := $(srctree)/package
USER_FILE_INSTALL_DIR := $(srctree)/workspace/rootfs/user_install

include $(PACKAGE_BUILD_TOPDIR)/build_core/definition.mk
include $(PACKAGE_BUILD_TOPDIR)/build_core/configure.mk

# Find all of make.inc
include $(PACKAGE_BUILD_TOPDIR)/make.inc

package_all: $(ALL_TARGETS)
	@echo $(ALL_TARGETS)
	@echo "Build Done"

.PHONY: clean

package_clean:
	@rm -rf $(PACKAGE_BUILD_OUTPATH)/*
