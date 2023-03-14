#!/bin/bash

set -e

# Establish uboot source code.
#
# (C) 2021.11.07 hkdywg 
#

ROOT=${1%X}
UBOOT_NAME=${2%X}
UBOOT_VERSION=${3%X}
UBOOT_SITE=${4%X}
UBOOT_GITHUB=${5%X}
UBOOT_SRC=${6%X}
OUTPUT=${ROOT}/output/uboot
ARCH=${7%X}
UBOOT_TOOLS=${8%X}

WORKSPACE=${ROOT}/workspace

[ ! -d  ${ROOT}/dl/${UBOOT_NAME} ] && mkdir -p ${ROOT}/dl
[ ! -d ${WORKSPACE} ] && mkdir -p ${WORKSPACE}
[ ! -d ${OUTPUT} ] && mkdir -p ${OUTPUT}

## Get from github
if [ ${UBOOT_SRC} = "1" ];then
    if [ ! -d ${ROOT}/dl/${UBOOT_NAME} ];then
        mkdir -p ${ROOT}/dl
        cd ${ROOT}/dl
        git clone ${UBOOT_GITHUB}
        cd -
    else
        cd ${ROOT}/dl/${UBOOT_NAME}
        git pull
    fi

    cp -rfa ${ROOT}/dl/${UBOOT_NAME} ${OUTPUT}/${UBOOT_NAME}-${UBOOT_VERSION}
    [ -L ${WORKSPACE}/uboot ] && rm ${WORKSPACE}/uboot -rf
    ln -s ${OUTPUT}/${UBOOT_NAME} ${WORKSPACE}/uboot
fi


## Get from External package
if [ ${UBOOT_SRC} = "2" ];then
    echo "Not supported currently"
fi

## Get from wget
if [ ${UBOOT_SRC} = "3" ];then
    BASE_NAME=${UBOOT_NAME}-${UBOOT_VERSION}.tar.bz2
    if [ ! -f ${ROOT}/dl/${BASE_NAME} ];then
        cd ${ROOT}/dl
        wget ${UBOOT_SITE}/${BASE_NAME}
        [ ! $? ] && echo "Downloading finish"
    fi
    cp  ${ROOT}/dl/${BASE_NAME} ${OUTPUT}/
    cd ${OUTPUT}/
    tar -jxf ${BASE_NAME}
    if [ $? -ne 0 ];then
        echo -e "\033[31m Invalid tar operation for: -xvf \033[0m"
        exit -1
    fi
    [ -L ${WORKSPACE}/uboot ] && rm ${WORKSPACE}/uboot -rf
    ln -s ${OUTPUT}/${UBOOT_NAME}-${UBOOT_VERSION} ${WORKSPACE}/uboot
fi

