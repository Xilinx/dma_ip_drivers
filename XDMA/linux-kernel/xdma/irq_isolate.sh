#!/bin/bash
# check the number of the interupt with:
# cat /proc/interrupts | grep xdma

xirqline=$(cat /proc/interrupts | grep xdma)
xirq=$(echo $xirqline | cut -d ":" -f1)

echo $xirq

tuna --irqs=* --cpus=0-2 --move
tuna --irqs=$xirq --cpus=3 --move
tuna --show_irqs
