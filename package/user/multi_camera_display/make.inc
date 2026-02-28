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

ifeq ($(ENABLE_EGL_GST), true)

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

CC              += --sysroot=/opt/poky/2.4.3/sysroots/aarch64-poky-linux
CXX             += --sysroot=/opt/poky/2.4.3/sysroots/aarch64-poky-linux 
PKG_PATH        := /opt/poky/2.4.3/sysroots/aarch64-poky-linux/usr/lib/pkgconfig:/opt/poky/2.4.3/sysroots/aarch64-poky-linux/usr/share/pkgconfig
PKG_SYSROOT_DIR := /opt/poky/2.4.3/sysroots/aarch64-poky-linux

LOCAL_TARGET	:= egl_video_player
DRMPKG          := libdrm libkms
LOCAL_SRCS	    := $(LOCAL_PATH)/egl_gst_video_player.cpp $(LOCAL_PATH)/gst_discoverer.c 
LOCAL_CFLAGS	:= $(shell PKG_CONFIG_PATH=$(PKG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_SYSROOT_DIR) pkg-config --cflags-only-I $(DRMPKG))
LOCAL_CFLAGS    += $(shell PKG_CONFIG_PATH=$(PKG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_SYSROOT_DIR) pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-pbutils-1.0) 
LOCAL_LDFLAGS	:= $(shell PKG_CONFIG_PATH=$(PKG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_SYSROOT_DIR) pkg-config --libs-only-L $(DRMPKG))
LOCAL_LDFLAGS	+= $(shell PKG_CONFIG_PATH=$(PKG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_SYSROOT_DIR) pkg-config --libs-only-l $(DRMPKG))
LOCAL_LDFLAGS   += $(shell PKG_CONFIG_PATH=$(PKG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_SYSROOT_DIR) pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-pbutils-1.0)	
LOCAL_LDFLAGS   += $(shell PKG_CONFIG_PATH=$(PKG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_SYSROOT_DIR) pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-pbutils-1.0) 
LOCAL_LDFLAGS   += -lEGL -lGLESv2 -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
