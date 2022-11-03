#!/bin/bash
# check the number of the interupt with:
# cat /proc/interrupts | grep xdma

xirqline=$(cat /proc/interrupts | grep xdma)
xirq=$(echo $xirqline | cut -d ":" -f1)

echo $xirq

tuna --irqs=* --cpus=0-6 --move
tuna --irqs=$xirq --cpus=7 --move
tuna --show_irqs
