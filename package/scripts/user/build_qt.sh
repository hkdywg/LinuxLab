#!/bin/bash

#user=`id -u`
#[ "${user}" != "0" ] && echo -e "\033[31m must be root user!!!\033[0m" && exit

PROCESS_NUM=`cat /proc/cpuinfo | grep processor | wc -l`

#source  /opt/poky/environment-setup-aarch64-poky-linux
export PATH=/usr/bin:${PATH}

SCRIPT_PATH=$(pwd)

#
#PLATFORM=linux-aarch64-gnu-g++
PLATFORM=linux-aarch64-v3h

#源码解压后的名称
MARJOR_NAME=qt-everywhere-src

#选择下载的源码版本
OPENSRC_VER_PREFIX=5.12
OPENSRC_VER_SUFFIX=.9

#源码解压后的名称
PACKAGE_NAME=${MARJOR_NAME}-${OPENSRC_VER_PREFIX}${OPENSRC_VER_SUFFIX}

#安装路径
INSTALL_PATH=/opt/${PACKAGE_NAME}

#安装包名称
COMPRESS_PACKAGE=${PACKAGE_NAME}.tar.xz

OPENSRC_VER=${OPENSRC_VER_PREFIX}${OPENSRC_VER_SUFFIX}

case ${OPENSRC_VER_PREFIX} in
    5.9 | 5.12 | 5.13 | 5.14 | 5.15 )
        DOWNLOAD_LINK=http://download.qt.io/official_releases/qt/${OPENSRC_VER_PREFIX}/${OPENSRC_VER}/single/${COMPRESS_PACKAGE}
        ;;
    *)
        DOWNLOAD_LINK=http://download.qt.io/new_archive/qt/${OPENSRC_VER_PREFIX}/${OPENSRC_VER}/single/${COMPRESS_PACKAGE}
        ;;
esac

SRC_PATH=${SCRIPT_PATH}/${PACKAGE_NAME}
CONFIG_PATH=${SCRIPT_PATH}/${PACKAGE_NAME}/qtbase/mkspecs/${PLATFORM}
CONFIG_FILE=${CONFIG_PATH}/qmake.conf


#下载源码包
do_download_src () {
    echo -e "\033[1;33mstart download ${PACKAGE_NAME}...\033[0m"

    if [ ! -f "${COMPRESS_PACKAGE}" ]; then
        if [  ! -d "${PACKAGE_NAME}" ];then
            wget -c ${DOWNLOAD_LINK}
        fi
    fi
    echo -e "\033[1;33mdone...\033[0m"
}

#解压源码
do_tar_package () {
    echo -e "\033[1;33mstart unpacking the ${PACKAGE_NAME} package ...\033[0m"
    
    if [ ! -d "${PACKAGE_NAME}" ]; then
        tar -xf ${COMPRESS_PACKAGE}
    fi
     # 修复5.11.3 版本的bug
     if [ ${OPENSRC_VER_PREFIX}=="5.11" -a ${OPENSRC_VER_SUFFIX}==".3" ]; then
        sed 's/asm volatile /asm /' -i ${SRC_PATH}/${PACKAGE_NAME}/qtscript/src/3rdparty/javascriptcore/JavaScriptCore/jit/JITStubs.cpp
     fi

    echo -e "\033[1;33mdone...\033[0m"
}

#安装依赖项
do_install_config_dependent () {
    sudo apt install python -y
    sudo apt install g++ make qt3d5-dev-tools -y
    sudo apt install qml-module-qtquick-xmllistmodel -y
    sudo apt install qml-module-qtquick-virtualkeyboard qml-module-qtquick-privatewidgets qml-module-qtquick-dialogs qml -y
    sudo apt install libqt53dquickscene2d5 libqt53dquickrender5 libqt53dquickinput5 libqt53dquickextras5 libqt53dquickanimation5 libqt53dquick5 -y
    sudo apt install qtdeclarative5-dev qml-module-qtwebengine qml-module-qtwebchannel qml-module-qtmultimedia qml-module-qtaudioengine -y
}

#修改配置文件
do_configure_pre () {
    echo -e "\033[1;33mstart configure platform ...\033[0m"

    if [ ! -d "${CONFIG_PATH}" ];then
        cp -a ${SCRIPT_PATH}/${PACKAGE_NAME}/qtbase/mkspecs/linux-aarch64-gnu-g++ ${CONFIG_PATH}
    fi

    #sed -i "s/aarch64-linux-gnu/aarch64-poky-linux/g" ${CONFIG_FILE}
     echo "#" > ${CONFIG_FILE}
     echo "# qmake configuration for building with aarch64-poky-linux-g++" >> ${CONFIG_FILE}
     echo "#" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "MAKEFILE_GENERATOR      = UNIX" >> ${CONFIG_FILE}
     echo "CONFIG                 += incremental" >> ${CONFIG_FILE}
#     echo "CONFIG                 += std_atomic64" >> ${CONFIG_FILE}
     echo "QMAKE_INCREMENTAL_STYLE = sublib" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "include(../common/linux.conf)" >> ${CONFIG_FILE}
     echo "include(../common/gcc-base-unix.conf)" >> ${CONFIG_FILE}
     echo "include(../common/g++-unix.conf)" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "# modifications to g++.conf" >> ${CONFIG_FILE}
     echo "QMAKE_CC                = ${CC} " >> ${CONFIG_FILE}
     echo "QMAKE_CXX               = ${CXX}" >> ${CONFIG_FILE}
     echo "QMAKE_LINK              = ${CXX}" >> ${CONFIG_FILE}
     echo "QMAKE_LINK_SHLIB        = ${CXX}" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}
     echo "# modifications to linux.conf" >> ${CONFIG_FILE}
     echo "QMAKE_AR                = aarch64-poky-linux-ar cqs" >> ${CONFIG_FILE}
     echo "QMAKE_OBJCOPY           = aarch64-poky-linux-objcopy" >> ${CONFIG_FILE}
     echo "QMAKE_NM                = aarch64-poky-linux-nm -P" >> ${CONFIG_FILE}
     echo "QMAKE_STRIP             = aarch64-poky-linux-strip" >> ${CONFIG_FILE}
     echo "load(qt_config)" >> ${CONFIG_FILE}
     echo "" >> ${CONFIG_FILE}

    echo -e "\033[1;33mdone...\033[0m"
}

#配置选项
do_configure () {
    echo -e "\033[1;33mstart configure ${PACKAGE_NAME} ...\033[0m"
    
    SYS_ROOT=$(printenv CC | awk '{print $8}')

    mkdir -p ${SRC_PATH}/build_path

    cd ${SRC_PATH}/build_path


    ../configure \
    -prefix ${INSTALL_PATH} \
    -xplatform ${PLATFORM} \
    -shared \
    -release \
    -opensource \
    -confirm-license \
    -no-openssl \
    -no-opengl \
    -no-xcb \
    -no-eglfs \
    -no-iconv \
    -nomake examples \
    -nomake tests \
    -skip qtmacextras \
    -skip qtandroidextras \
    -c++std c++11 \
    -no-glib  \
    -kms    \
    ${SYS_ROOT}

    cd -

    echo -e "\033[1;33mdone...\033[0m"
}

#编译后安装
do_make_install () {
    echo -e "\033[1;33mstart configure ${PACKAGE_NAME} ...\033[0m"
    cd ${SRC_PATH}/build_path
    time make -j${PROCESS_NUM} && make install
    cd -
    echo -e "\033[1;33mdone...\033[0m"
}

#删除下载的文件
do_delete_file () {
    cd ${SCRIPT_PATH}
    if [ -f "${COMPRESS_PACKAGE}" ]; then
        sudo rm -f ${COMPRESS_PACKAGE}
    fi
}

#检查交叉编译环境
do_check_crosscompile_tool () {
    if [ -z "${CC}" ];then
        echo -e "\033[31m crosscopile tools is not set!!! \033[0m"        
        exit -1
    fi
    if [[ "${CC}" != aarch64-poky-linux-gcc* ]];then
        echo -e "\033[31m crosscopile tools not correct!!! \033[0m"        
        exit -1
    fi
}

do_check_crosscompile_tool
do_download_src
do_tar_package
do_install_config_dependent
do_configure_pre
do_configure
do_make_install
#do_delete_file

