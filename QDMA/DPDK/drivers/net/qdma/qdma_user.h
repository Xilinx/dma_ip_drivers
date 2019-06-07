/*-
 * BSD LICENSE
 *
 * Copyright(c) 2018 Xilinx, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QDMA_USER_H__
#define __QDMA_USER_H__
/**
 * @file
 * @brief This file contains example design/user logic controlled
 * data structures
 * The driver is specific to an example design, if the example design
 * changes user controlled parameters, this file needs to be modified
 * appropriately.
 * Structures for Completion entry, Descriptor bypass can be added here.
 */

 /**
  * C2H Completion entry structure
  * This structure is specific for the example design.
  * Processing of this ring happens in qdma_rxtx.c.
  */
struct __attribute__ ((packed)) c2h_cmpt_ring
{
	volatile uint32_t	data_frmt:1; /* For 2018.2 IP, this field
					      * determines the Standard or User
					      * format of completion entry
					      */
	volatile uint32_t	color:1;     /* This field inverts every time
					      * PIDX wraps the completion ring
					      */
	volatile uint32_t	err:1;       /* Indicates that C2H engine
					      * encountered a descriptor
					      * error
					      */
	volatile uint32_t	desc_used:1;   /* Indicates that the completion
						* packet consumes descriptor in
						* C2H ring
						*/
	volatile uint32_t	length:16;     /* Indicates length of the data
						* packet
						*/
	volatile uint32_t	user_rsv:4;    /* Reserved field */
	volatile uint8_t	user_def[];    /* User logic defined data of
						* length based on CMPT entry
						* length
						*/
};


 /**
  * MM Completion entry structure
  * This structure is specific for the example design.
  * Processing of this ring happens in rte_pmd_qdma.c.
  */
struct __attribute__ ((packed)) mm_cmpt_ring
{
	volatile uint32_t	data_frmt:1; /* For 2018.2 IP, this field
					      * determines the Standard or User
					      * format of completion entry
					      */
	volatile uint32_t	color:1;     /* This field inverts every time
					      * PIDX wraps the completion ring
					      */
	volatile uint32_t	err:1;       /* Indicates that C2H engine
					      * encountered a descriptor
					      * error
					      */
	volatile uint32_t	rsv:1;   /* Reserved */
	volatile uint8_t	user_def[];    /* User logic defined data of
						* length based on CMPT entry
						* length
						*/
};
#endif /* ifndef __QDMA_USER_H__ */
