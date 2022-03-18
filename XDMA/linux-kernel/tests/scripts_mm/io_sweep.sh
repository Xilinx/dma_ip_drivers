#!/bin/sh

delay=5

if [ $# -lt 9 ]; then
	echo -ne "$0: <xid> <h2c channel> <c2h channel> <address> <offset> "
	echo "<io min> <io max> <data check> <dmesg log> [log dir]"
	echo -e "\t<xdma id>: xdma<N>"
	echo -e "\th2c channel: H2C channel #, 0-based"
	echo -e "\tc2h channel: C2H channel #, 0-based"
	echo -e "\t<address>: "
	echo -e "\t<offset>: "
	echo -e "\t<io min>,<io max>: dma size in byte, io size start from"
	echo -e "\t\tio_min, double each time until reaches io_max"
	echo -e "\t<data check>: read back the data and compare, 0|1"
	echo -e "\t<dmesg log>: log test info. into dmesg, 0|1"
	exit 1
fi

xid=$1
h2cno=$2
c2hno=$3
addr=$4
off=$5
io_min=$6
io_max=$7
data_check=$8
dmesg=$9

tmpdir="/tmp/${xid}_h2c${h2cno}c2h${c2hno}"

echo "====>$0 $xid $h2cno:$c2hno, $io_min~$io_max @$addr $off, $data_check, $tmpdir" 
if [ "$dmesg" -ne 0 ]; then
	echo "$0 $xid $h2cno:$c2hno, $io_min~$io_max @$addr $off, $tmpdir..." \
		>> /dev/kmsg
fi

if [ ! -d "$tmpdir" ]; then
	mkdir -p $tmpdir
fi
rm -rf ${tmpdir}/*

if [ "$data_check" -ne 0 ]; then
	cnt=$(($io_max / 1024))
	if [ "$cnt" -eq 0 ]; then
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
		dd if=/dev/urandom of=$datafile bs=64M count=$cnt \
			iflag=fullblock
	fi
fi

date

sz=$io_min
while [ "$sz" -le "$io_max" ]; do
	./io.sh $dmesg $data_check $sz $addr $off $xid $h2cno $c2hno $tmpdir \
	       	$datafile
	if [ "$?" -ne "0" ]; then
		#echo -e "\t$xid $h2cno:$c2hno, $sz FAILED" 
		exit 2
	fi

	if [ "$sz" -eq "$io_max" ]; then
		break
	fi

	sz=$(($sz * 2))
	if [ "$sz" -gt "$io_max" ]; then
		sz=$io_max
	fi

	if [ "$delay" -ne "0" ]; then
		sleep $delay
	fi
done

echo "====>$0 $xid $h2cno:$c2hno, $io_min~$io_max @$addr $off COMPLETED!" 
exit 0
