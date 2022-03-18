#!/bin/bash

display_help() {
	echo -n "$0 <xdma id> <# ch> <io sz> < runtime> <iodir> <thread #> "
	echo "<logdir>"
	echo -e "\t<xdma id>: xdmaN"
	echo -e "\t<dir>: io direction <h2c|c2h|bi>"
	echo -e "\t<# ch>: fio --num_ch"
       	echo -e "\t<io sz>: fio --io_size"
	echo -e "\t<runtime>: fio --runtime"
	echo -e "\t<thread #>: fio --threads"
       	echo -e "\t<logdir>: log directory"
        exit;
}

####################
#
# main body
#
####################

if [ $# -ne 7 ]; then
	display_help
fi

xid=$1
iodir=$2
num_ch=$3
io_size=$4
runtime=$5
threads=$6
logdir=$7

outfile="${logdir}/fio_${io_size}_t${threads}.log"

exec_cmd=
op_cmd=
engine=sync
cmd_common="fio --allow_file_create=0 --ioengine=${engine} --zero_buffers"
cmd_common="$cmd_common --mem=mmap  --runtime=${runtime} --time_based"
for ((i = 0; i < num_ch; i++)); do
	if [ ${iodir} == bi ]; then
		op_cmd="${op_cmd} --name=write${i} --bs=${io_size}"
		op_cmd="${op_cmd} --size=${io_size} --offset=0 --rw=write"
		op_cmd="${op_cmd} --filename=/dev/xdma0_h2c_${i}"
		op_cmd="${op_cmd} --numjobs=${threads} --group_reporting"
		op_cmd="${op_cmd} --name=read${i} --bs=${io_size}"
		op_cmd="${op_cmd} --size=${io_size} --offset=0 --rw=read "
		op_cmd="${op_cmd} --filename=/dev/xdma0_c2h_${i}"
		op_cmd="${op_cmd} --numjobs=${threads} --group_reporting"
	elif [ ${iodir} == h2c ]; then
		op_cmd="${op_cmd} --name=write${i} --bs=${io_size}"
		op_cmd="${op_cmd} --size=${io_size} --offset=0 --rw=write"
		op_cmd="${op_cmd} --filename=/dev/xdma0_${iodir}_${i}"
		op_cmd="${op_cmd} --numjobs=${threads} --group_reporting"
	else
		op_cmd="${op_cmd} --name=read${i} --bs=${io_size}"
		op_cmd="${op_cmd} --size=${io_size} --offset=0 --rw=read"
		op_cmd="${op_cmd} --filename=/dev/xdma0_${iodir}_${i}"
		op_cmd="${op_cmd} --numjobs=${threads} --group_reporting"
	fi 
done


exec_cmd="${cmd_common}${op_cmd}"
echo -e "${exec_cmd}\n\n" > ${outfile}

${exec_cmd} >> ${outfile} &
pid=$!
wait $pid
