#!/bin/bash

user=`id -u`
[ "$user" != "0" ] && echo "Must be root/sudo" && exit

expect_bin=`which expect`
if [ -z ${expect_bin} ];then
    sudo apt install expect
fi

DEV_NAME=`ls /dev/ttyUSB*`

current_path=`pwd`
FILE_FLASH_WRITTER="${current_path}/files/ICUMXA_Flash_writer_SCIF_DUMMY_CERT_EB200400_condor.mot"
FILE_BOOT_PARA="${current_path}/files/bootparam_sa0.srec"
FILE_LOADER="${current_path}/files/icumxa_loader.srec"
FILE_DUMMY_FW="${current_path}/files/dummy_fw.srec"
FILE_CETRIFICATION="${current_path}/files/cert_header_sa6.srec"
FILE_DUMMY_RTOS="${current_path}/files/dummy_rtos.srec"
FILE_BL31_ATF="${current_path}/files/bl31-condor.srec"
FILE_UBOOT="${current_path}/files/u-boot-elf.srec"

[ ! -f ${FILE_FLASH_WRITTER} ] && echo "${FILE_FLASH_WRITTER} none exist!!!" && exit -1
[ ! -f ${FILE_BOOT_PARA} ] && echo "${FILE_BOOT_PARA} none exist!!!" && exit -1
[ ! -f ${FILE_LOADER} ] && echo "${FILE_LOADER} none exist!!!" && exit -1
[ ! -f ${FILE_DUMMY_FW} ] && echo "${FILE_DUMMY_FW} none exist!!!" && exit -1
[ ! -f ${FILE_CETRIFICATION} ] && echo "${FILE_CETRIFICATION} none exist!!!" && exit -1
[ ! -f ${FILE_DUMMY_RTOS} ] && echo "${FILE_DUMMY_RTOS} none exist!!!" && exit -1
[ ! -f ${FILE_BL31_ATF} ] && echo "${FILE_BL31_ATF} none exist!!!" && exit -1
[ ! -f ${FILE_UBOOT} ] && echo "${FILE_UBOOT} none exist!!!" && exit -1

##| Filename                       | Program Top Address | eMMC Save Partition | eMMC Save Sectors | Description            |
##|--------------------------------|---------------------|---------------------|-------------------|------------------------|
##| bootparam_sa0.srec             | H'EB200000          | boot partition1     | H'000000          | Loader(Boot parameter) |
##| icumxa_loader.srec             | H'EB2D8000          | boot partition1     | H'040000          | Loader                 |
##| cert_header_sa6.srec           | H'EB200000          | boot partition1     | H'180000          | Loader(Certification)  |
##| dummy_fw.srec                  | H'EB2B4000          | boot partition1     | H'0C0000          | Secure FW              |
##| dummy_rtos.srec                | H'EB200000          | boot partition1     | H'1C0000          | RTOS                   |
##| bl31-condor.srec               | H'46400000          | boot partition1     | H'640000          | OP-TEE                 |
##| u-boot-elf.srec                | H'50000000          | boot partition2     | H'000000          | U-boot                 |


expect -c "
    set timeout 300
    spawn minicom -D $DEV_NAME -b 115200;
#
# transefer flash writer mot file
#
    expect  {
        "please\ send\ !" { send \"\01\"; send \"s\"; exp_continue }
        "*Upload*" { send \"jjjj\r\"; exp_continue }
        "*Directory*" { send \"\r\"; send \"$FILE_FLASH_WRITTER\r\";exp_continue }
        "*READY:*" { send \"\r\"; }
    }

#
# set uart to high speed baud 921600
#
    expect {
        "\>" { send \"SUP\r\"; exp_continue }
        "terminal" { send \"\01\"; send \"x\"; exp_continue }
        "Leave" { send \"\r\"; }
    }

"

clear
sleep 5

expect -c "
    spawn minicom -D $DEV_NAME -b 921600;
    set timeout 300
#
# set emmc register
#
    expect {
        "Press*" { send \"\r\"; exp_continue }
        "\>" { send \"EM_SECSD\r\"; exp_continue }
        "Index" { send \"b1\r\"; exp_continue }
        "Value" { send \"a\r\"; }
    }

#
# set emmc register
#
    expect {
        "\>" { send \"EM_SECSD\r\"; exp_continue}
        "Index" { send \"b3\r\"; exp_continue }
        "Value" { send \"8\r\"; }
    }
#
# erase emmc partation 2
#
    expect "\>" { send \"EM_E\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "\>" { send \"EM_E\r\"; }
    expect "Select" { send \"2\r\"; }
#
# write bootparam_sa0.srec to emmc EB200000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"0\r\"; }
    expect "Program" { send \"EB200000\r\"; }
    expect "please" { send \"\r\" }
    expect "send" { send \"\01\"; send \"s\"; }
    expect "*Upload*" { send \"jjjj\r\"; }
    expect "*Directory*" { send \"\r\"; send \"$FILE_BOOT_PARA\r\";}
    expect "*READY:*" { send \"\r\"; }

#
# write icumxa_loader.srec to emmc EB2D8000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"6BE\r\"; }
    expect "Program" { send \"EB2D8000\r\"; }
    expect "please" { send \"\r\" }
    expect "send" { send \"\01\"; send \"s\"; }
    expect "*Upload*" { send \"jjjj\r\"; }
    expect "*Directory*" { send \"\r\"; send \"$FILE_LOADER\r\"; }
    expect "*READY:*" { send \"\r\"; }

#
# write dummy_fw.srec to emmc EB2B4000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"8CE\r\"; }
    expect "Program" { send \"EB2B4000\r\"; }
    expect "please" { send \"\r\" }
    expect "send" { send \"\01\"; send \"s\"; }
    expect "*Upload*" { send \"jjjj\r\"; }
    expect "*Directory*" { send \"\r\"; send \"$FILE_DUMMY_FW\r\";}
    expect "*READY:*" { send \"\r\"; }

#
# write cert_header_sa6.srec to emmc EB200000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"C00\r\"; }
    expect "Program" { send \"EB200000\r\"; }
    expect "please" { send \"\r\" }
    expect  "send" { send \"\01\"; send \"s\"; }
    expect  "*Upload*" { send \"jjjj\r\"; }
    expect  "*Directory*" { send \"\r\"; send \"$FILE_CETRIFICATION\r\";}
    expect  "*READY:*" { send \"\r\"; }

#
# write dummy_rtos.srec to emmc EB200000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"C2B\r\"; }
    expect "Program" { send \"EB200000\r\"; }
    expect "please" { send \"\r\" }
    expect "send" { send \"\01\"; send \"s\"; }
    expect "*Upload*" { send \"jjjj\r\"; }
    expect "*Directory*" { send \"\r\"; send \"$FILE_DUMMY_RTOS\r\";}
    expect "*READY:*" { send \"\r\"; }


#
# write bl31-condor.srec to emmc 46400000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"D54\r\"; }
    expect "Program" { send \"46400000\r\"; }
    expect "please" { send \"\r\" }
    expect "send" { send \"\01\"; send \"s\"; }
    expect "*Upload*" { send \"jjjj\r\"; }
    expect "*Directory*" { send \"\r\"; send \"$FILE_BL31_ATF\r\";}
    expect "*READY:*" { send \"\r\"; }

#
# write uboot-elf.srec to emmc 50000000
#
    expect "\>" { send \"EM_W\r\"; }
    expect "Select" { send \"1\r\"; }
    expect "sector" { send \"DF7\r\"; }
    expect "Program" { send \"50000000\r\"; }
    expect "please" { send \"\r\" }
    expect "send" { send \"\01\"; send \"s\"; }
    expect "*Upload*" { send \"jjjj\r\"; }
    expect "*Directory*" { send \"\r\"; send \"$FILE_UBOOT\r\"; }
    expect "*READY:*" { send \"\r\"; }

"
clear

echo "================burn emmc done================="
