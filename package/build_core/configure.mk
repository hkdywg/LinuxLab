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

ifeq ($(CONFIG_KERNEL_CROSS_COMPILE),"arm-linux-gnueabi")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABI_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabi-
ARCH := arm
endif
ifeq ($(CONFIG_KERNEL_CROSS_COMPILE),"arm-linux-gnueabihf")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABIHF_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabihf-
ARCH := arm
endif
ifeq ($(CONFIG_KERNEL_CROSS_COMPILE),"aarch64-linux-gnu")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_AARCH64_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/aarch64-linux-gnu-
ARCH := arm64
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

ifndef CC
$(error Can not find cross compile toolchain, please source ENV File)
endif


###################################################
# BUILD COMMAND
###################################################

CLEAR_VARS := $(BUILD_TOPDIR)/build_core/clear_vars.mk
BUILD_APP := $(BUILD_TOPDIR)/build_core/build_app.mk

###################################################
# BUILD OUT DIRECTORY
###################################################
PACKAGE_WORKSPACE 			:= $(srctree)/workspace/kernel
PACKAGE_BUILD_OUTPATH 		:= $(PACKAGE_WORKSPACE)/build_out

export PACKAGE_BUILD_OUTPATH


