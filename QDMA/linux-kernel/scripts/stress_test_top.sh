#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Invalid arguements."
	echo "$0 <bbddf> [<test mask>] [<total execution time>]"
	exit;
fi;

bdf=01000
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

cd ..
make clean
make
make install-user

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
		drv_mode=1
	elif [ $1 == intr ]; then
		drv_mode=2
	elif [ $1 == aggr ]; then
		drv_mode=3
	else
		drv_mode=0
	fi
	cd ..
	insmod build/qdma.ko mode=${drv_mode}
	insmod build/qdma_vf.ko mode=${drv_mode}
	cd -
	chmod +x qdma_run_stress_test.sh
	./qdma_run_stress_test.sh $bdf 2048 $run_time 1 $1
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
