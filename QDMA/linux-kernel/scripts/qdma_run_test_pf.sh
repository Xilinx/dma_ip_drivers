#!/bin/bash
#
# Simple run script to test QDMA in AXI-MM and AXI-St mode.
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
	echo "Usage : $0 <bdf> <qid_start> <num_qs> <desc_bypass_en> <pftch_en> <pfetch_bypass_en> <flr_on>"
	echo "Ex : $0 06000 0 4 1 1 1 1"
	echo "<bdf> : PF Bus device function in bbddf format ex:06000"
	echo ""
	echo "<qid_start> : qid start"
	echo ""
	echo "<num_qs> : number of queue from qid_start"	
	echo "           Default - 04 "
	echo ""
	echo "<desc_byapss_en> : Enable desc bypass"
	echo "           Default - 0 "
	echo ""
	echo "<pftch_en> : Enable prefetch"
	echo "           Default - 0 "
	echo ""
	echo "<pftch_bypass_en> : Enable prefetch bypass"
	echo "           Default - 0 "
	echo ""
	echo "<flr_on> : Apply Function Level Reset"
	echo "           Default - 0 "
	echo ""
	echo ""
    echo ""
    exit 1
}

if [ $# -lt 2 ]; then
	echo "Invalid arguements."
	print_help
	exit;
fi;

pf=$1
qid_start=$2
num_qs=4
desc_byp=0
pftch=0
pftch_byp=0
flr_on=0

if [ ! -z $3 ]; then
	num_qs=$3
fi

if [ ! -z $4 ]; then #if arg4 is there byp enable
	desc_byp=$4
fi

if [ ! -z $5 ]; then #if arg5 is there pfetch enable
	pftch=$5
fi

if [ ! -z $6 ]; then #if arg6 is there pfetch byp enable
	pftch_byp=$6
fi
if [ ! -z $6 ]; then #if arg7 is there FLR enable
	flr_on=$7
fi

echo "$pf $qid_start $num_qs $desc_byp $pftch $pftch_byp"
size=1024
num_pkt=1 #number of packets not more then 64
infile='./datafile_16bit_pattern.bin'
declare -a bypass_mode_lst=(NO_BYPASS_MODE DESC_BYPASS_MODE CACHE_BYPASS_MODE SIMPLE_BYPASS_MODE)


function get_dev () {
	pf_list="$(dma-ctl dev list | grep qdma | grep -v qdmavf)"
	echo "$pf_list"
	while read -r line; do
		IFS=$'\t ,~' read -r -a array <<< "$line"	
		qdmabdf=${array[0]}
		bdf_array+=("${qdmabdf#*a}")
		full_bdf_array+=("${array[1]}")
		num_queue_per_pf+=("${array[4]}")
		qbase_array+=("${array[5]}")
 	done <<< "$pf_list"

}

function set_flr() { 
	echo "Applying function level reset"
	for pf_bdf in ${bdf_array[@]}; do
		pci_bus=${pf_bdf:0:2}
		pci_device=${pf_bdf:2:2}
		pci_func=${pf_bdf:4:1}
		echo "echo 1 > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/reset"
		echo 1 > /sys/bus/pci/devices/0000\:${pci_bus}\:${pci_device}.${pci_func}/reset
	done
}

function set_bypass_mode() {
	dev=$1
	mode=$2
	dir=$3
	bypass=$4
    local reg_val=0x00;
	
    if [ $mode == mm ]; then
    	case $dir in
    		h2c)
	        	if [ $bypass == DESC_BYPASS_MODE ]; then
	        		echo "setting DESC_BYPASS_MODE for ${mode}-$dir"
	        		reg_val=0x1;
	        	else
	        		reg_val=0x0;
	        	fi

        		;;
    		c2h)
	        	if [ $bypass == DESC_BYPASS_MODE ]; then
	        		echo "setting DESC_BYPASS_MODE for ${mode}-$dir"
	        		reg_val=0x2;
	        	else
	        		reg_val=0x0;
	        	fi

	        	;;
    		bi)
	        	if [ $bypass == DESC_BYPASS_MODE ]; then
	        		echo "setting DESC_BYPASS_MODE for ${mode}-$dir"
	        		reg_val=0x3;
	        	else
	        		reg_val=0x0;
				fi
	        	;;
        esac
    else
        case $dir in
			h2c)
				case $bypass in
					CACHE_BYPASS_MODE)
						echo "setting CACHE_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x1;
						;;
					SIMPLE_BYPASS_MODE)
						echo "setting SIMPLE_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x1;
						;;
					*)
						echo "setting NO_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x00;
						;;
					esac
				;;
			c2h)
				case $bypass in
					CACHE_BYPASS_MODE)
						echo "setting CACHE_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x2;
						;;
					SIMPLE_BYPASS_MODE)
						echo "setting SIMPLE_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x4;
						;;
					*)
						echo "setting NO_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x00;
						;;
				esac
				;;
			bi)
				case $bypass in
					CACHE_BYPASS_MODE)
						echo "setting CACHE_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x3;
						;;
					SIMPLE_BYPASS_MODE)
						echo "setting SIMPLE_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x5;
						;;
					*)
						echo "setting NO_BYPASS_MODE for ${mode}-$dir"
						reg_val=0x00;
						;;
				esac
				;;
		esac
	fi
    dma-ctl qdma$dev reg write bar 2 0x90 $reg_val
}

function get_bypass_mode() {
	byp_mode=0
	if [ $1 == mm ]; then
		if [ $desc_byp -eq 1 ]; then
			byp_mode=1
		fi
	else
		if [ $desc_byp -eq 1 ] && [ $pftch_byp -eq 0 ]; then
			byp_mode=2
		elif [ $desc_byp -eq 1 ] && [ $pftch_byp -eq 1 ]; then
			byp_mode=3
		fi
	fi
	echo $byp_mode
}

function queue_start() {
	echo "setting up qdma$1-$3-$2"
	dma-ctl qdma$1 q add idx $2 mode $3 dir $4 >> ./run_pf.log 2>&1
	if [ $? -ne 0 ]; then
		echo "q add failed for qdma$1-$3-$2"
		return
	fi
	bypass_mode=$(get_bypass_mode $3 $4)
	set_bypass_mode $1 $3 $4 ${bypass_mode_lst[${bypass_mode}]}
	if [ $3 == mm -o $4 == h2c ]; then
		if [ $desc_byp -eq 1 ]; then
			dma-ctl qdma$1 q start idx $2 dir $4 desc_bypass_en >> ./run_pf.log 2>&1
		else
			dma-ctl qdma$1 q start idx $2 dir $4 >> ./run_pf.log 2>&1
		fi
	else
		if [ $desc_byp -eq 1 ] && [ $pftch -eq 0 ]; then
			if [ $pftch_byp -eq 0 ]; then
				dma-ctl qdma$1 q start idx $2 dir $4 desc_bypass_en >> ./run_pf.log 2>&1
			else
				dma-ctl qdma$1 q start idx $2 dir $4 desc_bypass_en pfetch_bypass_en >> ./run_pf.log 2>&1
			fi
		elif [ $desc_byp -eq 1 ] && [ $pftch -eq 1 ]; then
			if [ $pftch_byp -eq 0 ]; then
				dma-ctl qdma$1 q start idx $2 dir $4 desc_bypass_en pfetch_en >> ./run_pf.log 2>&1
			else
				dma-ctl qdma$1 q start idx $2 dir $4 desc_bypass_en pfetch_en pfetch_bypass_en >> ./run_pf.log 2>&1
			fi
		elif [ $desc_byp -eq 0 ] && [ $pftch -eq 1 ] ; then #
			if [ $pftch_byp -eq 0 ]; then
				dma-ctl qdma$1 q start idx $2 dir $4 pfetch_en >> ./run_pf.log 2>&1
			else
				echo "Invalid case of bypass mode" >> ./run_pf.log 2>&1
				dma-ctl qdma$1 q del idx $2 dir bi >> ./run_pf.log 2>&1
				return 1
			fi
		else
			if [ $pftch_byp -eq 0 ]; then
				dma-ctl qdma$1 q start idx $2 dir $4>> ./run_pf.log 2>&1
			else
				echo "Invalid case of bypass mode" >> ./run_pf.log 2>&1
				dma-ctl qdma$1 q del idx $2 dir bi >> ./run_pf.log 2>&1
				return 1
			fi	
		fi
	fi
	if [ $? -ne 0 ]; then
		echo "q start failed for qdma$1-$3-$2-$4"
		dma-ctl qdma$1 q del idx $2 dir bi >> ./run_pf.log 2>&1
		return $?
	fi
	
	
	return 0
}

function cleanup_queue() {
	echo "cleaning up qdma$1-$3-$2"
        dma-ctl qdma$1 q stop idx $2 dir $4 >> ./run_pf.log 2>&1
        dma-ctl qdma$1 q del idx $2 dir $4 >> ./run_pf.log 2>&1

}


# Find AXI Master Lite bar
function get_user_bar () {
        local pf_bdf=$1
	tmp=`dma-ctl qdma$pf_bdf reg read bar 0 0x10C | grep "0x10c" | cut -d '=' -f2 | cut -d 'x' -f2 | cut -d '.' -f1`
	bar_ext=$(printf '%x\n' "$(( 0x$tmp & 0x00000f ))")

	if [ $bar_ext -eq 2 ]; then
		usr_bar=1
	elif [ $bar_ext -eq 4 ];then
		usr_bar=2
	fi
}


function run_mm_h2c_c2h () {
	for pf_bdf in ${bdf_array[@]}; do 
		echo "***********************************************" 2>&1 | tee -a ./run_pf.log
		echo "AXI-MM for Func $pf_bdf Start" 2>&1 | tee -a ./run_pf.log
		get_user_bar $pf_bdf
		for ((i=$qid_start; i < (($qid_start + $num_qs)); i++)); do
			# Setup for Queues
			qid=$i
			dev_mm_c2h="/dev/qdma$pf_bdf-MM-$qid"
			dev_mm_h2c="/dev/qdma$pf_bdf-MM-$qid"

			out_mm="/tmp/out_mm"$pf_bdf"_"$qid
			# Open the Queue for AXI-MM streaming interface.
			queue_start $pf_bdf $qid mm bi
			if [ $? -ne 0 ]; then
				echo "q setup for qdma$pf_bdf-MM-$qid failed"
				continue
			fi
			echo "setup for qdma$pf_bdf-MM-$qid done"
			# H2C transfer 
			dma-to-device -d $dev_mm_h2c -f $infile -s $size >> ./run_pf.log 2>&1

			# C2H transfer
			dma-from-device -d $dev_mm_c2h -f $out_mm -s $size >> ./run_pf.log 2>&1
			
			# Compare file for correctness
			cmp $out_mm $infile -n $size
			if [ $? -eq 1 ]; then
				echo "#### Test ERROR. Queue $qid data did not match ####"
				dma-ctl qdma$pf_bdf q dump idx $qid >> ./run_pf.log 2>&1
				dma-ctl qdma$pf_bdf reg dump >> ./run_pf.log 2>&1
			else
				echo "**** Test pass. Queue $qid"
			fi
			# Close the Queues
			cleanup_queue $pf_bdf $qid st bi
			echo "-----------------------------------------------"
		done
		echo "AXI-MM for Func $pf_bdf End" 2>&1 | tee -a ./run_pf.log
		echo "***********************************************" 2>&1 | tee -a ./run_pf.log
	done
}



function run_st_h2c () {

	# AXI-ST H2C transfer
	for pf_bdf in "${bdf_array[@]}"; do 
		echo "***********************************************" 2>&1 | tee -a ./run_pf.log
		echo "AXI-ST H2C for Func $pf_bdf Start" 2>&1 | tee -a ./run_pf.log
		get_user_bar $pf_bdf
		for ((i=$qid_start; i < (($qid_start + $num_qs)); i++)); do
			# Setup for Queues
			qid=$i
			queue_start $pf_bdf $qid st h2c # open the Queue for AXI-ST streaming interface.

			dev_st_h2c="/dev/qdma$pf_bdf-ST-$qid"

			# Clear H2C match from previous runs. this register is in card side.
			# MAtch clear register is on offset 0x0C 
			dma-ctl qdma$pf_bdf reg write bar $usr_bar 0x0C 0x1 >> ./run_pf.log 2>&1 # clear h2c Match register.

			# do H2C Transfer
			dma-to-device -d $dev_st_h2c -f $infile -s $size >> ./run_pf.log 2>&1

			if [ $? -ne 0 ]; then
				echo "#### ERROR Test failed. Transfer failed ####"
				cleanup_queue $pf_bdf $qid st h2c
				continue
			fi
			# check for H2C data match. MAtch register is in offset 0x10.
			pass=`dma-ctl qdma$pf_bdf reg read bar $usr_bar 0x10 | grep "0x10" | cut -d '=' -f2 | cut -d 'x' -f2 | cut -d '.' -f1`
			# convert hex to bin
			code=`echo $pass | tr 'a-z' 'A-Z'`
 	 		val=`echo "obase=2; ibase=16; $code" | bc`
			check=1
			if [ $(($val & $check)) -eq 1 ];then
				echo "*** Test passed for Queue $qid"
			else
				echo "#### ERROR Test failed. pattern did not match ####"
				dma-ctl qdma$pf_bdf q dump idx $qid >> ./run_pf.log 2>&1
				dma-ctl qdma$pf_bdf reg dump >> ./run_pf.log 2>&1
			fi
			cleanup_queue $pf_bdf $qid st h2c
			echo "-----------------------------------------------"
		done
		echo "AXI-ST H2C for Func $pf_bdf End" 2>&1 | tee -a ./run_pf.log
		echo "***********************************************" 2>&1 | tee -a ./run_pf.log
	done

}

function run_st_c2h () {
	local pf=0

	for pf_bdf in "${bdf_array[@]}"; do 

		echo "***********************************************" 2>&1 | tee -a ./run_pf.log
		echo "AXI-ST C2H for Func $pf_bdf Start" 2>&1 | tee -a ./run_pf.log

		get_user_bar $pf_bdf

		for ((i=$qid_start; i < (($qid_start + $num_qs)); i++)); do
			# Setup for Queues
			qid=$i
			out_st="/tmp/out_st"$pf_bdf"_"$qid

			# Each PF is assigned with 32 Queues. PF0 has queue 0 to 31, PF1 has 32 to 63 
			# Write QID in offset 0x00 
			hw_qid=$(($qid + ${qbase_array[$pf]} ))
			dma-ctl qdma$pf_bdf reg write bar $usr_bar 0x0 $hw_qid  >> ./run_pf.log 2>&1

			# open the Queue for AXI-ST streaming interface.
			queue_start $pf_bdf $qid st c2h
	
			dev_st_c2h="/dev/qdma$pf_bdf-ST-$qid"
			let "tsize= $size*$num_pkt" # if more packets are requested.

			# Write transfer size to offset 0x04
			dma-ctl qdma$pf_bdf reg write bar $usr_bar 0x4 $size >> ./run_pf.log 2>&1
	
			# Write number of packets to offset 0x20
			dma-ctl qdma$pf_bdf reg write bar $usr_bar 0x20 $num_pkt >> ./run_pf.log 2>&1 

			# Write to offset 0x80 bit [1] to trigger C2H data generator. 
			dma-ctl qdma$pf_bdf reg write bar $usr_bar 0x08 2 >> ./run_pf.log 2>&1

			# do C2H transfer 
			dma-from-device -d $dev_st_c2h -f $out_st -s $tsize >> ./run_pf.log 2>&1
			if [ $? -ne 0 ]; then
				echo "#### ERROR Test failed. Transfer failed ####"
				cleanup_queue $pf_bdf $qid st c2h
				continue
			fi
	
			cmp $out_st $infile -n $tsize
			if [ $? -ne 0 ]; then
				echo "#### Test ERROR. Queue $2 data did not match ####" 
				dma-ctl qdma$pf_bdf q dump idx $qid dir c2h >> ./run_pf.log 2>&1
				dma-ctl qdma$pf_bdf reg dump >> ./run_pf.log 2>&1
			else
				echo "**** Test pass. Queue $qid"
			fi
			# Close the Queues
			dma-ctl qdma$pf_bdf reg write bar $usr_bar 0x08 0x22 >> ./run_pf.log 2>&1
			var=`dma-ctl qdma$pf_bdf reg read bar $usr_bar 0x18 | sed 's/.*= //' | sed 's/.*x//' | cut -d. -f1`
			j=0
			while [ "$j" -lt "2" ]
			do
				j=$[$j+1]
				if [ $var -eq "1" ]
				then 
					break
				else
					sleep 1
				fi
			done
			cleanup_queue $pf_bdf $qid st c2h
			echo "-----------------------------------------------"
		done
		pf=$((pf+1));
		echo "AXI-ST C2H for Func $pf_bdf End" 2>&1 | tee -a ./run_pf.log
		echo "***********************************************" 2>&1 | tee -a ./run_pf.log
	done
}


echo "###############################################################" > "run_pf.log"
echo "QDMA Test on All PFs Starts" >> "run_pf.log"
echo "###############################################################" >> "run_pf.log"

get_dev
if [ $flr_on -ne 0 ]; then
	set_flr
fi
run_mm_h2c_c2h 
run_st_h2c
run_st_c2h

exit 0


