##
## Makefile
##
## Brief: Makefie for build vkms driver
##
## History:
##    2026/02/25 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

obj-m = vkms_drv.o

LOCAL_DRIVER_TARGET := vkms_drv

include $(BUILD_DRIVER)

$(LOCAL_DRIVER_TARGET): $(LOCAL_DRIVER_MODULE)
	@echo "--Build $@ done"

$(call add-target-into-build, $(LOCAL_DRIVER_TARGET))
