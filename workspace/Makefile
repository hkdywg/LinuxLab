#
# Workspace Makefile, make kernel, uboot, busybox, qemu
# Author: hkdywg <hkdywg@163.com>
#
# Copyright (C) 2021 LinuxLab
#
#

SUB_TARGET += kernel uboot busybox qemu

top_dir := $(CURDIR)/../

BUILD_SCRIPTS := $(top_dir)/scripts/workspace/build.sh

FORCE :

kernel_%: FORCE
	@echo "start build kernel"
	@$(BUILD_SCRIPTS) $(top_dir)X $@


