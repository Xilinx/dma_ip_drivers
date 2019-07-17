#!/bin/bash
#
# Copyright (c) 2017-present,  Xilinx, Inc.
# All rights reserved.
#

if [ $# -lt 2 ]; then
	echo "$0: <w1> <w0>"
	exit
fi

w1=$1
w0=$2

let "w0 = $w0 + 0"
let "w1 = $w1 + 0"

printf '\n0x%08x 0x%08x\n\n' $w1 $w0

let "v = ($w1 >> 13) & 0x1"
printf '[45]     (W1[13])                valid              0x%x\n' $v

let "v = $w1 & 0x1FFF"
let "v1 = ($w0 >> 29) & 0x7"
let "v2 = ($v << 3) | $v1"
printf '[44:29]  (W1[12:0] W0[31:29])    sw_crdt            0x%x\n' $v2

let "v = ($w0 >> 28) & 0x1"
printf '[28]   (W0[28]                   q_is_in_pftch      0x%x\n' $v

let "v = ($w0 >> 27) & 0x1"
printf '[27]   (W0[27]                   en_pftch           0x%x\n' $v

let "v = ($w0 >> 8) & 0x3FFFF"
printf '[25:8]   (W0[25:8]               rsvd               0x%x\n' $v

let "v = ($w0 >> 5) & 0x7"
printf '[7:5]    (W0[7:5]                port_id            0x%x\n' $v

let "v = ($w0 >> 1) & 0xF"
printf '[4:1]    (W0[4:1]                buf_size_idx       0x%x\n' $v

let "v = $w0 & 0x1"
printf '[0]      (W0[0]                  bypass             0x%x\n' $v
