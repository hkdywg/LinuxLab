#!/bin/sh

I2CBUS=0                       # 根据硬件调整
DES_ADDR=0x0c                  # desReadAddress
PAGES=20
REGISTERS=255

DATE=$(date +"%Y%m%d-%H%M%S")
CSV_NAME="PageDump${DES_ADDR}_${DATE}.csv"

echo "生成文件: $CSV_NAME"
exec 3>"$CSV_NAME"            # 文件句柄 3

###############################
# 写表头
###############################
printf "," >&3
for x in $(seq 0 $REGISTERS); do
    printf "%s," "$(printf "0x%x" $x)" >&3
done
printf "\r\n" >&3

###############################
# Main Page
###############################
printf "MainPage," >&3
echo "读取 Main Page..."

for x in $(seq 0 $REGISTERS); do
    REG=$(printf "0x%02X" "$x")
    VAL=$(i2cget -f -y "$I2CBUS" "$DES_ADDR" "$REG")
    printf "%s," "$VAL" >&3
done
printf "\r\n" >&3
echo "Main Page Done"

###############################
# Page 1~20
###############################
for y in $(seq 1 $PAGES); do
    REG=$(printf "0x%02X" "$((y*4+3))")
    i2cset -f -y $I2CBUS $DES_ADDR 0x40 "$REG"
    i2cset -f -y $I2CBUS $DES_ADDR 0x41 0x00

    printf "Page%d," $y >&3

    for x in $(seq 0 $REGISTERS); do
        VAL=$(i2cget -f -y $I2CBUS $DES_ADDR 0x42)
        printf "%s," "$VAL" >&3
    done

    printf "\r\n" >&3
    echo "Page $y Complete"
done

exec 3>&-       # 关闭 CSV 文件
echo "Complete"

