#/bin/bash

RESULT_LOG=${TEST_RESULT_DIR}/stressapptest.log

# get free memory size
mem_avail_size=$(cat /proc/meminfo | grep MemAvailable | awk '{print $2}')
mem_test_size=$(((mem_avail_size/1024/2)-10))M

# run memtester test
echo "******************************* DDR STRESSAPPTEST TEST 24H *************************************"
echo "**run: stressapptest -s 86400 -i 4 -C 4 -W --stop_on_errors -M $mem_test_size -l $RESULT_LOG &**"

stressapptest -s 86400 -i 4 -C 4 -W --stop_on_errors -M $mem_test_size -l $RESULT_LOG &

echo "******************* DDR STRESSAPPTEST TEST START, LOG AT $RESULT_LOG **************************"
