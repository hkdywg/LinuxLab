
LOCAL_DRIVER_DIR := $(call my-dir)

include $(CLEAR_VARS)


obj-m = simple_kdump.o

LOCAL_DRIVER_TARGET := simple_kdump

SUB_TARGET += $(LOCAL_DRIVER_TARGET)

include $(BUILD_DRIVER)
$(warning $(MAKEFILE_LIST))


#MODULE_SRC := $(call my-dir)

#simple_kdump:
#	make -C $(KERNEL_BUILD_OUTPATH)  CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) ARCH=$(ARCH)  O=$(KERNEL_BUILD_OUTPATH) M=$(MODULE_SRC) modules

