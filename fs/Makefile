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

SUB_TARGET 				+= busybox_download

BUSYBOX_SH 	= $(CONFIG_SHELL) $(BUSYBOX_SCRIPTS) $(1)X $(BUSYBOX_VERSION)X $(BUSYBOX_SRC)X	\
				$(BUSYBOX_GITHUB_SITE)X $(BUSYBOX_WGET_SITE)X 

busybox_download:
	$(Q)$(call BUSYBOX_SH, $(srctree))
