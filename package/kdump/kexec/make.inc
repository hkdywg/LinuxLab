##
## make.inc
##
## History:
##    2024/11/08 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

KEXEC_VERSION 			:= 2.0.22
KEXEC_FILE				:= kexec-tools-$(KEXEC_VERSION).tar.xz
KEXEC_WGET_SITE			:= https://mirrors.edge.kernel.org/pub/linux/utils/kernel/kexec/$(KEXEC_FILE)
KEXEC_GITHUB_SITE		:= https://github.com/horms/kexec-tools.git
KEXEC_WORKSPACE 		:= $(srctree)/output/package/kdump/kexec
KEXEC_BUILD_OUTPATH	    := $(KEXEC_WORKSPACE)/build_out/

SUB_TARGET 				+= kexec kexec-download kexec-config kexec-pack kexec-build

# This is a -R option in MAKEFLAGS in the top-level Makefile
# The -R option affects the use of COMPILE.c variables in kexec compile
MAKEFLAGS := 

.PHONY: kexec-download kexec-check-dir

kexec-download: kexec-check-dir
	@if [ ! -f "$(KEXEC_WORKSPACE)/kexec-tools-$(KEXEC_VERSION).tar.xz" ]; then \
		echo "File does not exist, downloading..."; \
		wget $(KEXEC_WGET_SITE) -O $(KEXEC_WORKSPACE)/$(KEXEC_FILE); \
		tar -xf $(KEXEC_WORKSPACE)/$(KEXEC_FILE) -C $(KEXEC_WORKSPACE); \
	fi

kexec-config:
	@cd $(KEXEC_WORKSPACE)/kexec-tools-$(KEXEC_VERSION); \
	./configure $(CONFIGURE_FLAGS) CC=$(CC) LD=$(LD) STRIP=$(STRIP) --prefix=$(KEXEC_BUILD_OUTPATH) O=$(KEXEC_BUILD_OUTPATH); \
	cd -

kexec-build:
	cd $(KEXEC_WORKSPACE)/kexec-tools-$(KEXEC_VERSION); \
	make && make install ; \
	cd - ; \
	cat $(srctree)/package/kdump/kexec/config >> $(KERNEL_KCONFIG_FILE); \
	make kernel-build

kexec-pack:
	mkdir -p $(USER_FILE_INSTALL_DIR)
	sudo cp -rf $(KEXEC_BUILD_OUTPATH)/*  $(USER_FILE_INSTALL_DIR)/ 
	sudo cp $(KERNEL_IMAGE) $(USER_FILE_INSTALL_DIR)/


kexec-check-dir:
	@if [ ! -d "$(KEXEC_WORKSPACE)" ];then \
		mkdir -p $(KEXEC_WORKSPACE); \
	fi

kexec: kexec-download kexec-config kexec-build kexec-pack

