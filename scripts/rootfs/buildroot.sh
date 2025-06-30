#!/bin/bash

set -e

# Establish linaro GNU GCC.
#
# (C) 2025.06.30 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

ROOT=${1%X}
BUILDROOT_VERSION=${2%X}
BUILDROOT_SRC=${3%X}
BUILDROOT_GITHUB_SITE=${4%X}
BUILDROOT_WGET_SITE=${5%X}

OUTPUT=${ROOT}/output/buildroot
WORKSPACE=${ROOT}/workspace

[ ! -d ${ROOT}/dl ] && mkdir -p ${ROOT}/dl
[ ! -d ${OUTPUT} ] && mkdir -p ${OUTPUT}
[ ! -d ${WORKSPACE} ] && mkdir -p ${WORKSPACE}

case ${BUILDROOT_SRC} in
    # Get from github
    1)
        arr=(${BUILDROOT_VERSION//./ })
        major=${arr[0]}
        minor=${arr[1]}
        sub=${arr[2]}
        if [ ! -d ${ROOT}/dl/buildroot ]; then
            cd ${ROOT}/dl
            git clone ${BUILDROOT_GITHUB_SITE} 
        else
            git pull
        fi
        cp -rfa ${ROOT}/dl/buildroot ${OUTPUT}/buildroot-${BUILDROOT_VERSION} 
        cd ${OUTPUT}/buildroot-${BUILDROOT_VERSION}
        git checkout ${major}_${minor}_${sub} 
        echo ${BUILDROOT_VERSION} > ${OUTPUT}/buildroot-${BUILDROOT_VERSION}/version
        rm ${ROOT}/dl/buildroot -rf
        echo -e "\033[32m BUILDROOT download successed!! \033[0m"
        [ -L ${WORKSPACE}/buildroot ] && rm ${WORKSPACE}/buildroot
        ln -s ${OUTPUT}/buildroot-${BUILDROOT_VERSION} ${WORKSPACE}/buildroot
        ;;

    2)
        BASE_NAME=buildroot-${BUILDROOT_VERSION}.tar.gz
        BASE=buildroot-${BUILDROOT_VERSION}
        if [ ! -f ${ROOT}/dl/${BASE_NAME} ]; then
            cd ${ROOT}/dl
            wget ${BUILDROOT_WGET_SITE}/${BASE_NAME}
        fi
        cd ${ROOT}/dl
        cp ${BASE_NAME} ${OUTPUT}
        cd ${OUTPUT}
        tar -xf ${BASE_NAME}
        if [ $? -ne 0 ] ;then
            rm ${ROOT}/dl/${BASE_NAME}
            echo -e "\033[31m tar operation failed\033[0m"
            exit -1
        fi
        rm ${BASE_NAME}
        echo ${BUILDROOT_VERSION} > ${OUTPUT}/${BASE}/version
        echo -e "\033[32m buildroot download successed!! \033[0m"
        [ -L ${WORKSPACE}/buildroot ] && rm ${WORKSPACE}/buildroot
        ln -s ${OUTPUT}/buildroot-${BUILDROOT_VERSION} ${WORKSPACE}/buildroot
        ;;
esac

