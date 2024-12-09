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


SUB_DTS_TOPDIR := $(srctree)/package/device_driver
DTB_INSTALL_DIR := $(srctree)/output/device_tree
newline := $(shell echo)


# 主设备树文件
MAIN_DTS := $(DTB_INSTALL_DIR)/build_main.dts
TOP_SOURCE_DTS := $(srctree)/device_tree/virtual_device.dts

# 自动搜索所有子目录下的 .dts 文件
DTS_FILES = $(shell find $(SUB_DTS_TOPDIR) -type f -name "*.dts" | tr ' ' '\n')

# 将所有 .dts 文件包含到主设备树文件中
INCLUDE_DTS = $(foreach dts_file, $(DTS_FILES), \#include "$(dts_file)")

HOST_CC := gcc
HOST_DTC := dtc

CFLAGS := -E -Wp,-MMD,

DTC_FLAGS := 0 -Wno-unit_address_vs_reg -Wno-unit_address_format \
             -Wno-avoid_unnecessary_addr_size -Wno-alias_paths -Wno-graph_child_address \
             -Wno-simple_bus_reg -Wno-unique_unit_address -Wno-pci_device_reg

DTC_DEF := -undef -D__DTS__ -x assembler-with-cpp

DTB_D_PRE_TMP := $(patsubst %.dts,%.dtb.d.pre.tmp,$(MAIN_DTS))
DTB_D_DTC_TMP := $(patsubst %.dts,%.dtb.d.dtc.tmp,$(MAIN_DTS))
DTB_D_TMP := $(patsubst %.dts,%.dtb.dts.tmp,$(MAIN_DTS))
DTB_TARGET := $(shell basename $(patsubst %.dts,%.dtb,$(TOP_SOURCE_DTS)))

# 生成最终的需要编译的设备树源文件（build_main.dts）
$(MAIN_DTS): $(DTS_FILES)
	@cp $(srctree)/device_tree/virtual_device.dts $(DTB_INSTALL_DIR)/
	@sed '/\/dts-v1\/;/a $(INCLUDE_DTS)' $(TOP_SOURCE_DTS) > $@
	@sed -i 's/  #/\n#/g' $@

# 编译设备树源文件为二进制文件（.dtb）
dtc: $(MAIN_DTS)
	@$(HOST_CC) -E $(CFLAGS)$(DTB_D_PRE_TMP) -nostdinc $(DTC_DEF) -o $(DTB_D_TMP) $(MAIN_DTS)
	@$(HOST_DTC) -O dtb -o $(DTB_INSTALL_DIR)/$(DTB_TARGET) -b $(DTC_FLAGS) -d $(DTB_D_DTC_TMP) $(DTB_D_TMP)

# 清理生成的文件
dtc_clean:
	@rm -rf $(DTB_INSTALL_DIR)/* 

.PHONY: dtc_clean dtc

