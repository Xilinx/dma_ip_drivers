#
# This file is part of the Xilinx DMA IP Core driver for Linux
#
# Copyright (c) 2018-2019,  Xilinx, Inc.
# All rights reserved.
#
# This source code is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#

#!/bin/bash

function print_help() {
	echo ""
	echo "Usage : $0 <bbddf> [<test mask>] [<total time>] [<override_pf_nvf>]"
	echo "Ex : $0 06000 0x1 00:00:30 2048 1"
	echo "<bdf> : PF Bus device function in bbddf format ex:06000"
	echo ""
	echo "<test mask> : This parameter specifies the mode in which the kernel modules need to be loaded."
	echo "             1 - Auto "
	echo "             2 - Poll "
	echo "             4 - Direct Interrupt Mode "
	echo "             8 - Indirect Interrupt Mode"
	echo ""
	echo "<total_time> : execution time in hh:mm:ss format"	
	echo "           Default - 00:01:00 "
	echo ""
	echo "<override_pf_nvf> : override the default pf_nvf_lst by reading sriov_totalvfs"
	echo "           Default - 0 - 1 VF per PF and all queues are divided equally among the total functions"
    echo "                     1 - Max Possible VFs are assigned to all PFs and the queues are equally distributed among the total functions "
	echo ""
	echo ""
    echo ""
}

function print_functionality() {
	echo ""
	echo "This script does the stress testing of QDMA IP Linux Driver."
	echo "This script requires the information of following parameters:"
	echo "    --> primary pcie interface in form of BDF (mandatory)"
	echo "    --> Toatl number of queues on which this tessting needs to be done (optional)."
	echo "        This paramater deafults to 2048. These queues are distributed for all available functions equally"
	echo "    --> Total durations for which the transfers needs to be stressed (optional) in hh:mm:sec format."
	echo "        This defaults to 1 min."
	echo "    --> whether to overide the default number of vfs per PF (which is 1) with the hw supported VFs per PF (optional)."
	echo "        The script reads from sriov_totalvfs to find the hw supported vfs per pf."
	echo ""
	echo "After the QDMA Linux PF and VF drivers are inserted, the script queries the device list using \'dmactl dev list\' command"
	echo "to find out the available PFs. The driver then starts vfs based on @pf_nvf_lst. This @pf_nvf_lst is updated with the actual"
	echo "hw suppoted nvf per pf by reading sriov_totalvfs if @overide_pf_nvf_lst is set. After all the vfs have been started, "
	echo "the script calculates total q's per function by (total_functions/total_q's). This qmax per function is set using sysfs of"
	echo "each function. The script again queries the available device list and gathers all the available devices and their q offsets."
	echo ""
	echo "Now the script starts setting up the environment by adding and starting all the queues (in both directions) in all functions."
	echo "While doing this, the script alternates between MM mode and ST mode for the queues. During this setup time, mm mode queues are pushed"
	echo "to a separate list and st queues are pushed to a separate list. Now separate therads are launched for MM , ST H2C and ST C2H transfers."
	echo ""
	echo "MM transfers thread runs on mm q list and ST H2C and ST C2H threads run on st q list. Each thread loops throught the list and"
	echo "transfers the data in respective direction. All the logs from each thread are collected into separate log files for book keeping"
	echo "later which will be used to detect any failures. These transfer tests continue for the amount of duration specified by @total_time."
	echo "For every loop the size of data transfer is determined by randamize function."
	echo ""
	echo "***Note: An exit function is registerd so that when the scirp completes execution or is forcefully terminated, the environemnt that"
	echo "was setup will be cleaned up."
}

if [ $# -lt 1 ]; then
	echo "Invalid arguements."
	print_help
	print_functionality
	exit 1
fi;

if [ $1 == "-h" ]; then
	print_help
	print_functionality
	exit 0
fi

bdf=01000
num_qs=2048
override_pf_nvf=0
if [ ! -z $1 ]; then
	bdf=$1
fi
test_mask=0x0f
if [ ! -z $2 ]; then
	test_mask=$2
fi
run_time="04:00:00"
if [ ! -z $3 ]; then
	run_time=$3
fi
if [ ! -z $4 ]; then
	num_qs=$4
fi
if [ ! -z $5 ]; then
	override_pf_nvf=$5
fi

cd ..
./make_libqdma.sh clean
./make_libqdma.sh
make install-user
make install-dev

insmod build/qdma.ko
echo ${bdf}
dmactl qdma${bdf} reg write bar 2 0xA0 0x01
rmmod qdma
cd -


function cleanup_env()
{
	rmmod qdma_vf
	rmmod qdma
}

function run_stress_test()
{
	drv_mode=0
	if [ $1 == poll ]; then
		drv_mode=1111
	elif [ $1 == intr ]; then
		drv_mode=2222
	elif [ $1 == aggr ]; then
		drv_mode=3333
	else
		drv_mode=0000
	fi
	pci_bus=${bdf:0:2}
	cd ..
	insmod build/qdma.ko mode=0x${pci_bus}${drv_mode}
	insmod build/qdma_vf.ko mode=0x${pci_bus}${drv_mode}
	cd -
	chmod +x qdma_run_stress_test.sh
	./qdma_run_stress_test.sh $bdf $num_qs $run_time $override_pf_nvf $1
	dmactl qdma$bdf reg write bar 2 0xA0 0x01
	cleanup_env
}

trap cleanup_env EXIT

if (( test_mask & 0x02 )); then
	run_stress_test poll
fi
if (( test_mask & 0x04 )); then
	run_stress_test intr
fi
if (( test_mask & 0x08 )); then
	run_stress_test aggr
fi
if (( test_mask & 0x01 )); then
	run_stress_test auto
fi

