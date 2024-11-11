##
## build_core/build_driver.mk
##
## History:
##    2024/11/11 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

OUT_DRIVER_DIR := $(patsubst $(PACKAGE_BUILD_TOPDIR)/%,$(KERNEL_BUILD_OUTPATH)/driver_modules/%,$(LOCAL_DRIVER_DIR))
BUILD_DIR_MAKEFILE := $(strip $(OUT_DRIVER_DIR)/Makefile)
LOCAL_DRIVER_INSTALL_PATH := $(USER_FILE_INSTALL_DIR)/kernel_modules

$(warning $(OUT_DRIVER_DIR))
$(warning $(LOCAL_DRIVER_DIR))

$(LOCAL_DRIVER_TARGET):$(BUILD_DIR_MAKEFILE)
	@mkdir -p $(LOCAL_DRIVER_INSTALL_PATH)
	@make -C $(KERNEL_BUILD_OUTPATH) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) ARCH=$(ARCH)  O=$(KERNEL_BUILD_OUTPATH) src=$(LOCAL_DRIVER_DIR) M=$(OUT_DRIVER_DIR) modules
	@cp "$(OUT_DRIVER_DIR)/$(LOCAL_DRIVER_TARGET).ko" $(LOCAL_DRIVER_INSTALL_PATH) 


$(BUILD_DIR_MAKEFILE):$(OUT_DRIVER_DIR)
	@touch "$@"

$(OUT_DRIVER_DIR):
	@mkdir -p "$@"
	
#make -C $(KERNEL_BUILD_OUTPATH) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) ARCH=$(ARCH)  O=$(KERNEL_BUILD_OUTPATH) M=$(OUT_DRIVER_DIR) INSTALL_MOD_DIR=$(USER_FILE_INSTALL_DIR) modules_install
