#!/bin/bash

set -e

# Establish linaro GNU GCC.
#
# (C) 2021.11.30 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

ROOT=${1%X}
BUSYBOX_VERSION=${2%X}
BUSYBOX_SRC=${3%X}
BUSYBOX_GITHUB_SITE=${4%X}
BUSYBOX_WGET_SITE=${5%X}

OUTPUT=${ROOT}/output/busybox
WORKSPACE=${ROOT}/workspace

[ ! -d ${ROOT}/dl ] && mkdir -p ${ROOT}/dl
[ ! -d ${OUTPUT} ] && mkdir -p ${OUTPUT}
[ ! -d ${WORKSPACE} ] && mkdir -p ${WORKSPACE}

case ${BUSYBOX_SRC} in
    # Get from github
    1)
        arr=(${BUSYBOX_VERSION//./ })
        major=${arr[0]}
        minor=${arr[1]}
        sub=${arr[2]}
        if [ ! -d ${ROOT}/dl/busybox ]; then
            cd ${ROOT}/dl
            git clone ${BUSYBOX_GITHUB_SITE} 
        else
            git pull
        fi
        cp -rfa ${ROOT}/dl/busybox ${OUTPUT}/busybox-${BUSYBOX_VERSION} 
        cd ${OUTPUT}/busybox-${BUSYBOX_VERSION}
        git checkout ${major}_${minor}_${sub} 
        echo ${BUSYBOX_VERSION} > ${OUTPUT}/busybox-${BUSYBOX_VERSION}/version
        rm ${ROOT}/dl/busybox -rf
        echo -e "\033[32m busybox download successed!! \033[0m"
        [ -L ${WORKSPACE}/busybox ] && rm ${WORKSPACE}/busybox
        ln -s ${OUTPUT}/busybox-${BUSYBOX_VERSION} ${WORKSPACE}/busybox  
        ;;

    2)
        BASE_NAME=busybox-${BUSYBOX_VERSION}.tar.bz2  
        BASE=busybox-${BUSYBOX_VERSION}
        if [ ! -f ${ROOT}/dl/${BASE_NAME} ]; then
            cd ${ROOT}/dl
            wget ${BUSYBOX_WGET_SITE}/${BASE_NAME}
        fi
        cd ${ROOT}/dl
        cp ${BASE_NAME} ${OUTPUT}
        cd ${OUTPUT}
        tar -xjf ${BASE_NAME}
        if [ $? -ne 0 ] ;then
            rm ${ROOT}/dl/${BASE_NAME}
            echo -e "\033[31m tar operation failed\033[0m"
            exit -1
        fi
        rm ${BASE_NAME}
        echo ${BUSYBOX_VERSION} > ${OUTPUT}/${BASE}/version
        echo -e "\033[32m busybox download successed!! \033[0m"
        [ -L ${WORKSPACE}/busybox ] && rm ${WORKSPACE}/busybox
        ln -s ${OUTPUT}/busybox-${BUSYBOX_VERSION} ${WORKSPACE}/busybox  
        ;;
esac


