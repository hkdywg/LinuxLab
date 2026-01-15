#!/bin/bash

set -e

# -----------------------------
# 获取所有已连接的 connector-id
# -----------------------------
mapfile -t CONNECTORS < <(modetest -c | awk '$3=="connected" {print $1}')

if [ ${#CONNECTORS[@]} -eq 0 ]; then
    echo "No connected connectors found"
    exit 1
fi

# -----------------------------
# 视频文件列表（按顺序分配）
# -----------------------------
VIDEOS=(
    "rtsp://172.29.4.196:8554/video1"
    "rtsp://172.29.4.196:8554/video2"
    "rtsp://172.29.4.196:8554/video3"
)

if [ ${#VIDEOS[@]} -eq 0 ]; then
    echo "No video files specified"
    exit 1
fi

# -----------------------------
# Ctrl+C 时清理所有子进程
# -----------------------------
cleanup() {
    echo
    echo "Stopping all playback..."
    pkill -P $$
    exit 0
}
trap cleanup INT TERM

# -----------------------------
# 启动播放
# -----------------------------
echo "Found ${#CONNECTORS[@]} connectors"

for i in "${!CONNECTORS[@]}"; do
    cid=${CONNECTORS[$i]}

    # 视频索引：超出时循环使用
    vid=${VIDEOS[$(( i % ${#VIDEOS[@]} ))]}

    echo "Connector $cid -> Video: $vid"

    ./gst_dec_display \
        -c "$cid" \
        -u "$vid" > /dev/null &
done

wait

