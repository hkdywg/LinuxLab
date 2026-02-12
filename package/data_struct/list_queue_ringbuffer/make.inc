##
## make.inc
##
## History:
##    2025/03/14 - [yinwg] Created file
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

LOCAL_TARGET	:= test_case_list
LOCAL_SRCS		:= $(LOCAL_PATH)/test_case_list.c
LOCAL_CFLAGS	:= -Wall -g

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
