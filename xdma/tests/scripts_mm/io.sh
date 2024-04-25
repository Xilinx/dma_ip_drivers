#!/bin/sh

tool_path=../../tools
logdir=/tmp

if [ $# -lt 9 ]; then
	echo -ne "$0 <dmesg log 0|1> <data check 0|1> <sz> <address> <offset> "
	echo "<xid> <h2c channel> <c2h channel> <log dir> [data file]"
	echo -e "\t<dmesg log>: log test into dmesg"
	echo -e "\t<data check 0|1>: read data back and compare"
	echo -e "\t<sz>: dma transfer size"
	echo -e "\t<address>: "
	echo -e "\t<offset>: "
	echo -e "\t<xdma id>: xdma<N>"
	echo -e "\t<h2c channel>: dma h2c channel #, 0-based"
	echo -e "\t\tif >= 4, no traffic will be ran"
	echo -e "\t<c2h channel>: dma c2h channel #, 0-based"
	echo -e "\t               if channel # >= 4 NO dma will be performed"
	echo -e "\t\tif >= 4, no traffic will be ran"
	echo -e "\t<log dir>: temp. log directory"
	echo -e "\t[data file]: data file, size >= io size, "
	echo -e "\t             optional if <data check>=0"
	exit
fi

dmesg=$1
data_check=$2
sz=$3
address=$4
offset=$5
xid=$6
h2cno=$7
c2hno=$8
logdir=$9
if [ $# -gt 9 ]; then
	datafile=${10}
fi

if [ ! -d "$logdir" ]; then
	mkdir -p $logdir
fi

echo -en "\n===>$0 $xid, channel $h2cno:$c2hno, io $sz, addr $address, "
echo "off $offset, data: $datafile, integrity $data_check, dmesg $dmesg."

if [ "$h2cno" -ge 4 ] && [ "$c2hno" -ge 4 ]; then
	echo "$0: NO valid dma channel $h2cno:$c2hno"
	exit 1
fi

h2c_cmd="$tool_path/dma_to_device -d /dev/${xid}_h2c_${h2cno}"
c2h_cmd="$tool_path/dma_from_device -d /dev/${xid}_c2h_${c2hno}"
if [ "$address" -ne "0" ]; then
	h2c_cmd="$h2c_cmd -a $address"
	c2h_cmd="$c2h_cmd -a $address"
fi

if [ "$offset" -ne "0" ]; then
	h2c_cmd="$h2c_cmd -o $offset"
	c2h_cmd="$c2h_cmd -o $offset"
fi

if [ "$data_check" -ne 0 ]; then
	if [ -z "$datafile" ]; then
		echo "no datafile specified"
		exit 2
	fi
	if [ ! -s "$datafile" ]; then
		echo "missing datafile: $datafile ..."
		exit 3
	fi

	h2c_fname="$logdir/$xid-h2c${h2cno}-io$sz-o$offset-a$address.bin" 
	rm -f $h2c_fname
	h2c_cmd="$h2c_cmd -f $datafile -w $h2c_fname"

	c2h_fname="$logdir/$xid-c2h${c2hno}-io$sz-o$offset-a$address.bin"
	rm -f $c2h_fname
	c2h_cmd="$c2h_cmd -f $c2h_fname"
fi

if [ "$h2cno" -lt 4 ]; then
	if [ "$dmesg" -ne "0" ]; then
		echo "$h2c_cmd -s $sz -c 1" > /dev/kmsg
	fi

	echo "$h2c_cmd -s $sz -c 1 ..." > \
			${logdir}/h2c-io${sz}-o${offset}-a${address}.log
	out=`$h2c_cmd -s $sz -c 1`

	echo $out >> ${logdir}/h2c-io${sz}-o${offset}-a${address}.log
	if [ "$?" -ne "0" ]; then
		echo -e "\tH2C${h2cno}: io $sz, ERROR $?."
		exit 4
	fi
fi	

if [ "$c2hno" -lt 4 ]; then
	if [ "$dmesg" -ne "0" ]; then
		echo "$c2h_cmd -s $sz -c 1 ..." > /dev/kmsg
	fi

	echo "$c2h_cmd -s $sz -c 1 ..." > \
       		${logdir}/c2h-io${sz}-o${offset}-a${address}.log
	out=`./$c2h_cmd -s $sz -c 1`

	echo $out >> ${logdir}/c2h-io${sz}-o${offset}-a${address}.log
	if [ "$?" -ne "0" ]; then
		echo -e "\tC2H$channel: io $sz, ERROR $?."
		exit 5
	fi
fi	

if [ "$data_check" -eq 0 ]; then
	# no data integrity check needs to be done
	exit 0
fi

#md5sum $c2h_fname
#md5sum $h2c_fname
diff -q $c2h_fname $h2c_fname > /dev/null
if [ "$?" -eq "1" ]; then
	echo -e "\t$xid $h2cno:$c2hno: io $sz, addr $address, off $offset," \
		"data integrity FAILED!."
	exit 6
fi
echo -e "\t$xid $h2cno:$c2hno: io $sz, addr $address, off $offset, data match."
rm -f $c2h_fname $h2c_fname

exit 0
