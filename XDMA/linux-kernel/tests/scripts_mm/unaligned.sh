#!/bin/sh

source ./global.sh;									# 2024031500

##########################
#
# preset test parameters:
#
##########################
# dma io size
io_list="";     for (( i=${io_min_bit};i<=${io_max_bit};i++ )); do io_list="${io_list} $((1<<${i}))"        ; done;
io_max=$((1 << ${io_max_bit}));

# offset
offset_list=""; for (( i=${io_min_bit};i<=${io_max_bit};i++ )); do offset_list="${offset_list} $((1<<${i}))"; done;

# starting address
address=0

# delay(sleep) before moving on to the next channel, if applicable
delay=2

##########################
#
# main
#
##########################
if [ $# -lt 5 ]; then
	echo "$0: <xid> <h2c channel> <c2h channel> <data check> <dmesg log>"
	echo -e "\t<xid>: xdma<N>"
	echo -e "\t<h2c channel>: H2C channel #, 0-based"
	echo -e "\t<c2h channel>: C2H channel #, 0-based"
	echo -e "\t<data check>: read back the data and compare, 0|1"
        echo -e "\t<dmesg log>: log test info. into dmesg, 0|1"
	exit
fi

xid=$1
h2cno=$2
c2hno=$3
data_check=$4
dmesg=$5

tmpdir="/tmp/${xid}_h2c${h2cno}_c2h${c2hno}_unaligned"

echo "====>$0 $xid $h2cno:$c2hno, $data_check,$dmesg, $tmpdir"
if [ "$dmesg" -ne 0 ]; then
	echo "$0 $xid $h2cno:$c2hno, $tmpdir..." >> /dev/kmsg
fi

if [ ! -d "$tmpdir" ]; then
	mkdir -p $tmpdir
fi
rm -rf $tmpdir/*

# generate data file, minimum 64MB
cnt=$(($io_max / 1024))
if [ "$cnt" -eq "0" ]; then
	cnt=1
fi
datafile="$tmpdir/datafile-$cnt-K"
cnt=$(($cnt / 65536))
if [ "$cnt" -eq "0" ]; then
	cnt=1
fi
if [ ! -f "$datafile" ]; then
	echo "creating datafile: $datafile ..."
	let cnt=cnt+1
	dd if=/dev/urandom of=$datafile bs=64M count=$cnt iflag=fullblock
fi

echo
date

echo "====>$0: $xid $h2cno:$c2hno, addr $address ..." > /dev/kmsg
echo "$0: $xid $h2cno:$c2hno, addr $address ..." > /dev/kmsg

for io in $io_list; do
	for offset in $offset_list; do
		./io.sh $dmesg $data_check $io $address $offset $xid \
			$h2cno $c2hno $tmpdir $datafile
		if [ "$?" -ne "0" ]; then
			echo -e "\t$xid $h2cno:$c2hno, $io, off $offset FAILED!"
			exit 2
		fi
	done

	if [ "$delay" -ne "0" ]; then
		sleep $delay
	fi
done

date
echo "====>$0: $xid $h2cno:$c2hno, addr $address COMPLETED!"
exit 0
