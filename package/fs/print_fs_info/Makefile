##
## Makefile
##
## Brief: Makefie for build driver of print filesytem basic information
##
## History:
##    2025/04/01 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

obj-m := print_fs_info.o 

LOCAL_DRIVER_TARGET := print_fs_info

include $(BUILD_DRIVER)

$(LOCAL_DRIVER_TARGET): $(LOCAL_DRIVER_MODULE)
	@echo "--Build $@ done"

$(call add-target-into-build, $(LOCAL_DRIVER_TARGET))
