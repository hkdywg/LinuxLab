LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

drm_debug-m := drm_debugfs.o drm_debug_pattern.o drm_debug_direct_show.o drm_debug_modeset.o
obj-m := drm_debug.o

LOCAL_DRIVER_TARGET := drm_debug

include $(BUILD_DRIVER)

$(LOCAL_DRIVER_TARGET): $(LOCAL_DRIVER_MODULE)
	@echo "--Build $@ done"

$(call add-target-into-build, $(LOCAL_DRIVER_TARGET))
