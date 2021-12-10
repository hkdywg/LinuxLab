#
# Workspace Makefile, make kernel, uboot, busybox, qemu
# Author: hkdywg <hkdywg@163.com>
#
# Copyright (C) 2021 LinuxLab
#
#

top_dir := $(CURDIR)/../

BUILD_SCRIPTS := $(top_dir)/scripts/workspace/build.sh

PHONY : all

FORCE :

kernel_%: FORCE
	@echo "start build kernel"
	@$(BUILD_SCRIPTS) $(top_dir)X $@

uboot_%:
	@echo "start build uboot"
	@$(BUILD_SCRIPTS) $(top_dir)X $@


busybox_%:
	@echo "start build busybox"
	@$(BUILD_SCRIPTS) $(top_dir)X $@
	

