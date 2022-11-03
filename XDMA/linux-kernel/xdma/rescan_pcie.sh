#!/bin/bash

lspcidev=$(lspci -d 10ee:7028 | cut -d " " -f1)
xdev=$(ls /sys/bus/pci/devices | grep  $lspcidev)
# e.g  xdev="0000:09:00.0"

echo 1 > /sys/bus/pci/devices/$xdev/remove
echo 1 > /sys/bus/pci/rescan
echo "Boards found after rescan:"
lspci -d 10ee:0032
echo "Isolating Interrupts"
./irq_isolate.sh
