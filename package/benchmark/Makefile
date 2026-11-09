##
## benchmark/Makefile
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## Copyright (C) 2016, JiangSu New Vision Automotive CO.,LTD.
## 
## All rights reserved. This software is New Vision property. Duplication
## or disclosure without New Vision written authorization is prohibited.
##

.PHONY: all
all:

BUILD_TOPDIR := $(shell pwd)

include $(BUILD_TOPDIR)/build_core/definition.mk
include $(BUILD_TOPDIR)/build_core/configure.mk

# Find all of make.inc
include $(BUILD_TOPDIR)/make.inc

all: $(ALL_TARGETS)
	@echo $(MAKEFILE_LIST)
	@echo $(ALL_TARGETS)
	@echo "Build Done"

.PHONY: clean 

clean:
	@rm -rf $(BUILD_OUT_TOPDIR)


