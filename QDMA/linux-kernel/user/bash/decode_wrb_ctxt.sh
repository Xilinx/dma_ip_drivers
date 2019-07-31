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

if [ $# -lt 4 ]; then
	echo "$0: <w3> <w2> <w1> <w0>"
	exit
fi

w4=$1
w3=$2
w2=$3
w1=$4
w0=$5

let "w0 = $w0 + 0"
let "w1 = $w1 + 0"
let "w2 = $w2 + 0"
let "w3 = $w3 + 0"
let "w4 = $w4 + 0"

printf '\n0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n\n' $w4 $w3 $w2 $w1 $w0

let "v = ($w4 >> 3) & 0x1"
printf '[131]     (W4[3])                       at           0x%x\n' $v

let "v = ($w4 >> 2) & 0x1"
printf '[130]     (W4[2])                       ovf_chk_dis  0x%x\n' $v

let "v = ($w4 >> 1) & 0x1"
printf '[129]     (W4[1])                       full_upd     0x%x\n' $v

let "v = ($w4 >> 0) & 0x1"
printf '[128]     (W4[0])                       tmr_runnig   0x%x\n' $v

let "v = ($w3 >> 31) & 0x1"
printf '[127]     (W3[31])                      user_trig_pend        0x%x\n' $v

let "v = ($w3 >> 29) & 0xFFFF"
printf '[126:125] (W3[30:29])                   err          0x%x\n' $v

let "v = ($w3 >> 28) & 0x1"
printf '[124]     (W3[28])                      valid        0x%x\n' $v

let "v = ($w3 >> 12) & 0xFFFF"
printf '[123:108] (W3[27:12])                   cidx         0x%x\n' $v

let "v = $w3 & 0xFFF"
let "v1 = ($w2 >> 28) & 0xF"
let "v2 = ($v << 4) | $v1"
printf '[107:92]  (W3[11:0] W2[31:28])          pidx         0x%x\n' $v2

let "v = ($w2 >> 26) & 0x3"
printf '[91:90]   (W2[27:26])                   desc_size    0x%x\n' $v

let "v2 = $w2 & 0x3FFFFFF"
let "v1 = ($w1 >> 6) & 0x3FFFFFF"
printf '[89:38]   (W2[25:0] W1[31:6])           baddr_64     '
printf '0x%08x ' $v2
printf '0x%08x \n' $v1

let "v = ($w1 >> 0) & 0x3F"
printf '[37:32]   (W0[5:0])                     rsvd         0x%x\n' $v

let "v = ($w0 >> 28) & 0xF"
printf '[31:28]   (W0[31:28])                   rng_sz       0x%x\n' $v

let "v = ($w0 >> 27) & 0x1"
printf '[27]      (W0[27])                      color        0x%x\n' $v

let "v = ($w0 >> 25) & 0x3"
printf '[26:25]   (W0[26:25])                   int_st       0x%x\n' $v

let "v = ($w0 >> 21) & 0xF"
printf '[24:21]   (W0[24:21])                   timer_idx    0x%x\n' $v

let "v = ($w0 >> 17) & 0xF"
printf '[20:17]   (W0[20:17])                   counter_idx  0x%x\n' $v

let "v = ($w0 >> 13) & 0xF"
printf '[16:13]    (W0[16:`3])                  rsvd         0x%x\n' $v

let "v = ($w0 >> 5) & 0xF"
printf '[12:5]    (W0[12:5])                    fnc_id       0x%x\n' $v

let "v = ($w0 >> 2) & 0x7"
printf '[4:2]     (W0[4:2])                     trig_mode    0x%x\n' $v

let "v = ($w0 >> 1) & 0x1"
printf '[1]       (W0[1])                       en_int       0x%x\n' $v

let "v = $w0 & 0x1"
printf '[0]       (W0[0])                       en_stat_desc 0x%x\n' $v
