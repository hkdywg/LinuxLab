##
## make.inc
##
## History:
##    2025/05/29 - [yinwg] Created file
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

LOCAL_TARGET	:= get_drm_fb
LOCAL_SRCS	:= $(LOCAL_PATH)/get_drm_fb.c 
LOCAL_CFLAGS	:= -Wall -g -I$(LOCAL_PATH)/include
LOCAL_LDFLAGS	:= -ldrm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###########################################################################################################

include $(CLEAR_VARS)

LOCAL_TARGET	:= set_drm_fb
LOCAL_SRCS	:= $(LOCAL_PATH)/set_drm_fb.c 
LOCAL_CFLAGS	:= -Wall -g -I$(LOCAL_PATH)/include
LOCAL_LDFLAGS	:= -ldrm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###########################################################################################################

include $(CLEAR_VARS)

LOCAL_TARGET	:= modetest
LOCAL_SRCS	:= $(LOCAL_PATH)/modetest.c 
LOCAL_CFLAGS	:= -Wall -g -I$(LOCAL_PATH)/include
LOCAL_LDFLAGS	:= -ldrm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))



