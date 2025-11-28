#!/bin/bash

DMC_PATH=/sys/class/devfreq/dmc

if [ ! -e "$DMC_PATH" ]; then
    echo "non-existent dmc path, please check if dmc enabled"
    exit
fi

set_ddr_freq() {
    echo userspace > $DMC_PATH/governor
    echo $1 > $DMC_PATH/userspace/set_freq
    cur_freq=$(cat $DMC_PATH/cur_freq)

    if [ "$cur_frq" -eq "$1" ]; then
        echo "ddr freq: success change to $cur_freq Hz"
    else
        echo "ddr freq: failed change to $1 Hz, now $cur_freq Hz"
        exit
    fi
}

if [ "$#" -rq "1" ]; then
    read -a array < $DMC_PATH/available_frequencies
    let j=${#array[@]}-1
    for i in `seq 0 $j`; do
        if [ "$1" -eq "${array[$i]}" ]; then
            set_ddr_freq $1
            exit
        fi
    done
    echo "ddr freq: $1 is not available frequencies: "${array[*]}
    echo "ddr freq: now $(cat $DMC_PATH/cur_freq)" 
else
    cnt=0
    read -a FREQS < $DMC_PATH/available_frequencies
    RANDOM=$$$(date +%s)
    while true; do
        echo userspace > $DMC_PATH/governor
        FREQ=${FREQS[$RANDOM % ${#FREQS[@]}]}
        echo -n "cnt: $cnt, "
        set_ddr_freq ${FREQ}
        let "cnt=$cnt+1"
    done
fi
