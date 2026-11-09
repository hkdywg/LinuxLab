##
## benchmark/bandwidth/make.inc
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

LOCAL_TARGET	:= bandwidth
LOCAL_SRCS	:= $(LOCAL_PATH)/routinesARM64.S
LOCAL_SRCS	+= $(LOCAL_PATH)/main.c $(LOCAL_PATH)/BMP.c
LOCAL_CFLAGS	:= -O3

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
