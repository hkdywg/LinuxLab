#!/bin/bash

# Establish linaro GNU GCC.
#
# (C) 2021.12.09 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

ROOT_PATH=${1%X}
TOOLCHAIN_PATH=${ROOT_PATH}/output/toolchain

ROOTFS_SIZE=150
ROOTFS_TYPE=ext4
ROOTFS_TOOL=mkfs.${ROOTFS_TYPE}
RAM_SIZE=512

target=(${2//_/ })
master_target=${target[0]}
sub_target=${target[1]}

cpu_core=`cat /proc/cpuinfo | grep process | wc -l`

check_root()
{
    current_per=`echo ${EUID}`
    return ${current_per}
}

build_uboot()
{
    BUILD_PATH=build_output
    cd ${ROOT_PATH}/workspace/uboot
    [ ! -d ${BUILD_PATH} ] && mkdir -p ${BUILD_PATH}
    case ${sub_target} in
        "defconfig")
	        make O=${BUILD_PATH} ARCH=arm defconfig
            ;;
        "clean")
	        make O=${BUILD_PATH} clean
            ;;
        "image")
            make O=${BUILD_PATH} ARCH=arm CROSS_COMPILE=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi- -j${cpu_core}
            ;;
        *)
            echo -e "\033[31m target ${sub_target} is not exited!!! \033[0m"

    esac
     
}

build_kernel()
{
    BUILD_PATH=build_output
    cd ${ROOT_PATH}/workspace/kernel
    [ ! -d ${BUILD_PATH} ] && mkdir -p ${BUILD_PATH}
    case ${sub_target} in
        "defconfig")
	        make O=${BUILD_PATH} ARCH=arm64 defconfig
            ;;
        "clean")
	        make O=${BUILD_PATH} clean
            ;;
        "image")
            make O=${BUILD_PATH} ARCH=arm64 CROSS_COMPILE=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- Image -j${cpu_core}
            ;;
        *)
            echo -e "\033[31m target ${sub_target} is not exited!!! \033[0m"

    esac
     
}

build_busybox()
{
    BUILD_PATH=build_output
    cd ${ROOT_PATH}/workspace/busybox
    [ ! -d ${BUILD_PATH} ] && mkdir -p ${BUILD_PATH}
    case ${sub_target} in
        "defconfig")
	        make O=${BUILD_PATH} ARCH=arm64 defconfig
            ;;
        "clean")
	        make O=${BUILD_PATH} clean
            ;;
        "install")
            make O=${BUILD_PATH} ARCH=arm64 CROSS_COMPILE=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- -j${cpu_core}
            make O=${BUILD_PATH} ARCH=arm64 CROSS_COMPILE=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- install -j${cpu_core}
            ;;
        *)
            echo -e "\033[31m target ${sub_target} is not exited!!! \033[0m"
    esac
}

build_rootfs()
{
    check_root
    if [ $? -ne 0 ];then
        echo -e "\033[31m this operation need root permission!!! \033[0m"
        exit
    else
        ROOTFS_PATH=${ROOT_PATH}/workspace/rootfs
        [ ! -d ${ROOTFS_PATH} ] && mkdir -p ${ROOTFS_PATH}
        cp ${ROOT_PATH}/workspace/busybox/build_output/_install/ ${ROOTFS_PATH}/${ROOTFS_TYPE} -raf 
        sudo chown root.root ${ROOTFS_PATH}/${ROOTFS_TYPE} -R
        dd if=/dev/zero of=${ROOTFS_PATH}/ramdisk bs=1M count=${ROOTFS_SIZE}
        ${ROOTFS_TOOL} -E lazy_itable_init=1,lazy_journal_init=1 -F ${ROOTFS_PATH}/ramdisk
        mkdir -p ${ROOTFS_PATH}/tmpfs
        sudo mount -t ${ROOTFS_TYPE} ${ROOTFS_PATH}/ramdisk ${ROOTFS_PATH}/tmpfs -o loop
        sudo cp -raf ${ROOTFS_PATH}/${ROOTFS_TYPE}/* ${ROOTFS_PATH}/tmpfs
        sudo umount ${ROOTFS_PATH}/tmpfs
        mv ${ROOTFS_PATH}/ramdisk ${ROOTFS_PATH}/rootfs.img
    fi
}

build_qemu()
{
    QEMU_PATH=${ROOT_PATH}/workspace/qemu
    EMULATER=aarch64-softmmu,arm-linux-user
    CONFIG="--enable-kvm --enable-virtfs"

    QEMU=${ROOT_PATH}/workspace/qemu/aarch64-softmmu/qemu-system-aarch64
    CMDLINE="earlycon root=/dev/vda rw rootfstype=${ROOTFS_TYPE} console=ttyAMA0 init=/linuxrc loglevel=8"

    case ${sub_target} in
        "build")
            cd ${QEMU_PATH}
            ./configure --target-list=${EMULATER} ${CONFIG}
            make -j${cpu_core}
            ;;
        "run")
            sudo ${QEMU} \
                -M virt \
                -m ${RAM_SIZE}M \
                -cpu cortex-a53 \
                -smp 2 \
                -kernel ${ROOT_PATH}/workspace/kernel/build_output/arch/arm64/boot/Image \
                -device virtio-blk-device,drive=hd0 \
                -drive if=none,file=${ROOT_PATH}/workspace/rootfs/BiscuitOS.img,format=raw,id=hd0 \
                -nographic \
                -append "${CMDLINE}"
            ;;
    esac
}

case ${master_target} in
    "uboot")
        build_uboot
        ;;
    "kernel")
        build_kernel
        ;;
    "busybox")
        build_busybox
        ;;
    "rootfs")
        build_rootfs
        ;;
    "qemu")
        build_qemu
        ;;
esac
