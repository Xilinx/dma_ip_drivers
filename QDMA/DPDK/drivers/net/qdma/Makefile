#   BSD LICENSE
#
#   Copyright(c) 2017-2019 Xilinx, Inc. All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


include $(RTE_SDK)/mk/rte.vars.mk

## if modified this BRAM_SIZE, then the same should be modified in the testapp Makefile also
## default set to 512KB
BRAM_SIZE ?= 524288

#
# library name
#
LIB = librte_pmd_qdma.a

# library version
 LIBABIVER := 1

# versioning export map
EXPORT_MAP := rte_pmd_qdma_version.map

CFLAGS += -O3 -DDMA_BRAM_SIZE=$(BRAM_SIZE)
#CFLAGS += -g
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -I$(RTE_SDK)/drivers/net/qdma/qdma_access/
CFLAGS += -I$(RTE_SDK)/drivers/net/qdma/
ifeq ($(TEST_64B_DESC_BYPASS),1)
  CFLAGS += -DTEST_64B_DESC_BYPASS
endif

# this lib depends upon:
LDLIBS += -lpthread
LDLIBS += -lrte_eal -lrte_mbuf -lrte_mempool -lrte_ring
LDLIBS += -lrte_ethdev -lrte_net -lrte_kvargs
LDLIBS += -lrte_bus_pci

#
# all source are stored in SRCS-y
#
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_ethdev.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_vf_ethdev.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_devops.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_common.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_rxtx.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_xdebug.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_user.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_access.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_list.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_resource_mgmt.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_mbox_protocol.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_access_common.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_cpm_access.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_access/qdma_reg_dump.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_mbox.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma_platform.c
SRCS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += rte_pmd_qdma.c


#
## Export include files
#
SYMLINK-y-include += rte_pmd_qdma.h


include $(RTE_SDK)/mk/rte.lib.mk
