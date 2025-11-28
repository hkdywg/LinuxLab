#!/bin/bash

TEST_DIR=$TEST_RESULT_DIR/flash_test
SOURCE_DIR=$TEST_DIR/src_test_data
DST_DIR=$TEST_DIR/dst_test_data
MD5_DIR=$TEST_DIR/md5_data
LOG_FILE=$TEST_DIR/test_log.txt

usage() {
    echo "Usage: flash_stress_test.sh [dirnum] [looptime]"
    echo "Example: flash_stress_test.sh 5 20000"
}

test_max_count=200
test_max_dir=5

if [ $1 -ne 0 ]; then
    test_max_dir=$1
fi

if [ $2 -ne 0 ]; then
    test_max_count=$2
fi

echo "Test Max dir Num = $test_max_dir"
echo "Test Max count = $test_max_count"

count=0
dir_loop=0
rm -rf $LOG_FILE
mkdir -p $SOURCE_DIR
mkdir -p $DST_DIR
mkdir -p $MD5_DIR

# generate 5MB random file
rm -rf $SOURCE_DIR/*
file_path=$SOURCE_DIR
file_size=(512 1024 3456 512 1024 3456)
file_radio=(5 5 5 5 5 5)

function random()
{
    min=$1
    max=$2
    num=$(date +%s)
    ((value=$num%($max-$min)+$min+1))
    echo $value
}

file_size_count=${#file_size[*]}
file_radio_count=${#file_radio[*]}

if [ $file_size_count == $file_radio_count ]; then
    #for i in `seq 0 $file_size_count`; do
    for ((i=0;i <$file_size_count;i++)); do
        for ((j=0;j <${file_radio[$i]};j++)); do
            if [ $i==0 ]; then
                rand=$(random 0 ${file_size[$i]})
            else
                rand=$(random ${file_size[$i-1]} ${file_size[$i]})
            fi
            dd if=/dev/urandom of=$file_path/test.$i.$rand.bin bs=$rand count=1024
        done
    done
fi

cd $SOURCE_DIR
md5sum ./* > $MD5_DIR/source.md5
cd -

while [ $count -lt $test_max_count ]; do
    echo $count
    echo $count >> $LOG_FILE
    dir_loop=0
    while [ $dir_loop -lt $test_max_dir ]; do
        echo "$count test $SOURCE_DIR to $DST_DIR/$dir_loop"
        rm -rf $DST_DIR/$dir_loop
        if [ $? == 0 ]; then
            echo "$count clean $DST_DIR/$dir_loop success"
            echo "$count clean $DST_DIR/$dir_loop" >> $LOG_FILE
        else
            echo "$count clean $DST_DIR/$dir_loop error"
            echo "$count clean $DST_DIR/$dir_loop error" >> $LOG_FILE
            exit 0
        fi
        # start copy data
        echo "$count $dir_loop start copy data"
        cp -rf $SOURCE_DIR $DST_DIR/$dir_loop
        if [ $? == 0 ]; then
            echo "$count copy $source_dir to $DST_DIR/$dir_loop success"
            echo "$count copy $source_dir to $DST_DIR/$dir_loop" >> $LOG_FILE
        else
            echo "$count copy $source_dir to $DST_DIR/$dir_loop error"
            echo "$count copy $source_dir to $DST_DIR/$dir_loop error" >> $LOG_FILE
        fi
        dir_loop=$(($dir_loop+1))
    done
    dir_loop=0
    sync && echo 3 > /proc/sys/vm/drop_caches
    sleep 5
    sync
    echo 3 > /proc/sys/vm/drop_caches
    sleep 5
    while [ $dir_loop -lt $test_max_dir ]; do
        # calc dir md5
        echo "$count calc $DST_DIR/$dir_loop md5 start"
        echo "$count calc $DST_DIR/$dir_loop md5 start" >> $LOG_FILE
        cd $DST_DIR/$dir_loop
        md5sum ./* > $MD5_DIR/dst$dir_loop.md5
        cd -
        diff $MD5_DIR/source.md5 $MD5_DIR/dst$dir_loop.md5
        if [ $? == 0 ];then
            echo "$count check source to $DST_DIR/$dir_loop success"
            echo "$count check source to $DST_DIR/$dir_loop success" >> $LOG_FILE
            rm $MD5_DIR/dst$dir_loop.md5
            rm -rf $DST_DIR/$dir_loop
        else
            echo "$count check source to $DST_DIR/$dir_loop error"
            echo "$count check source to $DST_DIR/$dir_loop error" >> $LOG_FILE
        fi
        dir_loop=$(($dir_loop+1))
    done
    count=$(($count+1))
    echo "--------------------------"
done

    echo "--------copy and check success----------"
    echo "--------copy and check success----------" >> $LOG_FILE








