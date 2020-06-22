#!/bin/bash
#
# Simple run script to test QDMA in VF AXI-MM mode.
# 
# VF AXI-MM Transfer
#	- H2C operation is performed to send data to BRAM on the FPGA. 
#	- C2H operation is performed to reads data from BRAM.
#       -   C2H data is stored in a local file 'out_mm0_$qid', which will be compared to original file for correctness. 

################################################
# User Configurable Parameters
################################################
iteration=$1 # [Optional] Iterations 
size=$2 # [Optional] Size per payload packet

################################################
# Hard Coded Parameters
################################################
q_per_vf=1
vf=00080
size_max=4096
host_adr_high=0
infile='./datafile_16bit_pattern.bin'


################################################
# Input check
################################################
if [ "$1" == "-h" ]; then
	echo "Example: qdma_run_test_st_vf.sh [iteration] [size(in byte)]"
	echo "Example: qdma_run_test_st_vf.sh This will run VF MM test in random mode"
        exit
fi

if [ -z $2 ] || [ $# -eq 0 ] ; then
	echo "Run VF MM test in random mode"
	sleep 3
fi

if [ -z $iteration ]; then
	iteration=1 
fi

if [ ! -z $size ]; then 
	f_size=1
else
   	f_size=0
fi

#################################################
# Helper Functions
################################################

function randomize_tx_params() {
        #random host address 
	if [ $host_adr_high -ne 0 ]; then
		hst_adr1=$RANDOM
		hst_adr1=$((hst_adr1 % host_adr_high))
	else
		hst_adr1=0
	fi

	# byte size
	size=$RANDOM
        if [ $size -eq 0 ]; then
		size=$(($RANDOM % 64 + 1)) ## for random number between 1 and 64
        else
		size=$((size % $size_max))
        fi

	# Correct if size is odd	
	even=$((size%2))	
        if [ $even -eq 1 ];then
		size=$((size+1))
        fi
}


function queue_start() {
	echo "---- Queue Start $2 ----"
	dma-ctl qdma$1 q add idx $2 mode $3 dir bi
	dma-ctl qdma$1 q start idx $2 dir bi
}

function cleanup_queue() {
	echo "---- Queue Clean up $2 ----"
        dma-ctl qdma$1 q stop idx $2 dir bi
        dma-ctl qdma$1 q del idx $2 dir bi
}

vfs=`dma-ctl dev list | grep qdmavf | cut -d'	' -f1`;

echo "**** AXI-MM Start ****"
for vfsdev in $vfs;do
	vf="${vfsdev#*f}"
	q_per_vf="$(dma-ctl dev list |grep qdmavf$vf | cut -d ' ' -f 3 | cut -d ',' -f 1 | xargs)"

	for ((i=0; i< $q_per_vf; i++)) do
	# Setup for Queues
	qid=$i
	dev_mm_c2h="/dev/qdmavf$vf-MM-$qid"
	dev_mm_h2c="/dev/qdmavf$vf-MM-$qid"
        loop=1

		out_mm="out_mm0_"$qid
		# Open the Queue for AXI-MM streaming interface.
		queue_start vf$vf $qid mm

		while [ "$loop" -le $iteration ]
		do
			# Determine if DMA is targeted @ random host address
			if [ $f_size -eq 1 ]; then
				hst_adr1=0
			else
				randomize_tx_params
			fi

			# H2C transfer 
			dma-to-device -d $dev_mm_h2c -f $infile -s $size -o $hst_adr1

			# C2H transfer
			dma-from-device -d $dev_mm_c2h -f $out_mm -s $size -o $hst_adr1

			# Compare file for correctness
			cmp $out_mm $infile -n $size

			if [ $? -eq 1 ]; then
				echo "#### Test ERROR. Queue $qid data did not match ####"
				exit 1
			else
				echo "**** Test pass. Queue $qid"
			fi

			wait

			((loop++))
		done
		# Close the Queues
		cleanup_queue vf$vf $qid
	done
done
echo "**** AXI-MM completed ****"
