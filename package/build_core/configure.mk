##
## build_core/configure.mk
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

MAKEFILE_V := 
MAKE_PARA := -s

###################################################
# TOOLCHAIN CONFIG
###################################################

ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"arm-linux-gnueabi")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABI_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabi-
ARCH := arm
CONFIGURE_FLAGS := --target=arm-linux-gnueabi --host=arm-linux-gnueabi --build=x86_64-linux
endif
ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"arm-linux-gnueabihf")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABIHF_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabihf-
ARCH := arm
CONFIGURE_FLAGS := --target=arm-linux-gnueabihf --host=arm-linux-gnueabihf --build=x86_64-linux
endif
ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"aarch64-linux-gnu")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_AARCH64_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/aarch64-linux-gnu-
ARCH := arm64
CONFIGURE_FLAGS := --target=aarch64-linux-gnu --host=aarch64-linux-gnu --build=x86_64-linux
endif

CC 		:= $(CROSS_COMPILE_CONFIG)gcc
CXX 	:= $(CROSS_COMPILE_CONFIG)g++
AS 		:= $(CROSS_COMPILE_CONFIG)as
LD 		:= $(CROSS_COMPILE_CONFIG)ld
STRIP 	:= $(CROSS_COMPILE_CONFIG)strip
OBJCOPY := $(CROSS_COMPILE_CONFIG)objcopy
OBJDUMP := $(CROSS_COMPILE_CONFIG)objdumo
AR 		:= $(CROSS_COMPILE_CONFIG)ar
NM 		:= $(CROSS_COMPILE_CONFIG)nm


HOST_DIR 		:= $(srctree)/out/toolchain/$(TOOL_CHAIN_NAME)
LOCAL_CFLAGS 	:= -I$(HOST_DIR)/include
LOCAL_LDFLAGS 	:= -L$(HOST_DIR)/lib -WL,-rpath,$(HOST_DIR)/lib

ifndef CC
$(error Can not find cross compile toolchain, please source ENV File)
endif


###################################################
# BUILD COMMAND
###################################################

CLEAR_VARS := $(PACKAGE_BUILD_TOPDIR)/build_core/clear_vars.mk
BUILD_APP := $(PACKAGE_BUILD_TOPDIR)/build_core/build_app.mk
BUILD_DRIVER := $(PACKAGE_BUILD_TOPDIR)/build_core/build_driver.mk

###################################################
# BUILD OUT DIRECTORY
###################################################
PACKAGE_WORKSPACE 			:= $(srctree)/workspace/package
PACKAGE_BUILD_OUTPATH 		:= $(PACKAGE_WORKSPACE)/build_out

export PACKAGE_BUILD_OUTPATH


