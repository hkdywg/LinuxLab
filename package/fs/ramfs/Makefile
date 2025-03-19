##
## Makefile
##
## Brief: Makefie for build simple tiny filesystem
##
## History:
##    2024/12/02 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

obj-m := tmp_ramfs.o 
tmp_ramfs-m += inode.o
tmp_ramfs-m += file.o

LOCAL_DRIVER_TARGET := tmp_ramfs

include $(BUILD_DRIVER)

$(LOCAL_DRIVER_TARGET): $(LOCAL_DRIVER_MODULE)
	@echo "--Build $@ done"

$(call add-target-into-build, $(LOCAL_DRIVER_TARGET))
