#/*
# * This file is part of the Xilinx DMA IP Core driver for Linux
# *
# * Copyright (c) 2017-2022, Xilinx, Inc. All rights reserved.
# * Copyright (c) 2022-2024, Advanced Micro Devices, Inc. All rights reserved.
# *
# * This source code is free software; you can redistribute it and/or modify it
# * under the terms and conditions of the GNU General Public License,
# * version 2, as published by the Free Software Foundation.
# *
# * This program is distributed in the hope that it will be useful, but WITHOUT
# * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# * more details.
# *
# * The full GNU General Public License is included in this distribution in
# * the file called "COPYING".
# */

#Build flags common to xdma/qdma

ifeq ($(VF),1)
   EXTRA_FLAGS += -D__QDMA_VF__
   PFVF_TYPE = -vf
else
   PFVF_TYPE = -pf
endif

