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

cpu_core=`cat /proc/cpuinfo | grep process | wc -l`

target=(${2//_/ })
master_target=${target[0]}
sub_target=${target[1]}

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

case ${master_target} in
    "kernel")
        build_kernel
        ;;
    "busybox")
        build_busybox
        ;;
esac
