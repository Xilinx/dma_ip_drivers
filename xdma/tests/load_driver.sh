#!/bin/bash
# set -x

display_help() {
        echo "$0 [interrupt mode]"
        echo "interrupt mode: optional"
        echo "0: auto"
        echo "1: MSI"
        echo "2: Legacy"
        echo "3: MSIx"
        echo "4: do not use interrupt, poll mode only"
        exit;
}

if [ "$1" == "help" ]; then
        display_help
fi;

interrupt_selection=$1
echo "interrupt_selection $interrupt_selection."
device_id=903f

# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

# Remove the existing xdma kernel module
lsmod | grep xdma
if [ $? -eq 0 ]; then
	rmmod xdma
	if [ $? -ne 0 ]; then
		echo "rmmod xdma failed: $?"
		exit 1
	fi
fi

# Use the following command to Load the driver in the default 
# or interrupt drive mode. This will allow the driver to use 
# interrupts to signal when DMA transfers are completed.
echo -n "Loading driver..."
case $interrupt_selection in
	"0")
		echo "insmod xdma.ko interrupt_mode=1 ..."
		ret=`insmod ../xdma/xdma.ko interrupt_mode=0`
		;;
	"1")
		echo "insmod xdma.ko interrupt_mode=2 ..."
		ret=`insmod ../xdma/xdma.ko interrupt_mode=1`
		;;
	"2")
		echo "insmod xdma.ko interrupt_mode=3 ..."
		ret=`insmod ../xdma/xdma.ko interrupt_mode=2`
		;;
	"3")
		echo "insmod xdma.ko interrupt_mode=4 ..."
		ret=`insmod ../xdma/xdma.ko interrupt_mode=3`
		;;
	"4")
		echo "insmod xdma.ko poll_mode=1 ..."
		ret=`insmod ../xdma/xdma.ko poll_mode=1`
		;;
	*)
		intp=`sudo lspci -d :${device_id} -v | grep -o -E "MSI-X"`
		intp1=`sudo lspci -d :${device_id} -v | grep -o -E "MSI:"`
	       	if [[ ( -n $intp ) && ( $intp == "MSI-X" ) ]]; then
			echo "insmod xdma.ko interrupt_mode=0 ..."
			ret=`insmod ../xdma/xdma.ko interrupt_mode=0`
	       	elif [[ ( -n $intp1 ) && ( $intp1 == "MSI:" ) ]]; then
			echo "insmod xdma.ko interrupt_mode=1 ..."
			ret=`insmod ../xdma/xdma.ko interrupt_mode=1`
		else
			echo "insmod xdma.ko interrupt_mode=2 ..."
			ret=`insmod ../xdma/xdma.ko interrupt_mode=2`
		fi
		;;
esac

if [ ! $ret == 0 ]; then
	echo "Error: xdma driver did not load properly"
	echo " FAILED"
	exit 1
fi

# Check to see if the xdma devices were recognized
echo ""
cat /proc/devices | grep xdma > /dev/null
returnVal=$?
if [ $returnVal == 0 ]; then
	# Installed devices were recognized.
	echo "The Kernel module installed correctly and the xmda devices were recognized."
else
	# No devices were installed.
	echo "Error: The Kernel module installed correctly, but no devices were recognized."
	echo " FAILED"
	exit 1
fi

echo "DONE"
