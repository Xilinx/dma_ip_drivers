#!/bin/bash
#./estherdaq  -a 0xfa0dcd8 -b 0x2328fc18 -c 0xfa0dcd8 -s 0x400000 -m 0x31999 &
./estherdaq &
sleep 1
echo "Gen trig ..."
ssh red-pitaya "LD_LIBRARY_PATH=/opt/redpitaya/lib /root/projects/esther-pulse-gen/src/gen_esther_pulse2"
echo "Completed"

