#!/bin/bash

####################
#
# test settings
#
####################
outdir="/tmp"
driver_modes="0 4"		;# driver mode
address=0
offset=0
io_min=64
io_max=$((1 << 30))		;# 1GB
delay=5				;# delay between each test

fio_time=30
fio_thread_list="4 8"
fio_iodir_list="h2c c2h bi"

####################
#
# main body
#
####################

display_help() {
	echo "$0 <xdma BDF> [log dir]"
	echo -e "xdma BDF:\tfpga pci device specified in the format of "
	echo -e "\t\t\t<domain>:<bus>:<device>.<func>"
	echo -e "log dir:\toptional, default to /tmp"
	echo
	exit;
}

if [ $# -eq 0 ]; then
	display_help
fi

bdf=$1
if [ $# -gt 1 ]; then
	outdir=$2
fi

echo "xdma bdf:$bdf, outdir: $outdir"

source ./libtest.sh

check_if_root

curdir=$PWD

for dm in $driver_modes; do

	echo -e "\n\n====> xdma mode $dm ...\n"

	cd ../../tests

	./load_driver.sh $dm
	if [ $? -ne 0 ]; then
                echo "load_driver.sh failed: $?"
                exit 1
        fi

	cd $curdir
	xid=$(bdf_to_xdmaid $bdf)
	if [ ! -n "$xid" ]; then
        	echo "$bdf, no correponding xdma found, driver mode $dm."
        	exit 1
	fi
	echo "xdma id: $xid."

	h2c_channels=$(get_h2c_channel_count $xid)
	check_rc $? get_h2c_channel_count 1

	c2h_channels=$(get_c2h_channel_count $xid)
	check_rc $? get_c2h_channel_count 1

	channel_pairs=$(($h2c_channels < $c2h_channels ? \
			$h2c_channels : $c2h_channels))
	echo "channels: $h2c_channels,$c2h_channels, pair $channel_pairs"

	if [ "$channel_pairs" -eq 0 ]; then
		echo "Error: 0 DMA channel pair: $h2c_channels,$c2h_channels."
		exit 1
	fi

	# test cdev
	TC_dma_chrdev_open_close $xid $h2c_channels $c2h_channels

	#
	# run 1 channel at a time
	#

	for i in {1..80}; do echo -n =; done
	echo -e "\nSingle H2C Channel $h2c_channels io test ...\n"
	for ((i=0; i<$h2c_channels; i++)); do
		# aligned: no data integrity check
		./io_sweep.sh $xid $i 4 $address $offset \
			$io_min $io_max 0 1
		check_rc $? "h2c-$i" 1
		./unaligned.sh $xid $i 4 0 1 
		check_rc $? "h2c-$i-unaligned" 1
	done

	for i in {1..80}; do echo -n =; done
	echo -e "\nSingle C2H Channel $c2h_channels io test ...\n"
	for ((i=0; i<$c2h_channels; i++)); do
		./io_sweep.sh $xid 4 $i $address $offset \
			$io_min $io_max 0 1
		check_rc $? "c2h-$i" 1
		./unaligned.sh $xid 4 $i 0 1 
		check_rc $? "c2h-$i-unaligned" 1
	done

	for i in {1..80}; do echo -n =; done
	echo -e "\nh2c/c2h pair $channel_pairs io test with data check ...\n"
	for ((i=0; i<$channel_pairs; i++)); do
		./io_sweep.sh $xid $i $i $address $offset $io_min $io_max 1 1
		check_rc $? "pair-$i" 1
		./unaligned.sh $xid $i $i 1 1 
		check_rc $? "pair-$i-unaligned" 1
	done

	#
	# fio test
	#

	check_cmd_exist fio
	if [ "$?" -ne 0 ]; then
		echo "fio test skipped"
		continue
	fi

	for i in {1..80}; do echo -n =; done
	echo -e "\nfio test  ...\n"
	
	#
	# result directory structure:
	# - <outdir/fio>
	#       - <number of channels>
	#               - <direction: h2c c2h bi>
	#
	for ((i=1; i<=$channel_pairs; i++)); do
		for iodir in $fio_iodir_list; do
			out=${outdir}/fio_d${dm}/${i}/${iodir}
			mkdir -p ${out}
			rm -rf ${out}/*

			for (( sz=$io_min; sz<=$io_max; sz=$(($sz*2)) )); do
				for thread in $fio_thread_list; do
                                	name=${sz}_t${thread}
                                	echo "$iodir $i: $name ..."
					./fio_test.sh $xid $iodir $i ${sz} \
						${fio_time} ${thread} ${out}
				done
			done
	       	done
       	done
	./fio_parse_result.sh ${outdir}/fio_d${dm}

	echo -e "\n\n====> xdma mode $dm COMPLETED.\n"
done

echo "$0: COMPLETED."
