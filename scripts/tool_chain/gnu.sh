#!/bin/bash

# Establish linaro GNU GCC.
#
# (C) 2021.11.11 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

ROOT=$1
GNU_TOOLCHAIN_NAME=${2%X}
GNU_TOOLCHAIN_VERSION=${3%X}
GNU_TOOLCHAIN_SRC=${4%X}
GNU_TOOLCHAIN_SITE=${5%X}
GNU_TOOLCHAIN_GITHUB=${6%X}
GNU_TOOLCHAIN_WGETNAME=${GNU_TOOLCHAIN_SITE##*/}
OUTPUT=${ROOT}/output/toolchain

# Normal check
if [ ! -d ${OUTPUT} ];then
    mkdir -p ${OUTPUT}
fi

[ ! -d ${ROOT}/dl/gnu_toolchain ] && mkdir -p ${ROOT}/dl/gnu_toolchain

# Get from github
if [ "${GNU_TOOLCHAIN_SRC}" = "3" ];then
    cd ${ROOT}/dl/gnu_toolchain
    git clone ${GNU_TOOLCHAIN_GITHUB)}
    git tag > GNU_TOOLCAHIN_TAG
    cd -
    mkdir -p ${OUTPUT}/${GNU_TOOLCHAIN_NAME}
    cp -rfa ${ROOT}/dl/gnu_toolchain ${OUTPUT}/${GNU_TOOLCHAIN_NAME}
    cd ${OUTPUT}/${GNU_TOOLCHAIN_NAME}/gnu_toolchain
    git reset --hard ${GNU_TOOLCHAIN_VERSION}
    if [ $? -ne 0 ];then
        cat GNU_TOOLCHAIN_TAG
        echo -e "\033[31m GNU_TOOLCHAIN only suppot obove version! \033[0m"
        exit -1
    fi
fi


# Get from wget
if [ "${GNU_TOOLCHAIN_SRC}" = "2" ];then
    BASE_NAME=${GNU_TOOLCHAIN_WGETNAME}
    if [ ! -f ${ROOT}/dl/gnu_toolchain/${GNU_TOOLCHAIN_WGETNAME} ];then
        cd ${ROOT}/dl/gnu_toolchain
        wget ${GNU_TOOLCHAIN_SITE}.asc
        wget ${GNU_TOOLCHAIN_SITE}

        # MD5 Check
        cd ${ROOT}/dl/gnu_toolchain
        md5sum ${GNU_TOOLCHAIN_WGETNAME} > tmp_md5sum.asc
        diff tmp_md5sum.asc ${GNU_TOOLCHAIN_WGETNAME}.asc
        if [ $? -ne 0 ];then
            echo -e "\033[31m Bad Package ${GNU_TOOLCHAIN_WGETNAME} \033[0m"
            exit -1
        fi
        rm -rf tmp_md5sum.asc
        tar -xf ${GNU_TOOLCHAIN_WGETNAME} -C ${OUTPUT}
    else
        # MD5 Check
        cd ${ROOT}/dl/gnu_toolchain
        md5sum ${GNU_TOOLCHAIN_WGETNAME} > tmp_md5sum.asc
        diff tmp_md5sum.asc ${GNU_TOOLCHAIN_WGETNAME}.asc > /dev/null
        if [ $? -eq 0 ];then
            echo -e "\033[32m ${GNU_TOOLCHAIN_WGETNAME} has download \033[0m"
        else
            echo -e "\033[31m Bad Package ${GNU_TOOLCHAIN_WGETNAME} \033[0m"
            rm ${GNU_TOOLCHAIN_WGETNAME} 
            rm ${GNU_TOOLCHAIN_WGETNAME}.asc 
            exit 0
        fi
    fi
fi
