#!/usr/bin/make -f

## BUILD PATH CONFIG
CURRENT_PATH=$(shell pwd)
BUILD_CORE_PATH := $(CURRENT_PATH)/core
GPT_CONFIG_PATH := $(BUILD_CORE_PATH)/config
BUILD_OUT_PATH := $(CURRENT_PATH)/out
TEMP_MOUNT_PATH := $(BUILD_OUT_PATH)/mount_path
PREBUILD_ROOTFS_PATH := $(CURRENT_PATH)/rootfs


# BUILD FILE CONFIG
GPT_PARTITION_TABLE := gpt_table
TEMP_ROOTFS_IMG := tmpfs.img
TARGET := mk_rootfs.img

# 
START_SECTOR := 2048

all:check_outpath $(TARGET)


define gen_gpt_partition
	echo "start generate gpt partition table"
	@python $(BUILD_CORE_PATH)/gen_gpt.py  $(GPT_CONFIG_PATH)/gpt_partition.conf $(BUILD_OUT_PATH)/$(GPT_PARTITION_TABLE)
endef

define gen_rootfs_img
	echo "start generate rootfs img"
	cd $(PREBUILD_ROOTFS_PATH); \
	prebuild_rootfs_cap=`du -sh | awk 'sub(/M/,"",$$1) {print $$1}'`; \
	echo "rootfs size = $$prebuild_rootfs_cap"; \
	rootfs_size=$$(($$prebuild_rootfs_cap+256)); \
	cd $(BUILD_OUT_PATH); \
	dd if=$(GPT_PARTITION_TABLE) of=$(TARGET) bs=512; \
	dd if=/dev/zero of=$(TEMP_ROOTFS_IMG) bs=1M count=$$rootfs_size; \
	mkfs.ext4 $(TEMP_ROOTFS_IMG); \
	fsck.ext4 $(TEMP_ROOTFS_IMG); \
	resize2fs $(TEMP_ROOTFS_IMG); \
	dd if=$(TEMP_ROOTFS_IMG) of=$(TARGET) seek=$(START_SECTOR) bs=512; \
	offset_mount=$$(($(START_SECTOR)*512)); \
	[ ! -d  $(TEMP_MOUNT_PATH) ] && mkdir -p $(TEMP_MOUNT_PATH); \
	sudo mount -o loop,offset=$$offset_mount $(TARGET) $(TEMP_MOUNT_PATH); \
	sudo cp -dpRf $(PREBUILD_ROOTFS_PATH)/* $(TEMP_MOUNT_PATH); \
	sync; \
	sudo umount $(TEMP_MOUNT_PATH)
endef

define check_permissions
	@user=`id -u`; \
	if [ "$$user" != "0" ]; then \
		echo "make rootfs must be root/sudo"; \
		exit -1;\
	fi
endef

check_outpath:
	@if [ ! -d $(BUILD_OUT_PATH) ]; then \
		mkdir -p $(TEMP_MOUNT_PATH); \
	fi


$(TARGET):
	@$(call gen_gpt_partition)
	@$(call gen_rootfs_img)

clean:
	@rm -rf $(BUILD_OUT_PATH)

FORCE:


