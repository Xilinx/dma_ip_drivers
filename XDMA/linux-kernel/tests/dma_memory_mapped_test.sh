#!/bin/bash
display_help() {
	echo "$0 <xdma id> <io size> <io count> <h2c #> <c2h #>"
	echo -e "xdma id:\txdma[N] "
	echo -e "io size:\tdma transfer size in byte"
	echo -e "io count:\tdma transfer count"
       	echo -e "h2c #:\tnumber of h2c channels"
	echo -e "c2h #:\tnumber of c2h channels"
       	echo
       
	exit 1
}

if [ $# -eq 0 ]; then
	display_help
fi

xid=$1
transferSz=$2
transferCount=$3
h2cChannels=$4
c2hChannels=$5

tool_path=../tools

testError=0
# Run the PCIe DMA memory mapped write read test
echo "Info: Running PCIe DMA memory mapped write read test"
echo -e "\ttransfer size:  $transferSz, count: $transferCount"

# Write to all enabled h2cChannels in parallel
if [ $h2cChannels -gt 0 ]; then
	# Loop over four blocks of size $transferSz and write to them
	for ((i=0; i<=3; i++)); do
		addrOffset=$(($transferSz * $i))
		curChannel=$(($i % $h2cChannels))
	       	echo "Info: Writing to h2c channel $curChannel at address" \
		       "offset $addrOffset."
		$tool_path/dma_to_device -d /dev/${xid}_h2c_${curChannel} \
		       	-f data/datafile${i}_4K.bin -s $transferSz \
			-a $addrOffset -c $transferCount &
		# If all channels have active transactions we must wait for
	        # them to complete
		if [ $(($curChannel+1)) -eq $h2cChannels ]; then
			echo "Info: Wait for current transactions to complete."
			wait
		fi
	done
fi

# Wait for the last transaction to complete.
wait

# Read from all enabled c2hChannels in parallel
if [ $c2hChannels -gt 0 ]; then
	# Loop over four blocks of size $transferSz and read from them
	for ((i=0; i<=3; i++)); do
		addrOffset=$(($transferSz * $i))
		curChannel=$(($i % $c2hChannels))

		rm -f data/output_datafile${i}_4K.bin
		echo "Info: Reading from c2h channel $curChannel at " \
			"address offset $addrOffset."
		$tool_path/dma_from_device -d /dev/${xid}_c2h_${curChannel} \
		       	-f data/output_datafile${i}_4K.bin -s $transferSz \
		       	-a $addrOffset -c $transferCount &
		# If all channels have active transactions we must wait for
	        # them to complete
		if [ $(($curChannel+1)) -eq $c2hChannels ]; then
			echo "Info: Wait for current transactions to complete."
			wait
		fi
	done
fi

# Wait for the last transaction to complete.
wait

# Verify that the written data matches the read data if possible.
if [ $h2cChannels -eq 0 ]; then
	echo "Info: No data verification was performed because no h2c " \
		"channels are enabled."
elif [ $c2hChannels -eq 0 ]; then
	echo "Info: No data verification was performed because no c2h " \
		"channels are enabled."
else
	echo "Info: Checking data integrity."
	for ((i=0; i<=3; i++)); do
		cmp data/output_datafile${i}_4K.bin data/datafile${i}_4K.bin \
			-n $transferSz
		returnVal=$?
	       	if [ ! $returnVal == 0 ]; then
			echo "Error: The data written did not match the data" \
			       " that was read."
			echo -e "\taddress range: " \
				"$(($i*$transferSz)) - $((($i+1)*$transferSz))"
			echo -e "\twrite data file: data/datafile${i}_4K.bin"
			echo -e "\tread data file:  data/output_datafile${i}_4K.bin"
			testError=1
		else
			echo "Info: Data check passed for address range " \
				"$(($i*$transferSz)) - $((($i+1)*$transferSz))"
		fi
	done
fi

# Exit with an error code if an error was found during testing
if [ $testError -eq 1 ]; then
	echo "Error: Test completed with Errors."
	exit 1
fi

# Report all tests passed and exit
echo "Info: All PCIe DMA memory mapped tests passed."
exit 0
