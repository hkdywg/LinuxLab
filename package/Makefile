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

.PHONY: all
all:

BUILD_TOPDIR := $(shell pwd)

include $(BUILD_TOPDIR)/build_core/definition.mk
include $(BUILD_TOPDIR)/build_core/configure.mk

# Find all of make.inc
#include $(BUILD_TOPDIR)/make.inc
include $(call all-subdir-makefiles)

all: $(ALL_TARGETS)
    @echo $(MAKEFILE_LIST)
    @echo $(ALL_TARGETS)
    @echo "Build Done"

.PHONY: clean

clean:
    @rm -rf $(PACKAGE_BUILD_OUTPATH)/*
