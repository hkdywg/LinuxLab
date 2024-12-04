##
## Makefile
##
## Brief: Makefie for build oops sample
##
## History:
##    2024/12/04 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

obj-m = oops.o

LOCAL_DRIVER_TARGET := oops

include $(BUILD_DRIVER)

$(LOCAL_DRIVER_TARGET): $(LOCAL_DRIVER_MODULE)
	@echo "--Build $@ done"

$(call add-target-into-build, $(LOCAL_DRIVER_TARGET))
