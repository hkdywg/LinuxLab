##
## Makefile
##
## History:
##    2024/12/04 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##


DTB_BUILD_TOPDIR := $(srctree)/device_tree
DTB_INSTALL_DIR := $(srctree)/output/device_tree

# Find all of make.inc
#
dtc:
	@mkdir -p $(DTB_INSTALL_DIR)
	@dtc -I dts -O dtb -o $(DTB_INSTALL_DIR)/virtual_device.dtb $(DTB_BUILD_TOPDIR)/virtual_device.dts
	@echo "Build $@ Done"
