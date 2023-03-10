##############################################################################
#
# uboot
#
##############################################################################

UBOOT_NAME 				:= u-boot
UBOOT_VERSION 			:= $(CONFIG_UBOOT_VERSION) 
UBOOT_SITE 				:= $(CONFIG_UBOOT_WGET_SITE)
UBOOT_GITHUB 			:= $(CONFIG_UBOOT_GITHUB_SITE)
UBOOT_CROSS_COMPILE 	:= $(CONFIG_UBOOT_CROSS_COMPILE)
UBOOT_PATCH 			:= $(srctree)/boot/u-boot/patch
UBOOT_OUT 				:=
UBOOT_DOWNLOAD_SCRIPTS	:= $(srctree)/scripts/boot/uboot_download.sh

UBOOT_BUILD_OUTPATH 	:= $(srctree)/workspace/uboot/build_out
UBOOT_KCONFIG_FILE 		:= $(UBOOT_BUILD_OUTPATH)/.config

SUB_TARGET 	+= uboot-download uboot-build 

UBOOT_DOWNLOAD_SH = $(CONFIG_SHELL) $(UBOOT_DOWNLOAD_SCRIPTS) \
		   ${1}X ${UBOOT_NAME}X ${UBOOT_VERSION} ${UBOOT_SITE}X \
		   ${UBOOT_GITHUB}X ${CONFIG_UBOOT_SRC}X ${CONFIG_ARCH}X ${UBOOT_CROSS_COMPILE}X 


uboot-download: 
	${Q}$(call UBOOT_DOWNLOAD_SH, ${srctree})

uboot-menuconfig:
	@mkdir -p $(UBOOT_BUILD_OUTPATH)
	${Q}${MAKE} -C $(srctree)/workspace/uboot menuconfig O=$(UBOOT_BUILD_OUTPATH) 

uboot-defconfig:
	@mkdir -p $(UBOOT_BUILD_OUTPATH)
	${Q}${MAKE} -C $(srctree)/workspace/uboot $(CONFIG_UBOOT_DEFCONFIG) O=$(UBOOT_BUILD_OUTPATH)

uboot-clean:
	cd $(srctree)/workspace/uboot
	@make clean O=$(UBOOT_BUILD_OUTPATH)


CPU_CORE=$(shell cat /proc/cpuinfo | grep processor | wc -l)

ifeq ($(CONFIG_UBOOT_CROSS_COMPILE),"arm-linux-gnueabi")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABI_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabi-
endif
ifeq ($(CONFIG_UBOOT_CROSS_COMPILE),"arm-linux-gnueabihf")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABIHF_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabihf-
endif
ifeq ($(CONFIG_UBOOT_CROSS_COMPILE),"aarch64-linux-gnu")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_AARCH64_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/aarch64-linux-gnu-
endif

uboot-build:$(UBOOT_KCONFIG_FILE)
	@mkdir -p $(UBOOT_BUILD_OUTPATH)
	${Q}${MAKE} -C $(srctree)/workspace/uboot ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) O=$(UBOOT_BUILD_OUTPATH) -j$(CPU_CORE)
		

$(UBOOT_KCONFIG_FILE):
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** $(UBOOT_KCONFIG_FILE) is not found!\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** please run some configurator(e.g. "make uboot-menuconfig")\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@/bin/false

