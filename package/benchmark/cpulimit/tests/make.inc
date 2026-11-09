##
## benchmark/cpulimit/tests/make.inc
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

LOCAL_TARGET	:= busy
LOCAL_SRCS	:= $(LOCAL_PATH)/busy.c
LOCAL_CFLAGS	:= -Wall -g
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= process_iterator_test
LOCAL_SRCS	:= $(LOCAL_PATH)/process_iterator_test.c $(LOCAL_PATH)/../src/list.c \
	$(LOCAL_PATH)/../src/process_iterator.c $(LOCAL_PATH)/../src/process_group.c
LOCAL_CFLAGS	:= -Wall -g -I$(LOCAL_PATH)/../src
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


