################################################
#
# Qemu
#
################################################

QEMU_VERSION 			:= $(CONFIG_QEMU_VERSION)
QEMU_SRC 				:= $(CONFIG_QEMU_SRC)
QEMU_WGET_SITE			:= https://download.qemu.org
QEMU_GITHUB_SITE		:= https://gitlab.com/qemu-project/qemu.git
QEMU_DOWNLOAD_SCRIPTS 	:= $(srctree)/scripts/qemu/qemu_download.sh
QEMU_CONFIG_SCRIPTS 	:= $(srctree)/scripts/qemu/qemu_config.sh
GEN_RUNQEMU_SCRIPTS 	:= $(srctree)/scripts/qemu/gen_runqemu.sh

SUB_TARGET 				+= qemu_download qemu_config

QEMU_DOWNLOAD_SH 	    = $(CONFIG_SHELL) $(QEMU_DOWNLOAD_SCRIPTS) $(1)X $(QEMU_VERSION)X $(QEMU_SRC)X \
						  $(QEMU_WGET_SITE)X $(QEMU_GITHUB_SITE)X 

QEMU_CONFIG_SH 			= $(CONFIG_SHELL) $(QEMU_CONFIG_SCRIPTS) $(1)X $(QEMU_VERSION)X $(QEMU_SRC)X $(CONFIG_ARCH)X

GEN_RUNQEMU_SH 			= $(CONFIG_SHELL) $(GEN_RUNQEMU_SCRIPTS) $(1)X $(CONFIG_ARCH)X $(CONFIG_RAM_SIZE)X $(CONFIG_CPU_CORE)X $(CONFIG_CPU_CORE_NUM)X

qemu_download:
	$(Q)$(call QEMU_DOWNLOAD_SH, $(srctree))

qemu_config:
	$(Q)$(call QEMU_CONFIG_SH, $(srctree))

gen-runqemu:
	$(Q)$(call GEN_RUNQEMU_SH, $(srctree))
