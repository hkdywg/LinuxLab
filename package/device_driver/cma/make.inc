##
## make.inc
##
## History:
##    2024/12/11 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##
###########################################################################################################
#	build local linux application
###########################################################################################################

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_cma_base
LOCAL_SRCS	:= $(LOCAL_PATH)/test_cma_base.c
LOCAL_CFLAGS	:= -Wall -g
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###########################################################################################################
#	build local linux driver
###########################################################################################################
include $(LOCAL_PATH)/Makefile
