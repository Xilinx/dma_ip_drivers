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
#
# This script is designed for general as well as stress testing. VF distruction can be done by appropriately
# configuring pf_nvf_lst.
# 
# AXI-MM Transfer
#	First H2C operation is performed for 1KBytes, this will write 1Kbytes of data to BRAM on card side. 
#	Then C2H operation is performed for 1KBytes. DMA reads data from BRAM and will transfer
#	to local file 'out_mm0_0', which will be compared to original file for correctness. 
#
# AXI-ST H2C Transfer
#	for H2C Streaming transfer data needs to be a per-defined data. 16 bit incremental data. 
#	Data file is provided with the script. 
#	H2C operation is performed, Data is read from Host memory and send to Card side. There is a data checker
#	on the card side which will check the data for correctness and will log the result in a register.
#	Script then read the register to check for results.
#	
#
# AXI-ST C2H Transfer
#	For C2H operation there is a data generator on the Card side which needs to be setup to generate data.
#	Qid, transfer length and number of paket are written before C2H transfer. Then 
#	C2H transfer is started by writing to register. C2H operation is completed and the data is written to 'out_st0_0"
#	file which then is compared to a per-defined data file. The data generator will only generate pre-defined 
#	data, so data comparison will need to be done with 'datafile_16bit_pattern.bin' file only.
#
# 


function print_help() {
	echo ""
	echo "Usage : $0 <bdf> <num_qs> <total_time> <overide_pf_nvf_lst> <log_file_suffix>"
	echo "Ex : $0 81000 256 00:01:00 0"
	echo "<bdf> : PF Bus device function in bbddf format ex:06000"
	echo ""
	echo "<num_qs> : number of queue from qid_start"	
	echo "           Default - 04 "
	echo ""
	echo "<total_time> : execution time in hh:mm:ss format"	
	echo "           Default - 00:01:00 "
	echo ""
	echo "<overide_pf_nvf_lst> : override the default pf_nvf_lst by reading sriov_totalvfs"	
	echo "           Default - 0 "
	echo ""
	#echo "<desc_byapss_en> : Enable desc bypass"
	#echo "           Default - 0 "
	#echo ""
	#echo "<pftch_en> : Enable prefetch"
	#echo "           Default - 0 "
	#echo ""
	#echo "<pftch_bypass_en> : Enable prefetch bypass"
	#echo "           Default - 0 "
	#echo ""
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
	exit 1
fi;
if [ $1 == "-h" ]; then
	print_help
	print_functionality
	exit 0
fi

log_file_suffix=default
total_time="00:01:00"
pf=$1
num_qs=2048
desc_byp=0
pftch=0
pftch_byp=0
declare -a pf_nvf_lst=(1 1 1 1)
overide_pf_nvf_lst=0
max_q_func=512
qmax_vfs=0
total_funcs=4
num_pfs=4
declare -a bdf_array=()
declare -a mm_q_lst=()
declare -a st_q_lst=()
declare -a qbase_array=()
declare -a num_queue_per_pf=()
declare -a full_bdf_array=()

if [ ! -z $2 ]; then
	num_qs=$2
fi
if [ ! -z $3 ]; then
	total_time=$3
fi
if [ ! -z $4 ]; then
	overide_pf_nvf_lst=$4
fi
if [ ! -z $5 ]; then
	log_file_suffix=$5
fi
if [ ! -z $6 ]; then #if arg4 is there byp enable
	desc_byp=$6
fi

if [ ! -z $7 ]; then #if arg5 is there pfetch enable
	pftch=$7
fi

if [ ! -z $8 ]; then #if arg6 is there pfetch byp enable
	pftch_byp=$8
fi
thrs=$((${total_time:0:2}))
tmins=$((${total_time:3:2}))
tsecs=$((${total_time:6:2}))
total_exec_time=$(( (60*60*thrs) + (60*tmins) + tsecs ))

infile='./datafile_16bit_pattern.bin'
stress_test_exit=0

function get_dev() {
	bdf_array=()
	qbase_array=()
	num_queue_per_pf=()
	full_bdf_array=()
	local dev_list="$(dmactl dev list | grep qdma)"
	while read -r line; do
		IFS=$'\t ,~' read -r -a array <<< "$line"	
		qdmadev=${array[0]}
		if [ "${qdmadev:0:6}" == "qdmavf" ]; then
			qdmavfbdf=${qdmadev:6:5}
			bdf_array+=("${qdmavfbdf}")
		else
			qdmabdf=${qdmadev:4:5}
			bdf_array+=("${qdmabdf}")
		fi
		full_bdf_array+=("${array[1]}")
		num_queue_per_pf+=("${array[4]}")
		qbase_array+=("${array[5]}")
 	done <<< "$dev_list"
}

function is_vf_bdf() {
	local lbdf=$1
	local lpci_device=$((0x${lbdf:2:2}))
	local lpci_func=$((0x${lbdf:4:1}))
	if [ ${lpci_device} -gt 0 -o ${lpci_func} -gt 3 ]; then
		echo 1
	else
		echo 0
	fi
}

function get_base_qid() {
	num_devs=${#bdf_array[@]}
	lq_base==-1
	for ((i = 0; i < num_devs; i++))
	do
		if [ ${bdf_array[i]} == $1 ]; then
			lq_base=$((${qbase_array[${i}]}))
			break
		fi
	done
	echo ${lq_base}
}

function queue_start() {
	qdmadev="qdma"
	is_vf=$(is_vf_bdf $1)
	if [ $is_vf -eq 1 ]; then
		qdmadev="qdmavf"
	fi
	echo -ne "setting up ${qdmadev}$1-$3-$2\033[0K\r"
	dmactl ${qdmadev}$1 q add idx $2 mode $3 dir bi >> ./run_stress_test_${log_file_suffix}.log 2>&1
	if [ $? -lt 0 ]; then
		echo "q add failed for ${qdmadev}$1-$3-$2"
		return
	fi

	if [ $desc_byp -eq 1 ] && [ $pftch -eq 0 ]; then
		if [ $pftch_byp -eq 0 ]; then
			dmactl ${qdmadev}$1 q start idx $2 dir bi desc_bypass_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		else
			dmactl ${qdmadev}$1 q start idx $2 dir bi desc_bypass_en pfetch_bypass_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		fi
	elif [ $desc_byp -eq 1 ] && [ $pftch -eq 1 ]; then
		if [ $pftch_byp -eq 0 ]; then
			dmactl ${qdmadev}$1 q start idx $2 dir bi desc_bypass_en pfetch_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		else
			dmactl ${qdmadev}$1 q start idx $2 dir bi desc_bypass_en pfetch_en pfetch_bypass_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		fi
	elif [ $desc_byp -eq 0 ] && [ $pftch -eq 1 ] ; then
		if [ $pftch_byp -eq 0 ]; then
			dmactl ${qdmadev}$1 q start idx $2 dir bi pfetch_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		else
			dmactl ${qdmadev}$1 q start idx $2 dir bi pfetch_en pfetch_bypass_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		fi
	else
		if [ $pftch_byp -eq 0 ]; then
			dmactl ${qdmadev}$1 q start idx $2 dir bi>> ./run_stress_test_${log_file_suffix}.log 2>&1
		else
			dmactl ${qdmadev}$1 q start idx $2 dir bi pfetch_bypass_en >> ./run_stress_test_${log_file_suffix}.log 2>&1
		fi	
	fi
	if [ $? -lt 0 ]; then
		echo "q start failed for ${qdmadev}$1-$3-$2"
		dmactl ${qdmadev}$1 q del idx $2 dir bi >> ./run_stress_test_${log_file_suffix}.log 2>&1
		return $?
	fi
	if [ $3 == mm ]; then
		mm_q_lst+=("$1-$2")
	else
		st_q_lst+=("$1-$2")
	fi
	return 0
}

function cleanup_queue() {
	qdmadev="qdma"
	is_vf=$(is_vf_bdf $1)
	if [ $is_vf -eq 1 ]; then
		qdmadev="qdmavf"
	fi
	echo -ne "cleaning up ${qdmadev}$1-$2\033[0K\r"
	dmactl ${qdmadev}$1 q stop idx $2 dir bi >> ./run_stress_test_${log_file_suffix}.log 2>&1
	dmactl ${qdmadev}$1 q del idx $2 dir bi >> ./run_stress_test_${log_file_suffix}.log 2>&1

}


# Find user bar
function get_user_bar() {
    bdf=$1
	tmp=`dmactl qdma${bdf} reg read bar 0 0x10C | grep "0x10c" | cut -d '=' -f2 | cut -d 'x' -f2 | cut -d '.' -f1`
	bar_ext=$(printf '%x\n' "$(( 0x$tmp & 0x00000f ))")

	if [ $bar_ext -eq 2 ]; then
		usr_bar=1
	elif [ $bar_ext -eq 4 ];then
		usr_bar=2
	fi
}

function get_env() {
	if [ ${overide_pf_nvf_lst} -ne 0 ]; then
		pf_nvf_lst=()
		for bdf in ${bdf_array[@]}
		do
			pci_bus=${bdf:0:2}
			pci_device=${bdf:2:2}
			pci_func=${bdf:4:1}
			nvf=$(cat /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/sriov_totalvfs)
			pf_nvf_lst+=("${nvf}")
		done
	fi
	total_funcs=$num_pfs
	for (( i = 0; i < ${num_pfs}; i++))
	do
		num_vf=${pf_nvf_lst[$i]}
		total_funcs=$((total_funcs+num_vf))
	done
	if [ $total_funcs -gt $num_qs ]; then
		echo "ERROR: num_qs less than total functions. Min queues required are $total_funcs"
		exit 1
	fi
	max_q_func=$((num_qs/total_funcs))
}

function start_vfs() {
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}

		num_vfs=${pf_nvf_lst[${pci_func}]}
		echo "echo ${num_vfs} > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/sriov_numvfs"
		echo ${num_vfs} > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/sriov_numvfs
		if [ $? -ne 0 ]; then
			echo "enabling vfs failed for 0000\:${pci_bus}\:${pci_device}.${pci_func}/virtfn${i}"
		fi
	done
	sleep 1 # allow all probes to finish
}

function set_pf_qmax() {
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}
		echo -ne "echo ${max_q_func} > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/qdma/qmax\033[0K\r"
		echo ${max_q_func} > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/qdma/qmax
		if [ $? -lt 0 ]; then
			echo "set_qmax failed for 0000\:${pci_bus}\:${pci_device}.${pci_func}"
		fi
		sleep .1
		
	done
	echo ""
}

function set_vf_qmax() {
	total_funcs=${#bdf_array[@]}
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}
		num_vfs=${pf_nvf_lst[${pci_func}]}
		for (( i = 0; i < num_vfs; i++ ))
		do
			echo -ne "echo ${max_q_func} > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/virtfn${i}/qdma/qmax\033[0K\r"
			echo ${max_q_func} > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/virtfn${i}/qdma/qmax
			if [ $? -lt 0 ]; then
				echo "set_qmax failed for 0000\:${pci_bus}\:${pci_device}.${pci_func}/virtfn${i}"
			fi
			sleep .1
		done
	done
	echo ""
}

function queue_init() {
	mode=mm
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}
		for (( qid = 0; qid < max_q_func; qid++ ))
		do
			if [ $mode == mm ]; then
				mode=st
			else
				mode=mm
			fi
			queue_start $bdf $qid $mode
		done
	done
	echo ""
}

function queue_exit() {
	echo "exit sequence..."
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}
		for (( qid = 0; qid < max_q_func; qid++ ))
		do
			cleanup_queue $bdf $qid
		done
	done
	echo ""
	echo "Device Stats:" > ./run_stress_test_stat_${log_file_suffix}.log
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}
		qdmadev="qdma"
		is_vf=$(is_vf_bdf $bdf)
		if [ $is_vf -ne 0 ]; then
			qdmadev="qdmavf"
		fi
		echo "${qdmadev}${bdf} Stats:" >> ./run_stress_test_stat_${log_file_suffix}.log
		dmactl ${qdmadev}${bdf} stat >> ./run_stress_test_stat_${log_file_suffix}.log
		echo "---------------------------------------------------------" >> ./run_stress_test_stat_${log_file_suffix}.log
	done
	for bdf in ${bdf_array[@]}
	do
		pci_bus=${bdf:0:2}
		pci_device=${bdf:2:2}
		pci_func=${bdf:4:1}
		is_vf=$(is_vf_bdf $bdf)
		if [ $is_vf -eq 0 ]; then
			echo "echo 0 > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/sriov_numvfs"
			echo 0 > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/sriov_numvfs
		fi
	done
}

function setup_env() {
	get_dev
	num_pfs=${#bdf_array[@]}
	get_env
	start_vfs
	set_pf_qmax
	set_vf_qmax
	get_dev
	dmactl dev list
	get_user_bar $pf
}

function monitor_exec_time() {
	flag=1
	MON_STARTTIME=$(date +%s)
	monelapsed_time=0
	while [ true ]; do
		if [ $flag -eq 1 ]; then
			echo -ne "elapsed time: $monelapsed_time secs...\033[0K\r"
			flag=0
		else
			echo -ne "elapsed time: $monelapsed_time secs..|\033[0K\r"
			flag=1
		fi
		MON_CURTIME=$(date +%s)
		monelapsed_time=$((MON_CURTIME - MON_STARTTIME))
		if [ $monelapsed_time -gt $total_exec_time ]; then
			stress_test_exit=1
			return
		else
			sleep 1
		fi
	done
	echo ""
	echo "$1-completed"
}

function run_mm_h2c_c2h() {
	MM_STARTTIME=$(date +%s)
	while [ true ]
	do 
		echo "***********************************************" 2>&1 >> ./run_mm_stress_test_${log_file_suffix}.log
		local size=$(( ($RANDOM % 262144) +1 ))
		total_qs=${#mm_q_lst[@]}
		for ((i=0; i < total_qs; i++)); do
			mm_q_lst_entry=${mm_q_lst[${i}]}
			bdf=${mm_q_lst_entry:0:5}
			strlen=$(echo -n $mm_q_lst_entry | wc -m)
			end_idx=$((strlen-1))
			qid=${mm_q_lst_entry:6:${end_idx}}
			qdmadev="qdma"
			is_vf=$(is_vf_bdf $bdf)
			if [ $is_vf -eq 1 ]; then
				qdmadev="qdmavf"
			fi
			
			echo "AXI-MM for Func ${bdf} Start" 2>&1 >> ./run_mm_stress_test_${log_file_suffix}.log
			# Setup for Queues
			dev_mm_c2h="/dev/${qdmadev}${bdf}-MM-$qid"
			dev_mm_h2c="/dev/${qdmadev}${bdf}-MM-$qid"

			out_mm="/tmp/out_mm${bdf}_$qid"
			if [ $desc_byp -eq 1 ]; then
				dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x090 3 >> ./run_mm_stress_test_${log_file_suffix}.log 2>&1
			fi
			# H2C transfer 
			dma_to_device -d $dev_mm_h2c -f $infile -s $size >> ./run_mm_stress_test_${log_file_suffix}.log 2>&1

			# C2H transfer
			dma_from_device -d $dev_mm_c2h -f $out_mm -s $size >> ./run_mm_stress_test_${log_file_suffix}.log 2>&1
			
			# Compare file for correctness
			cmp $out_mm $infile -n $size
			if [ $? -eq 1 ]; then
				echo "#### Test ERROR. Queue ${qdmadev}${bdf}-$qid data did not match - iosz - ${size} ####" >> ./run_mm_stress_test_${log_file_suffix}.log 2>&1
				#dmactl ${qdmadev}${bdf} q dump idx $qid >> ./run_stress_test_${log_file_suffix}.log 2>&1
				#dmactl ${qdmadev}${bdf} reg dump >> ./run_stress_test_${log_file_suffix}.log 2>&1
			else
				echo "**** Test pass. Queue ${qdmadev}${bdf}-$qid" >> ./run_mm_stress_test_${log_file_suffix}.log 2>&1
			fi
			echo "AXI-MM for Func ${bdf} End" 2>&1 >> ./run_mm_stress_test_${log_file_suffix}.log
			MM_CURTIME=$(date +%s)
			mmelapsed_time=$((MM_CURTIME - MM_STARTTIME))
			if [ $mmelapsed_time -gt $total_exec_time ]; then
				echo "exiting run_mm_h2c_c2h()"
				return
			fi
		done
		echo "***********************************************" 2>&1 >> ./run_mm_stress_test_${log_file_suffix}.log
	done
}



function run_st_h2c() {
	# AXI-ST H2C transfer
	STH2C_STARTTIME=$(date +%s)
	while [ true ]
	do 
		echo "***********************************************" 2>&1 >> ./run_st_h2c_stress_test_${log_file_suffix}.log
		local size=$(( ($RANDOM % 28672) +1 ))
		total_qs=${#st_q_lst[@]}
		for ((i=0; i < total_qs; i++)); do
			st_q_lst_entry=${st_q_lst[${i}]}
			bdf=${st_q_lst_entry:0:5}
			strlen=$(echo -n $st_q_lst_entry | wc -m)
			end_idx=$((strlen-1))
			qid=${st_q_lst_entry:6:${end_idx}}
			qdmadev="qdma"
			is_vf=$(is_vf_bdf $bdf)
			if [ $is_vf -eq 1 ]; then
				qdmadev="qdmavf"
			fi
			dev_st_h2c="/dev/${qdmadev}${bdf}-ST-$qid"
			echo "AXI-ST H2C for Func ${bdf}-${qid} Start" 2>&1 >> ./run_st_h2c_stress_test_${log_file_suffix}.log

			# Clear H2C match from previous runs. this register is in card side.
			# MAtch clear register is on offset 0x0C 
			dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x0C 0x1 >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1 # clear h2c Match register.

			if [ $desc_byp -eq 1 ]; then
				dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x090 3 >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1
			fi

			# do H2C Transfer
			dma_to_device -d $dev_st_h2c -f $infile -s $size >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1

			# check for H2C data match. MAtch register is in offset 0x10.
			pass=`dmactl ${qdmadev}${bdf} reg read bar $usr_bar 0x10 | grep "0x10" | cut -d '=' -f2 | cut -d 'x' -f2 | cut -d '.' -f1`
			# convert hex to bin
			code=`echo $pass | tr 'a-z' 'A-Z'`
 	 		val=`echo "obase=2; ibase=16; $code" | bc`
			check=1
			if [ $(($val & $check)) -eq 1 ];then
				echo "*** Test passed for Queue $qid" >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1
			else
				echo "#### ERROR Test failed for ${qdmadev}${bdf}-${qid}. pattern did not match - iosz - ${size} ####" >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1
				#dmactl ${qdmadev}${bdf} q dump idx $qid >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1
				#dmactl ${qdmadev}${bdf} reg dump >> ./run_st_h2c_stress_test_${log_file_suffix}.log 2>&1
			fi
			echo "AXI-ST H2C for Func ${bdf}-${qid} End" 2>&1 >> ./run_st_h2c_stress_test_${log_file_suffix}.log
			STH2C_CURTIME=$(date +%s)
			sth2celapsed_time=$((STH2C_CURTIME - STH2C_STARTTIME))
			if [ $sth2celapsed_time -gt $total_exec_time ]; then
				echo "exiting run_st_h2c()"
				return
			fi
		done
		echo "***********************************************" 2>&1 >> ./run_st_h2c_stress_test_${log_file_suffix}.log
	done

}

function run_st_c2h() {
	STC2H_STARTTIME=$(date +%s)
	while [ true ]
	do
		echo "***********************************************" 2>&1 >> ./run_st_c2h_stress_test_${log_file_suffix}.log
		local size=$(( ($RANDOM % 28672) +1 ))
		local num_pkt=$(( ($RANDOM % 7) +1 ))
		size=$((size+(size %2)))
		total_qs=${#st_q_lst[@]}
		for ((i = 0; i < total_qs; i++)); do
			STC2H_CURTIME=$(date +%s)
			stc2helapsed_time=$((STC2H_CURTIME - STC2H_STARTTIME))
			if [ $stc2helapsed_time -gt $total_exec_time ]; then
				echo "exiting run_st_c2h()"
				return
			fi
			st_q_lst_entry=${st_q_lst[${i}]}
			bdf=${st_q_lst_entry:0:5}
			strlen=$(echo -n $st_q_lst_entry | wc -m)
			end_idx=$((strlen-1))
			qid=${st_q_lst_entry:6:${end_idx}}
			qdmadev="qdma"
			is_vf=$(is_vf_bdf $bdf)
			if [ $is_vf -eq 1 ]; then
				qdmadev="qdmavf"
			fi
			echo "AXI-ST C2H for Func ${bdf} Start" 2>&1 >> ./run_st_c2h_stress_test_${log_file_suffix}.log
			q_base=`get_base_qid ${bdf}`
			echo "q_base=${q_base}">> ./run_st_c2h_stress_test_${log_file_suffix}.log
			if [ ${q_base} -lt 0 ]; then
				echo "run_st_c2h: could not find q base for $bdf"
				continue
			fi
			hw_qid=$((qid + q_base))
			dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x0 ${hw_qid}  >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1

			dev_st_c2h="/dev/${qdmadev}${bdf}-ST-$qid"
			out_st="/tmp/out_st${bdf}_$qid"
			let "tsize= $size*$num_pkt" # if more packets are requested.

			#cache bypass	
			if [ $desc_byp -eq 1 ] && [ $pftch_byp -eq 0 ] ; then
				dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x090 2 >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
			fi

			#simple bypass
			if [ $desc_byp -eq 1 ] && [ $pftch_byp -eq 1 ] ; then
				dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x090 4 >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
			fi

			#no bypass
			if [ $desc_byp -eq 0 ] && [ $pftch_byp -eq 0 ] ; then
				dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x090 0 >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
			fi

			# Write transfer size to offset 0x04
			dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x4 $size >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1

			# Write number of packets to offset 0x20
			dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x20 $num_pkt >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1 

			# Write to offset 0x80 bit [1] to trigger C2H data generator. 
			dmactl ${qdmadev}${bdf} reg write bar $usr_bar 0x08 2 >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1

			# do C2H transfe
			dma_from_device -d $dev_st_c2h -f $out_st -s $tsize >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1

			cmp $out_st $infile -n $tsize>> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
			if [ $? -eq 1 ]; then
				echo "#### Test ERROR. Queue ${qdmadev}${bdf}-$qid data did not match - iosz/numpkt - ${size}/${num_pkt} ####" >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
				#dmactl ${qdmadev}${bdf} q dump idx $qid dir c2h >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
				#dmactl ${qdmadev}${bdf} reg dump >> ./run_stress_test_${log_file_suffix}.log 2>&1
			else
				echo "**** Test pass. Queue $qid" >> ./run_st_c2h_stress_test_${log_file_suffix}.log 2>&1
			fi
			echo "AXI-ST C2H for Func ${bdf} End" 2>&1 >> ./run_st_c2h_stress_test_${log_file_suffix}.log
		done
		echo "***********************************************" 2>&1 >> ./run_st_c2h_stress_test_${log_file_suffix}.log
	done
}

trap queue_exit EXIT
echo "setting up environment"
setup_env
queue_init

echo "transfer tests will run for $total_exec_time secs"
echo "starting transfers"

echo "###############################################################" > "run_mm_stress_test_${log_file_suffix}.log"
echo "QDMA MM Stress Test on All Functions Starts" >> "run_mm_stress_test_${log_file_suffix}.log"
echo "###############################################################" >> "run_mm_stress_test_${log_file_suffix}.log"
run_mm_h2c_c2h &

echo "###############################################################" > "run_st_h2c_stress_test_${log_file_suffix}.log"
echo "QDMA ST H2C Stress Test on All Functions Starts" >> "run_st_h2c_stress_test_${log_file_suffix}.log"
echo "###############################################################" >> "run_st_h2c_stress_test_${log_file_suffix}.log"
run_st_h2c &

echo "###############################################################" > "run_st_c2h_stress_test_${log_file_suffix}.log"
echo "QDMA ST C2H Stress Test on All Functions Starts" >> "run_st_c2h_stress_test_${log_file_suffix}.log"
echo "###############################################################" >> "run_st_c2h_stress_test_${log_file_suffix}.log"
run_st_c2h &

monitor_exec_time
wait

function print_errs() {
	while read -r line; do
		echo $line
 	done <<< "$1"
}

mm_errs=$(cat run_mm_stress_test_${log_file_suffix}.log | grep "Error\|ERROR\|FAIL\|Fail")
st_h2c_errs=$(cat run_st_h2c_stress_test_${log_file_suffix}.log | grep "Error\|ERROR\|FAIL\|Fail")
st_c2h_errs=$(cat run_st_c2h_stress_test_${log_file_suffix}.log | grep "Error\|ERROR\|FAIL\|Fail")
if [ "${mm_errs}" != "" ]; then
	echo "MM Errors:"
fi
print_errs "${mm_errs}"
if [ "${st_h2c_errs}" != "" ]; then
	echo "ST H2C Errors:"
fi
print_errs "${st_h2c_errs}"
if [ "${st_c2h_errs}" != "" ]; then
	echo "ST C2H Errors:"
fi
print_errs "${st_c2h_errs}"

echo ""

exit 0
