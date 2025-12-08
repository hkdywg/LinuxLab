##
## make.inc
##
## History:
##    2025/12/08 - [yinwg] Created file
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

LOCAL_TARGET	:= test_asm_ldr
LOCAL_SRCS		:= $(wildcard $(LOCAL_PATH)/src/*.c)
LOCAL_SRCS		+= $(wildcard $(LOCAL_PATH)/src/*.S)

LOCAL_CFLAGS	:=  -g -Wall -MMD -nostdlib -nostdinc -I$(LOCAL_PATH)/include
LOCAL_AFLAGS    := -MMD -I$(LOCAL_PATH)/include
LOCAL_LDFLAGS   := -T $(LOCAL_PATH)/src/linker.ld -e _start


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
######################################################################################

LOCAL_TARGET_RUN := $(LOCAL_TARGET)_run
QEMU_FLAGS := -M virt -cpu cortex-a72 -nographic -kernel

$(LOCAL_TARGET_RUN): $(LOCAL_MODULE)
	@echo "running $< with qemu"
	qemu-system-aarch64 $(QEMU_FLAGS) $< -S -s

$(call add-target-into-build, $(LOCAL_TARGET_RUN))

######################################################################################
LOCAL_TARGET_GDB := $(LOCAL_TARGET)_gdb

$(LOCAL_TARGET_GDB): $(LOCAL_MODULE)
	@echo "use gdb connect to $<"
	gdb-multiarch -tui $<
