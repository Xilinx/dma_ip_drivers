#
# This file is part of the Xilinx DMA IP Core driver for Linux
#
# Copyright (c) 2017-2019,  Xilinx, Inc.
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

if [ -z $1 ] || [ $1 == '-h' ]
then
	echo "ERROR:   Invalid Command Line"
	echo "Example: run_st_c2h_vf.sh pfn vfn size(in byte) num_pkt iteration"
	exit 
fi

################################################
# User Configurable Parameters
################################################

pfn=$1 # PF number
vfn=$2 # VF number
size=$3 # Size per payload packet
num_pkt=$4 # number of payload packet
iteration=$5 # iterations of C2H tests
num_qs=4
logfile="loopback$1_$2.log"
infile='/root/Desktop/datafile_16bit_pattern.bin'

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

usr_bar=1
vf=0
host_adr_high=4094
mem_div=16384
s_high=$mem_div

function randomize_tx_params() {
        #random host address between 0 to 3094
        hst_adr1=$random
        let "hst_adr1 %= $host_adr_high"
        hst_adr1=0;
        # byte size
        f1=$random
        let "f1 %= 2"
        if [ $f1 -eq 0 ]; then
                size=$(( ($random %64) +1 )) ## for random number between 1 and 64
        else
                size=$(( ($random<<15)|($random+1) ))
        fi
        let "size %= $s_high"
        echo "size = $size"
        even=$size
        let "even %= 2"
        if [ $even -eq 1 ];then
                let "size = $size+1"
        fi

 	num_pkt=$(( ($RANDOM % 9) +1))
	echo " num_pkt = $num_pkt"

}

function queue_start() {
	echo "---- Queue Start $2 ----"
	dmactl qdma$1 q add idx $2 mode $3 dir h2c
	dmactl qdma$1 q start idx $2 dir h2c mode $3
	dmactl qdma$1 q add idx $2 mode $3 dir c2h
	dmactl qdma$1 q start idx $2 dir c2h mode $3
}

function cleanup_queue() {
	echo "---- Queue Clean up $2 ----"
        dmactl qdma$1 q stop idx $2 dir h2c mode $3
        dmactl qdma$1 q del idx $2 dir h2c mode $3
        dmactl qdma$1 q stop idx $2 dir c2h mode $3
        dmactl qdma$1 q del idx $2 dir c2h mode $3
}


echo "############################# AXI-ST Start #################################"


for ((i=0; i< $num_qs; i++)) do
	# Setup for Queues
	qid=$i
        hw_qid=$(($qid + $(($vfn*8)) + 128))
	dev_st_c2h="/dev/qdmavf0-ST-C2H-$qid"
	dev_st_h2c="/dev/qdmavf0-ST-H2C-$qid"
	out="out$qid"
        loop=1

	# Open the Queue for AXI-ST streaming interface.
	queue_start vf$vf $qid st > /dev/null

	while [ "$loop" -le $iteration ]
	do
		# Determine if DMA is targeted @ random host address
		if [ $f_size -eq 1 ]; then
			hst_adr1=0
		else
			randomize_tx_params
		fi

		# if more packets are requested.
		let "tsize= $size*$num_pkt"
 
                echo ""
                echo "########################################################################################"
		echo "#############  H2C ST LOOP $loop : dev=$dev_st_h2c pfn=$pfn vfn=$vfn qid=$qid hw_qid=$hw_qid"
		echo "#############               transfer_size=$tsize pkt_size=$size pkt_count=$num_pkt hst_adr=$hst_adr1"
                echo "########################################################################################"

                #clear match bit before each H2C ST transfer
		dmactl qdmavf0 reg write bar $usr_bar 0x0c 0x01


		# H2C transfer 
		dma_to_device -d $dev_st_h2c -f $infile -s $tsize &
                re=$?

		wait

	        # Check match bit and QID
		hwqid_match=$(dmactl qdmavf0 reg read bar $usr_bar 0x10 | grep -o '0x[0-9][0-9][0-9]')
                hw_qid_hex=$(printf '%x' $hw_qid)
                if [ $hwqid_match != 0x$hw_qid_hex'1' ]; then
                  echo "#### ERROR: QID MATCH is $hwqid_match"
                  re=-1
                fi               

                if [ $re == 0 ]; then 
                  echo "######################################################"
		  echo "##############   VF H2C ST PASS QID $qid ################"
                  echo "######################################################"
                else
                  echo "#### ERROR: VF H2C ST FAIL"
                fi

                echo ""
                echo "########################################################################################"
		echo "#############  C2H ST LOOP $loop : dev=$dev_st_c2h pfn=$pfn vfn=$vfn qid=$qid hw_qid=$hw_qid"
		echo "#############               transfer_size=$tsize pkt_size=$size pkt_count=$num_pkt hst_adr=$hst_adr1"
                echo "########################################################################################"

		dmactl qdmavf0 reg write bar $usr_bar 0x0 $hw_qid  # for Queue 0
		dmactl qdmavf0 reg write bar $usr_bar 0x4 $size
		dmactl qdmavf0 reg write bar $usr_bar 0x20 $num_pkt #number of packets
		dmactl qdmavf0 reg write bar $usr_bar 0x08 2 # Must set C2H start before starting transfer

		dma_from_device -d $dev_st_c2h -f $out -o $hst_adr1 -s $tsize &

		wait

		#Check if files is there.
		if [ ! -f $out ]; then
			echo " #### ERROR: Queue $qid output file does not exists ####"
			echo " #### ERROR: Queue $qid output file does not exists ####" >> $logfile
			cleanup_queue vf$vf $qid st
			exit -1
		fi

		# check files size
		filesize=$(stat -c%s "$out")
		if [ $filesize -gt $tsize ]; then
			echo "#### ERROR: Queue $qid output file size does not match, filesize= $filesize ####"
			echo "#### ERROR: Queue $qid output file size does not match, filesize= $filesize ####" >> $logfile
			cleanup_queue vf$vf $qid st
			exit -1 
		fi

		#compare file
		cmp $out $infile -n $tsize
		if [ $? -eq 1 ]; then
			echo "#### Test ERROR. Queue $qid data did not match ####" 
			echo "#### Test ERROR. Queue $qid data did not match ####" >> $logfile
			dmactl qdmavf0 q dump idx $qid mode st dir c2h
			dmactl qdmavf0 reg dump
			cleanup_queue vf$vf $qid st
			exit -1
		else
                        echo "######################################################"
			echo "##############   VF C2H ST PASS QID $qid ################"
                        echo "######################################################"
		fi
		wait
		((loop++))
	done
	cleanup_queue vf$vf $qid st > /dev/null
done
echo "########################## AXI-ST completed ###################################"
exit 0

