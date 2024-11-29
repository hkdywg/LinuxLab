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

OUT_DRIVER_DIR := $(patsubst $(PACKAGE_BUILD_TOPDIR)/%,$(KERNEL_BUILD_OUTPATH)/driver_modules/%,$(LOCAL_PATH))
BUILD_DIR_MAKEFILE := $(strip $(OUT_DRIVER_DIR)/Makefile)
LOCAL_DRIVER_INSTALL_PATH := $(USER_FILE_INSTALL_DIR)/kernel_modules
LOCAL_DRIVER_MODULE := $(patsubst $(PACKAGE_BUILD_TOPDIR)/%,$(KERNEL_BUILD_OUTPATH)/driver_modules/%,$(LOCAL_PATH)/$(LOCAL_DRIVER_TARGET))

$(LOCAL_DRIVER_MODULE):$(BUILD_DIR_MAKEFILE) $(LOCAL_PATH) $(OUT_DRIVER_DIR) 
	@echo "Start Build $@"
	@mkdir -p $(LOCAL_DRIVER_INSTALL_PATH)
	@make -C $(KERNEL_BUILD_OUTPATH) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) ARCH=$(ARCH)  O=$(KERNEL_BUILD_OUTPATH) src=$(word 2, $^) M=$(word 3, $^) modules
	@cp "$(OUT_DRIVER_DIR)/$(LOCAL_DRIVER_TARGET).ko" $(LOCAL_DRIVER_INSTALL_PATH) 


$(BUILD_DIR_MAKEFILE):$(OUT_DRIVER_DIR)
	touch "$@"

$(OUT_DRIVER_DIR):
	@mkdir -p "$@"
	
