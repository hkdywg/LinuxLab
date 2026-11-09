##
## benchmark/cpulimit/src/make.inc
##
## History:
##    2024/04/15 - [yinwg] Created file
##
## Copyright (C) 2016, JiangSu New Vision Automotive CO.,LTD.
## 
## All rights reserved. This software is New Vision property. Duplication
## or disclosure without New Vision written authorization is prohibited.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= cpulimit
LOCAL_SRCS	:= $(LOCAL_PATH)/process_iterator.c $(LOCAL_PATH)/process_group.c \
	$(LOCAL_PATH)/list.c $(LOCAL_PATH)/cpulimit.c
LOCAL_CFLAGS	:= -Wall -g -D_GNU_SOURCE

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

