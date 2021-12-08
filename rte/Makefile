################################################
#
# Qemu
#
################################################

QEMU_VERSION 			:= $(CONFIG_QEMU_VERSION)
QEMU_SRC 				:= $(CONFIG_QEMU_SRC)
QEMU_WGET_SITE			:= https://download.qemu.org
QEMU_GITHUB_SITE		:= https://gitlab.com/qemu-project/qemu.git
QEMU_DOWNLOAD_SCRIPTS 	:= $(srctree)/scripts/qemu/qemu.sh

SUB_TARGET 				+= qemu_download

QEMU_SH 				= $(CONFIG_SHELL) $(QEMU_DOWNLOAD_SCRIPTS) $(1)X $(QEMU_VERSION)X $(QEMU_SRC)X \
						  $(QEMU_WGET_SITE)X $(QEMU_GITHUB_SITE)X 

$(warning $(QEMU_SH))
qemu_download:
	$(Q)$(call QEMU_SH, $(srctree))
#	$(QEMU_SH) $(srctree)
