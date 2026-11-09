##
## benchmark/iozone/make.inc
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

LOCAL_TARGET	:= iozone
LOCAL_SRCS      := $(LOCAL_PATH)/iozone.c $(LOCAL_PATH)/libbif.c $(LOCAL_PATH)/libasync.c  
LOCAL_CFLAGS	:= -O3 -Dunix -DHAVE_ANSIC_C -DASYNC_IO -DHAVE_PREAD -DNAME='"linux-arm"' -DLINUX_ARM -DSHARED_MEM \
                    -Dlinux -D_LARGEFILE64_SOURCE -lrt -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

