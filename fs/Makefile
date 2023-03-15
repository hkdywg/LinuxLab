########################################
#
# busxbox
#
########################################

BUSYBOX_VERSION 		:= $(CONFIG_BUSYBOX_VERSION)
BUSYBOX_SRC 			:= $(CONFIG_BUSYBOX_SRC)
BUSYBOX_GITHUB_SITE 	:= https://gitee.com/linux-os/busybox.git
BUSYBOX_WGET_SITE 		:= https://busybox.net/downloads/
BUSYBOX_SCRIPTS 		:= $(srctree)/scripts/rootfs/busybox.sh

BUSYBOX_WORKSPACE 		:= $(srctree)/workspace/busybox
BUSYBOX_BUILD_OUTPATH 	:= $(BUSYBOX_WORKSPACE)/build_out
BUSYBOX_KCONFIG_FILE 	:= $(BUSYBOX_BUILD_OUTPATH)/.config

MKROOTFS_SCRIPTS 		:= $(srctree)/scripts/rootfs/rootfs.sh
ROOTFS_WORKSPACE 		:= $(srctree)/workspace/rootfs
ROOTFS_NAME 			:= rootfs
ROOTFS_VERSION 			:= 1.0

SUB_TARGET 				+= busybox_download

BUSYBOX_SH 	= $(CONFIG_SHELL) $(BUSYBOX_SCRIPTS) $(1)X $(BUSYBOX_VERSION)X $(BUSYBOX_SRC)X	\
				$(BUSYBOX_GITHUB_SITE)X $(BUSYBOX_WGET_SITE)X 

busybox_download:
	$(Q)$(call BUSYBOX_SH, $(srctree))

busybox-defconfig:
	@mkdir -p ${BUSYBOX_BUILD_OUTPATH}
	${Q}${MAKE} -C $(BUSYBOX_WORKSPACE) $(CONFIG_BUSYBOX_DEFCONFIG) O=$(BUSYBOX_BUILD_OUTPATH)

busybox-menuconfig:
	@mkdir -p ${BUSYBOX_BUILD_OUTPATH}
	${Q}${MAKE} -C $(BUSYBOX_WORKSPACE) ARCH=$(ARCH) menuconfig O=$(BUSYBOX_BUILD_OUTPATH) 	

busybox-clean:
	@${Q}${MAKE} -C $(BUSYBOX_WORKSPACE) clean O=$(BUSYBOX_BUILD_OUTPATH)

CPU_CORE=$(shell cat /proc/cpuinfo | grep processor | wc -l)

COMPILE_TYPE := $(subst ",,$(CONFIG_LINUX_KERNEL_CROSS_COMPILE))

ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"arm-linux-gnueabi")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABI_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabi-
endif
ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"arm-linux-gnueabihf")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABIHF_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabihf-
endif
ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"aarch64-linux-gnu")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_AARCH64_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/aarch64-linux-gnu-
endif

busybox-build:$(BUSYBOX_KCONFIG_FILE)
	@mkdir -p $(BUSYBOX_BUILD_OUTPATH)
	${Q}${MAKE} -C $(BUSYBOX_WORKSPACE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) O=$(BUSYBOX_BUILD_OUTPATH) -j$(CPU_CORE)
	${Q}${MAKE} -C $(BUSYBOX_WORKSPACE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) O=$(BUSYBOX_BUILD_OUTPATH) install -j$(CPU_CORE)

$(BUSYBOX_KCONFIG_FILE):
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** $(BUSYBOX_KCONFIG_FILE) is not found!\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** please run some configurator(e.g. "make busybox-menuconfig")\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@/bin/false


MKROOTFS_SH = $(CONFIG_SHELL) $(MKROOTFS_SCRIPTS) $(ROOTFS_WORKSPACE)X $(ROOTFS_NAME)X $(ROOTFS_VERSION)X $(CONFIG_ROOTFS_SIZE)X \
			  	$(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/$(COMPILE_TYPE)X	$(BUSYBOX_BUILD_OUTPATH)X


rootfs-mk:
	$(Q)$(call MKROOTFS_SH)



