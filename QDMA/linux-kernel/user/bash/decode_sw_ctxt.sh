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

if [ $# -lt 5 ]; then
	echo "$0: <w4> <w3> <w2> <w1> <w0>"
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

printf '\nW4 0x%08x:\n' $w4

let "v = ($w4 >> 11) & 0x1"
printf '[139]     (W4[11])    int_aggr    0x%x\n' $v

let "v = ($w4 >> 0) & 0x7FF"
printf '[138:128] (W3[10:0]   dsc_base_h  0x%x\n' $v

printf '\n[127:64] (W3,W2)      dsc_base    0x%08x%08x\n' $w3 $w2

printf '\nW1 0x%08x:\n' $w1

let "v = ($w1 >> 31) & 0x1"
printf '[63]     (W1[31])     is_mm       0x%x\n' $v

let "v = ($w1 >> 30) & 0x1"
printf '[62]     (W1[30])     mrkr_dis    0x%x\n' $v

let "v = ($w1 >> 29) & 0x1"
printf '[61]     (W1[29])     irq_req     0x%x\n' $v

let "v = ($w1 >> 28) & 0x1"
printf '[60]     (W1[28])     err_cmpl_status_sent 0x%x\n' $v

let "v = ($w1 >> 26) & 0x3"
printf '[59:58]  (W1[27:26]   err         0x%x\n' $v

let "v = ($w1 >> 25) & 0x1"
printf '[57]     (W1[25])     irq_no_last 0x%x\n' $v

let "v = ($w1 >> 22) & 0x7"
printf '[56:54]  (W1[23:22]   port_id     0x%x\n' $v

let "v = ($w1 >> 21) & 0x1"
printf '[53]     (W1[21])     irq_en      0x%x\n' $v

let "v = ($w1 >> 20) & 0x1"
printf '[52]     (W1[20])     cmpl_status_en      0x%x\n' $v

let "v = ($w1 >> 19) & 0x1"
printf '[51]     (W1[19])     mm_chn      0x%x\n' $v

let "v = ($w1 >> 18) & 0x1"
printf '[50]     (W1[18])     byp         0x%x\n' $v

let "v = ($w1 >> 16) & 0x3"
printf '[49:48]  (W1[17:16]   dsc_sz      0x%x\n' $v

let "v = ($w1 >> 12) & 0xF"
printf '[47:44]  (W1[15:12]   rng_sz      0x%x\n' $v

let "v = ($w1 >> 8) & 0xF"
printf '[43:40]  (W1[11:8]    rsvd        0x%x\n' $v

let "v = ($w1 >> 5) & 0x7"
printf '[39:37]  (W1[7:5]     fetch_max   0x%x\n' $v

let "v = ($w1 >> 4) & 0x1"
printf '[36]  (W1[4]          at          0x%x\n' $v

let "v = ($w1 >> 3) & 0x1"
printf '[35]     (W1[3])      cmpl_status_acc_en       0x%x\n' $v

let "v = ($w1 >> 2) & 0x1"
printf '[34]     (W1[2])      cmpl_status_pend_chk     0x%x\n' $v

let "v = ($w1 >> 1) & 0x1"
printf '[33]     (W1[1])      fcrd_en     0x%x\n' $v

let "v = $w1 & 0x1"
printf '[32]     (W1[0])      qen         0x%x\n' $v

printf '\nW0 0x%08x:\n' $w0

let "v = ($w0 >> 25) & 0x7F"
printf '[31:25]  (W0[31:25]   reserved    0x%x\n' $v

let "v = ($w0 >> 17) & 0xFF"
printf '[24:17]  (W0[24:17]   fnc_id      0x%x\n' $v

let "v = ($w0 >> 16) & 0x1"
printf '[16]     (W0[16])     irq_arm     0x%x\n' $v

let "v = $w0 & 0xFF"
printf '[15:0]   (W0[15:0]    pidx        0x%x\n' $v
