#!/bin/bash
#
# Simple script to automate the VF regression testing using gtest framework
# 
# This script will insert the module, configure the qmax_vfs, and instantiate
# vfs on each PF one at a time. After instantiating the VFs, it will trigger
# qdma_test regression test suite for VF only.
# 
# Inputs: 
#	MODULE_TOP_DIR= Top directory for qdma linux module
#	GTEST_TOP_DIR= Top directory for gtest regression test suite
#	MASTER_PF_PCI_BDF= BDF For master PF.

MODULE_DIR=$1
GTEST_DIR=$2
qdma_mod=$MODULE_DIR/build/qdma.ko
qdma_mod_vf=$MODULE_DIR/build/qdma_vf.ko
gtest_bin=$GTEST_DIR/build/src/qdma_test
#busdev="0000:05:00"
master_pf=0000:$3
busdev=${master_pf%.*}
mod_param_arr=("mode=1" "mode=2" "mode=3")
vfs_cnt=(64 60 60 68)
qmax_vfs=1024

RED='\033[0;31m'
NC='\033[0m'

prep_test_env () {
	rmmod qdma
	rmmod qdma_vf
	sleep 2
	insmod $qdma_mod $1
	insmod $qdma_mod_vf
	sleep 2
	if [ ! -f /sys/bus/pci/devices/$master_pf/qdma/qmax_vfs ];then
		echo -e "${RED} Aborting, no /sys/bus/pci/devices/$master_pf/qdma/qmax_vfs found.${NC}"
		exit -4
	fi
	echo $qmax_vfs > /sys/bus/pci/devices/$master_pf/qdma/qmax_vfs
	echo $3 > /sys/bus/pci/devices/$busdev.$2/sriov_numvfs
	ret=$?
	echo 
	echo "**********************************"
	echo "******Doing QDMA VF Tests *******"
	echo "**********************************"
	echo
	echo PF=$2, VF=$3, qmax_vfs=$qmax_vfs
	echo module_param=$1
	echo 
	dma-ctl dev list

	return $ret
}

if [ $# -lt 3 ];then
	echo  -e "${RED}Aborting. Invalid usgae. Try $0 <MODULE_TOP_DIR> <GTEST_TOP_DIR> <MASTER_PF_PCI_BDF>. ${NC}"
	exit -1
fi

if [ ! -f $qdma_mod ] || [ ! -f $qdma_mod_vf ];then 
	echo -e "${RED}Aborting. Missing qdma drivers at $MODULE_DIR${NC}"
	exit -2
fi

if [ ! -f $gtest_bin ];then 
	echo -e "${RED}Aborting. Missing qdma_test at $GTEST_DIR${NC}"
	exit -3
fi

for fn in `seq 0 3`;do
	for params in "${mod_param_arr[@]}";do
		prep_test_env "$params" $fn ${vfs_cnt[$fn]}
		ret=$?
		if [ $ret -ne 0 ];then
			echo -e "${RED}FAILED VF tests, Aborting${NC}"
			exit -1
		fi
		sleep 3
		$gtest_bin --gtest_filter="*VF*"
	done
done
