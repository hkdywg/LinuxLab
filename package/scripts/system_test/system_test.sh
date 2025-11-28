#!/bin/bash

CURRENT_DIR=`dirname $0`
TEST_RESULT_DIR=${CURRENT_DIR}/result
TEST_LOG_DIR=${CURRENT_DIR}/log

mkdir -p ${TEST_RESULT_DIR}
mkdir -p ${TEST_LOG_DIR}

export MODULE_CHOICE TEST_RESULT_DIR TEST_LOG_DIR

module_choice()
{
    echo "******************************************************"
    echo "***                                                ***"
    echo "***          *****************************         ***"
    echo "***          *    SYSTEM TEST TOOLS      *         ***"
    echo "***          *  V1.0 updated on 20251128 *         ***"
    echo "***          *****************************         ***"
    echo "***                                                ***"
    echo "*****************************************************"


    echo "*****************************************************"
    echo "ddr test:             1 (ddr stress test)"
    echo "cpu test:             2 (cpu stress test)"
    echo "gpu test:             3 (gpu stress test)"
    echo "npu test:             4 (npu stress test)"
    echo "suspend_resume test:  5 (suspend resume)"
    echo "reboot test:          6 (auto reboot test)"
    echo "power lost test:      7 (power lost test)"
    echo "flash stress test:    8 (flash stress test)"
    echo "recovery test:        9 (recovery wipe all test)"
    echo "audio test:           10 (audio test)"
    echo "camera test:          11 (camera test)"
    echo "video test:           12 (video test)"
    echo "bluetooth test:       13 (bluetooth test)"
    echo "wifi test:            14 (wifi test)"
    echo "wifibt config test:   15 (wifibt config test)"
    echo "pcie test:            16 (pcie test)"
    echo "chromium test:        17 (chromium with video test)"
    echo "benchmark test:       18 (unixbench、glmark2...)"
    echo "*****************************************************"

    read -t 30 -p "please input test moudle: " MODULE_CHOICE
}


ddr_test()
{
    bash ${CURRENT_DIR}/ddr/ddr_test.sh
}

cpu_test()
{
    bash ${CURRENT_DIR}/cpu/cpu_test.sh
}
