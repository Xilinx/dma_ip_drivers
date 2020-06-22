#!/bin/bash
#
# Simple script to generate conf file for module params.
# 


function print_help() {
	echo ""
	echo "Usage : $0 <bus_num> <num_pfs> <mode> <config_bar> <master_pf>"
	echo "Ex : $0 0x06 4 0 0 0"
	echo "<bus_num> : PF Bus Number"
	echo ""
	echo "<num_pfs> : Number of PFs supported, Default - 4"
	echo ""
	echo "<mode> : Mode, Default - 0 (Auto)"	
	echo ""
	echo "<config_bar> : Config Bar number, Default - 0"	
	echo ""
	echo "<master_pf> : Master PF, Default - 0"
	echo ""
	echo ""
    echo ""
    exit 1
}

if [ $# -lt 1 ]; then
	echo "Invalid arguements."
	print_help
	exit;
fi;

bus_num=$1
num_pfs=4
mode=0
config_bar=0
master_pf=0

if [ ! -z $2 ]; then
	num_pfs=$2
fi

if [ ! -z $3 ]; then 
	mode=$3
fi

if [ ! -z $4 ]; then 
	config_bar=$4
fi

if [ ! -z $5 ]; then 
	master_pf=$5
fi

generate_conf()
{
	conf_file="qdma.conf"
	echo -n "options qdma-pf mode=" > conf_file
	for ((j = 0; j < ${num_pfs}; j++))
	do
		echo -n "${bus_num}:${j}:${mode}" >> conf_file
		if [ $j != $((${num_pfs} - 1)) ]; then
			echo -n "," >> conf_file
		fi	
	done
	echo -e "" >> conf_file
	echo -n "options qdma-pf config_bar=" >> conf_file
	for ((j = 0; j < ${num_pfs}; j++))
	do
		echo -n "${bus_num}:${j}:${config_bar}" >> conf_file
		if [ $j != $((${num_pfs} - 1)) ]; then
			echo -n "," >> conf_file
		fi	 
	done
	echo -e "" >> conf_file
	echo -n "options qdma-pf master_pf=${bus_num}:${master_pf}" >> conf_file
	
	echo -e "" >> conf_file
	echo -n "options qdma-vf mode=" >> conf_file
	for ((j = 0; j < ${num_pfs}; j++))
	do
		echo -n "${bus_num}:${j}:${mode}" >> conf_file
		if [ $j != $((${num_pfs} - 1)) ]; then
			echo -n "," >> conf_file
		fi	
	done
	echo -e "" >> conf_file
	echo -n "options qdma-vf config_bar=" >> conf_file
	for ((j = 0; j < ${num_pfs}; j++))
	do
		echo -n "${bus_num}:${j}:${config_bar}" >> conf_file
		if [ $j != $((${num_pfs} - 1)) ]; then
			echo -n "," >> conf_file
		fi	 
	done
	
	rm -rf /etc/modprobe.d/qdma.conf
	cp conf_file /etc/modprobe.d/qdma.conf
	rm -rf conf_file
}

generate_conf