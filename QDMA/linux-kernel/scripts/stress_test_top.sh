#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Invalid arguements."
	echo "$0 <bbddf> [<test mask>] [<total execution time>]"
	exit;
fi;

bdf=01000
num_qs=2048
override_pf_nvf=0
if [ ! -z $1 ]; then
	bdf=$1
fi
test_mask=0x0f
if [ ! -z $2 ]; then
	test_mask=$2
fi
run_time="04:00:00"
if [ ! -z $3 ]; then
	run_time=$3
fi
if [ ! -z $4 ]; then
	num_qs=$4
fi
if [ ! -z $5 ]; then
	override_pf_nvf=$5
fi

cd ..
./make_libqdma.sh clean
./make_libqdma.sh
make install-user
make install-dev

insmod build/qdma.ko
echo ${bdf}
dmactl qdma${bdf} reg write bar 2 0xA0 0x01
rmmod qdma
cd -


function cleanup_env()
{
	rmmod qdma_vf
	rmmod qdma
}

function run_stress_test()
{
	drv_mode=0
	if [ $1 == poll ]; then
		drv_mode=1111
	elif [ $1 == intr ]; then
		drv_mode=2222
	elif [ $1 == aggr ]; then
		drv_mode=3333
	else
		drv_mode=0000
	fi
	pci_bus=${bdf:0:2}
	cd ..
	insmod build/qdma.ko mode=0x${pci_bus}${drv_mode}
	insmod build/qdma_vf.ko mode=0x${pci_bus}${drv_mode}
	cd -
	chmod +x qdma_run_stress_test.sh
	./qdma_run_stress_test.sh $bdf $num_qs $run_time $override_pf_nvf $1
	dmactl qdma$bdf reg write bar 2 0xA0 0x01
	cleanup_env
}

trap cleanup_env EXIT

if (( test_mask & 0x02 )); then
	run_stress_test poll
fi
if (( test_mask & 0x04 )); then
	run_stress_test intr
fi
if (( test_mask & 0x08 )); then
	run_stress_test aggr
fi
if (( test_mask & 0x01 )); then
	run_stress_test auto
fi

