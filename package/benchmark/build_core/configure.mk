##
## benchmark/build_core/configure.mk
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## Copyright (C) 2016, JiangSu New Vision Automotive CO.,LTD.
## 
## All rights reserved. This software is New Vision property. Duplication
## or disclosure without New Vision written authorization is prohibited.
##

MAKEFILE_V := 
MAKE_PARA := -s

###################################################
# TOOLCHAIN CONFIG
###################################################
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

BUILD_OUT_TOPDIR := $(BUILD_TOPDIR)/build_out

export BUILD_OUT_TOPDIR


