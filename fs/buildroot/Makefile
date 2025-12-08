########################################
#
# busxbox
#
########################################

BUILDROOT_VERSION 		:= $(CONFIG_BUILDROOT_VERSION)
BUILDROOT_SRC 			:= $(CONFIG_BUILDROOT_SRC)
BUILDROOT_GITHUB_SITE 	:= https://gitee.com/mirrors/buildroot.git
BUILDROOT_WGET_SITE 	:= https://buildroot.org/downloads/
BUILDROOT_SCRIPTS 		:= $(srctree)/scripts/rootfs/buildroot.sh

BUILDROOT_WORKSPACE 	:= $(srctree)/workspace/buildroot
BUILDROOT_BUILD_OUTPATH := $(BUILDROOT_WORKSPACE)/build_out
BUILDROOT_KCONFIG_FILE 	:= $(BUILDROOT_BUILD_OUTPATH)/.config

MKROOTFS_SCRIPTS 		:= $(srctree)/scripts/rootfs/rootfs.sh
ROOTFS_WORKSPACE 		:= $(srctree)/workspace/rootfs
ROOTFS_NAME 			:= rootfs
ROOTFS_VERSION 			:= 1.0

SUB_TARGET 				+= buildroot-download buildroot-defconfig buildroot-build rootfs-mk

BUILDROOT_SH 	= $(CONFIG_SHELL) $(BUILDROOT_SCRIPTS) $(1)X $(BUILDROOT_VERSION)X $(BUILDROOT_SRC)X	\
				$(BUILDROOT_GITHUB_SITE)X $(BUILDROOT_WGET_SITE)X 

buildroot-download:
	$(Q)$(call BUILDROOT_SH, $(srctree))

buildroot-defconfig:
	@mkdir -p ${BUILDROOT_BUILD_OUTPATH}
	@echo $(BUILDROOT_WORKSPACE)
	@echo $(CONFIG_BUILDROOT_DEFCONFIG)
	@echo $(BUILDROOT_BUILD_OUTPATH)
	${Q}${MAKE} -C $(BUILDROOT_WORKSPACE) $(CONFIG_BUILDROOT_DEFCONFIG) O=$(BUILDROOT_BUILD_OUTPATH) VERBOSE=1

buildroot-menuconfig:
	@mkdir -p ${BUILDROOT_BUILD_OUTPATH}
	${Q}${MAKE} -C $(BUILDROOT_WORKSPACE) ARCH=$(ARCH) menuconfig O=$(BUILDROOT_BUILD_OUTPATH) 	

buildroot-clean:
	@${Q}${MAKE} -C $(BUILDROOT_WORKSPACE) clean O=$(BUILDROOT_BUILD_OUTPATH)

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

buildroot-build:export TARGET_OUTPUT_DIR=${BUILDROOT_BUILD_OUTPATH}
buildroot-build:export BR_DEFCONFIG=${CONFIG_BUILDROOT_DEFCONFIG}
buildroot-build:export ARCH=arm64
buildroot-build:
	@mkdir -p $(BUILDROOT_BUILD_OUTPATH)
	${Q}$(MAKE) -C ${BUILDROOT_WORKSPACE} ${BR_DEFCONFIG} BR2_TOOLCHAIN_EXTERNAL_PATH=${CROSS_COMPILE_CONFIG} O=${BUILDROOT_BUILD_OUTPATH}
	${Q}${BUILDROOT_WORKSPACE}/utils/brmake -j${CPU_CORE} -C ${BUILDROOT_WORKSPACE} O=${BUILDROOT_BUILD_OUTPATH}

$(BUILDROOT_KCONFIG_FILE):
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** $(BUILDROOT_KCONFIG_FILE) is not found!\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** please run some configurator(e.g. "make buildroot-menuconfig")\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@/bin/false


MKROOTFS_SH = $(CONFIG_SHELL) $(MKROOTFS_SCRIPTS) $(ROOTFS_WORKSPACE)X $(ROOTFS_NAME)X $(ROOTFS_VERSION)X $(CONFIG_ROOTFS_SIZE)X \
			  	$(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/$(COMPILE_TYPE)X	$(BUILDROOT_BUILD_OUTPATH)X


rootfs-mk:
	$(Q)$(call MKROOTFS_SH)



