/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2019 Xilinx, Inc. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h> /**> memset */
#include <signal.h>
#include <termios.h>
#include <rte_eal.h> /**> rte_eal_init */
#include <rte_debug.h> /**> for rte_panic */
#include <rte_ethdev.h> /**> rte_eth_rx_burst */
#include <rte_errno.h> /**> rte_errno global var */
#include <rte_memzone.h> /**> rte_memzone_dump */
#include <rte_memcpy.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_log.h>
#include <rte_string_fns.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <time.h>  /** For SLEEP **/
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <rte_mbuf.h>

#include "pcierw.h"
#include "commands.h"
#include "qdma_regs.h"
#include "testapp.h"
#include "../../drivers/net/qdma/rte_pmd_qdma.h"

int num_ports;
char *filename;

struct port_info pinfo[QDMA_MAX_PORTS];

int do_recv_mm(int portid, int fd, int queueid, int ld_size, int tot_num_desc)
{
	struct rte_mbuf *pkts[NUM_RX_PKTS] = { NULL };
	int nb_rx = 0, i = 0, ret = 0, num_pkts;
	int tdesc;
#ifdef PERF_BENCHMARK
	uint64_t prev_tsc, cur_tsc, diff_tsc;
#endif

	if (tot_num_desc == 0) {
		printf("Error: tot_num_desc : invalid value\n");
		return -1;
	}

	printf("recv start num-desc: %d, with data-len: %d, "
			"last-desc-size:%d\n",
			tot_num_desc, pinfo[portid].buff_size, ld_size);
	tdesc = tot_num_desc;
	if (ld_size)
		tdesc--;

	while (tdesc) {
		if (tdesc > NUM_RX_PKTS)
			num_pkts = NUM_RX_PKTS;
		else
			num_pkts = tdesc;
#ifdef PERF_BENCHMARK
		prev_tsc = rte_rdtsc_precise();
#endif
		/* try to receive RX_BURST_SZ packets */
		nb_rx = rte_eth_rx_burst(portid, queueid, pkts, num_pkts);

#ifdef PERF_BENCHMARK
		cur_tsc = rte_rdtsc_precise();
		diff_tsc = cur_tsc - prev_tsc;
#endif

		if (nb_rx == 0) {
			printf("Error: dma_from_device failed to "
					"receive packets\n");
			return -1;
		}
#ifdef PERF_BENCHMARK
	   /* Calculate average operations processed per second */
		double pkts_per_second = ((double)nb_rx * rte_get_tsc_hz() /
							diff_tsc);

		/* Calculate average throughput (Gbps) in bits per second */
		double throughput_gbps = ((pkts_per_second *
				pinfo[portid].buff_size) / 1000000000);
		printf("Throughput GBps %lf\n", throughput_gbps);
				printf("%16s%16s%16s%16s%16s%16s%16s\n\n",
					"Buf Size", "Burst Size",
					"pps", "Gbps", "freq", "Cycles",
					"Cycles/Buf");

			printf("%16u%16u%16.4lf%16.4lf%16 "
					""PRIu64"%16"PRIu64"%16"PRIu64"\n",
					pinfo[portid].buff_size,
					nb_rx,
					pkts_per_second,
					throughput_gbps,
					rte_get_tsc_hz(),
					diff_tsc,
					diff_tsc/nb_rx);
#endif

		for (i = 0; i < nb_rx; i++) {
			struct rte_mbuf *mb = pkts[i];
			ret = write(fd, rte_pktmbuf_mtod(mb, void*),
						pinfo[portid].buff_size);
			rte_pktmbuf_free(mb);
#ifndef PERF_BENCHMARK
			printf("recv count: %d, with data-len: %d\n", i, ret);
#endif
		}
		tdesc = tdesc - nb_rx;
	}
	if (ld_size) {
		struct rte_mbuf *mb;
		nb_rx = rte_eth_rx_burst(portid, queueid, pkts, 1);
		if (nb_rx != 0) {
			mb = pkts[0];
			ret = write(fd, rte_pktmbuf_mtod(mb, void*), ld_size);
			rte_pktmbuf_free(mb);
		}
	}
	fsync(fd);
	printf("DMA received number of packets: %d, on queue-id:%d\n",
							nb_rx, queueid);
	return 0;
}

int do_recv_st(int portid, int fd, int queueid, int input_size)
{
	struct rte_mbuf *pkts[NUM_RX_PKTS] = { NULL };
	int nb_rx = 0, i, ret = 0, tmp = 0, num_pkts, nb_pkts;
	int reg_val, num_pkts_recv = 0;
	int regval;
	int user_bar_idx;
	struct rte_mbuf *nxtmb;
	int qbase = pinfo[portid].queue_base, diag;
	unsigned int max_completion_size;
	unsigned int max_rx_retry;

#ifdef DUMP_MEMPOOL_USAGE_STATS
	struct rte_mempool *mp;
	mp = rte_mempool_lookup(pinfo[portid].mem_pool);

	/* get the mempool from which to acquire buffers */
	if (mp == NULL)
		rte_exit(EXIT_FAILURE, "Could not find mempool with name %s\n",
				pinfo[portid].mem_pool);
#endif //DUMP_MEMPOOL_USAGE_STATS

	user_bar_idx = pinfo[portid].user_bar_idx;
	PciWrite(user_bar_idx, C2H_ST_QID_REG, (queueid + qbase), portid);

	/* As per  hardware design a single completion will point to atmost
	 * 7 descriptors. So If the size of the buffer in descriptor is 4KB ,
	 * then a single completion which corresponds a packet can  give you
	 * atmost 28KB data.
	 *
	 * As per this when testing sizes beyond 28KB, one needs to split it
	 * up in chunks of 28KB, example : to test 56KB data size, set 28KB
	 * as packet length in USER BAR 0x04 register and no of packets as 2
	 * in user BAR 0x20 register this would give you completions or
	 * packets, which needs to be combined as one in application.
	 */

	max_completion_size = pinfo[portid].buff_size * 7;

	/* Calculate number of packets to receive and programming user bar */
	if (input_size == 0) /* zerobyte support uses one descriptor */
		num_pkts = 1;
	else if (input_size % max_completion_size != 0)
		num_pkts = input_size / max_completion_size + 1;
	else
		num_pkts = input_size / max_completion_size;

	reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, portid);
	reg_val &= C2H_CONTROL_REG_MASK;
	if (!(reg_val & ST_LOOPBACK_EN)) {
		PciWrite(user_bar_idx, C2H_PACKET_COUNT_REG, num_pkts, portid);

		if (num_pkts > 1)
			PciWrite(user_bar_idx, C2H_ST_LEN_REG,
					max_completion_size, portid);
		else
			PciWrite(user_bar_idx, C2H_ST_LEN_REG, input_size,
								portid);

		regval = PciRead(user_bar_idx, C2H_PACKET_COUNT_REG, portid);
		printf("BAR-%d is the QDMA C2H number of packets:0x%x,\n",
				user_bar_idx, regval);
	}

	while (num_pkts) {
		if (num_pkts > NUM_RX_PKTS)
			nb_pkts = NUM_RX_PKTS;
		else
			nb_pkts = num_pkts;

		max_rx_retry = RX_TX_MAX_RETRY;
		/* Start the C2H Engine */
		reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, portid);
		reg_val &= C2H_CONTROL_REG_MASK;
		if (!(reg_val & ST_LOOPBACK_EN)) {
			reg_val |= ST_C2H_START_VAL;
			PciWrite(user_bar_idx, C2H_CONTROL_REG, reg_val,
								portid);
		}
		 /* Immediate data Enabled*/
		if ((reg_val & ST_C2H_IMMEDIATE_DATA_EN)) {
			/* payload received is zero for the immediate data case.
			 * Therefore, no need to call the rx_burst function
			 * again in this case and set the num_pkts to nb_rx
			 * which is always Zero.
			 */
			diag = rte_pmd_qdma_set_immediate_data_state(portid,
							queueid, 1);
			if (diag < 0)
				rte_exit(EXIT_FAILURE, "rte_pmd_qdma_set_immediate_data_state : "
						"failed\n");
			nb_rx = rte_eth_rx_burst(portid, queueid, pkts,
								nb_pkts);
			num_pkts = num_pkts_recv = nb_rx;

			/* Reset the queue's DUMP_IMMEDIATE_DATA mode */
			diag = rte_pmd_qdma_set_immediate_data_state(portid,
							queueid, 0);
			if (diag < 0)
				rte_exit(EXIT_FAILURE, "rte_pmd_qdma_set_immediate_data_state : "
					  "failed\n");
		} else {
			/* try to receive RX_BURST_SZ packets */

			nb_rx = rte_eth_rx_burst(portid, queueid, pkts,
							nb_pkts);

			/* For zero byte packets, do not continue looping */
			if (input_size == 0)
				break;

			tmp = nb_rx;
			while ((nb_rx < nb_pkts) && max_rx_retry) {
				rte_delay_us(1);
				nb_pkts -= nb_rx;
				nb_rx = rte_eth_rx_burst(portid, queueid,
								&pkts[tmp],
								nb_pkts);
				tmp += nb_rx;
				max_rx_retry--;
			}
			num_pkts_recv = tmp;
			if ((max_rx_retry == 0) && (num_pkts_recv == 0)) {
				printf("ERROR: rte_eth_rx_burst failed for "
						"port %d queue id %d, Expected pkts = %d "
						"Received pkts = %d\n",
						portid, queueid, nb_pkts, num_pkts_recv);
				break;
			}
		}

		 /* Stop the C2H Engine */
		reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, portid);
		reg_val &= C2H_CONTROL_REG_MASK;
		if (!(reg_val & ST_LOOPBACK_EN)) {
			reg_val &= ~(ST_C2H_START_VAL);
			PciWrite(user_bar_idx, C2H_CONTROL_REG, reg_val,
					portid);
		}

#ifdef DUMP_MEMPOOL_USAGE_STATS
		printf("%s(): %d: queue id = %d, mbuf_avail_count = %d, "
				"mbuf_in_use_count = %d\n",
				__func__, __LINE__, queueid,
				rte_mempool_avail_count(mp),
				rte_mempool_in_use_count(mp));
#endif //DUMP_MEMPOOL_USAGE_STATS
		for (i = 0; i < num_pkts_recv; i++) {
			struct rte_mbuf *mb = pkts[i];
			while (mb != NULL) {
				ret += write(fd, rte_pktmbuf_mtod(mb, void*),
						rte_pktmbuf_data_len(mb));
				nxtmb = mb->next;
				mb = nxtmb;
			}

			mb = pkts[i];
			rte_pktmbuf_free(mb);
#ifndef PERF_BENCHMARK
			printf("recv count: %d, with data-len: %d\n", i, ret);
#endif
			ret = 0;
		}
#ifdef DUMP_MEMPOOL_USAGE_STATS
		printf("%s(): %d: queue id = %d, mbuf_avail_count = %d, "
				"mbuf_in_use_count = %d, num_pkts_recv = %d\n",
				__func__, __LINE__, queueid,
				rte_mempool_avail_count(mp),
				rte_mempool_in_use_count(mp), num_pkts_recv);
#endif //DUMP_MEMPOOL_USAGE_STATS
		num_pkts = num_pkts - num_pkts_recv;
		printf("DMA received number of packets: %d, on queue-id:%d\n",
				num_pkts_recv, queueid);
	}

	fsync(fd);
	return 0;
}

int do_xmit(int portid, int fd, int queueid, int ld_size, int tot_num_desc,
				int zbyte)
{
	struct rte_mempool *mp;
	struct rte_mbuf *mb[NUM_RX_PKTS] = { NULL };
	int ret = 0, nb_tx, i = 0, tdesc, num_pkts = 0, total_tx = 0, reg_val;
	int tmp = 0, user_bar_idx;
	int qbase = pinfo[portid].queue_base;
	uint32_t max_tx_retry;

#ifdef PERF_BENCHMARK
	uint64_t prev_tsc, cur_tsc, diff_tsc;
#endif
	mp = rte_mempool_lookup(pinfo[portid].mem_pool);
	/* get the mempool from which to acquire buffers */
	if (mp == NULL)
		rte_exit(EXIT_FAILURE, "Could not find mempool with name %s\n",
				pinfo[portid].mem_pool);
	tdesc = tot_num_desc;
	user_bar_idx = pinfo[portid].user_bar_idx;

	if (ld_size)
		tdesc--;

	while (tdesc) {
		if (tdesc > NUM_RX_PKTS)
			num_pkts = NUM_RX_PKTS;
		else
			num_pkts = tdesc;

		max_tx_retry = RX_TX_MAX_RETRY;
#ifdef DUMP_MEMPOOL_USAGE_STATS
		printf("%s(): %d: queue id %d, mbuf_avail_count = %d, "
				"mbuf_in_use_count = %d",
				__func__, __LINE__, queueid,
				rte_mempool_avail_count(mp),
				rte_mempool_in_use_count(mp));
#endif //DUMP_MEMPOOL_USAGE_STATS
		for (i = 0; i < num_pkts; i++) {
			mb[i] = rte_pktmbuf_alloc(mp);
			if (mb[i] == NULL)
				rte_exit(EXIT_FAILURE, " #####Cannot "
						"allocate mbuf packet\n");

			if (!zbyte)
				ret = read(fd, rte_pktmbuf_mtod(mb[i], void *),
						pinfo[portid].buff_size);
			if (ret < 0) {
				printf("Error: Could not the read "
						"input-file\n");
				return -1;
			}
			mb[i]->nb_segs = 1;
			mb[i]->next = NULL;
			rte_pktmbuf_data_len(mb[i]) = (uint16_t)ret;
			rte_pktmbuf_pkt_len(mb[i])  = (uint16_t)ret;
		}

#ifdef DUMP_MEMPOOL_USAGE_STATS
		printf("%s(): %d: queue id %d, mbuf_avail_count = %d, "
				"mbuf_in_use_count = %d, num_pkts_tx = %d",
				__func__, __LINE__, queueid,
				rte_mempool_avail_count(mp),
				rte_mempool_in_use_count(mp), num_pkts);
#endif //DUMP_MEMPOOL_USAGE_STATS

		total_tx = num_pkts;
		PciWrite(user_bar_idx, C2H_ST_QID_REG, (queueid + qbase),
				portid);
		/* try to transmit TX_BURST_SZ packets */

#ifdef PERF_BENCHMARK
		prev_tsc = rte_rdtsc_precise();
#endif
		nb_tx = rte_eth_tx_burst(portid, queueid, mb, num_pkts);
#ifdef PERF_BENCHMARK
		cur_tsc = rte_rdtsc_precise();
		diff_tsc = cur_tsc - prev_tsc;
	   /* Calculate average operations processed per second */
		double pkts_per_second = ((double)nb_tx * rte_get_tsc_hz() /
								diff_tsc);

		/* Calculate average throughput (Gbps) in bits per second */
		double throughput_gbps = ((pkts_per_second *
				pinfo[portid].buff_size) / 1000000000);
		printf("Throughput GBps %lf\n", throughput_gbps);
				printf("%12s%12s%12s%12s%12s%12s%12s\n\n",
					"Buf Size", "Burst Size",
					"pps", "Gbps", "freq", "Cycles",
					"Cycles/Buf");

			printf("%12u%12u%12.4lf%12.4lf%12"
					""PRIu64"%12"PRIu64"%12"PRIu64"\n",
					pinfo[portid].buff_size,
					nb_tx,
					pkts_per_second,
					throughput_gbps,
					rte_get_tsc_hz(),
					diff_tsc,
					diff_tsc/nb_tx);
#endif
		tmp = nb_tx;
		while ((nb_tx < num_pkts) && max_tx_retry) {
			printf("Couldn't transmit all the packets:  Expected = %d "
					"Transmitted = %d.\n"
					"Calling rte_eth_tx_burst again\n",
					num_pkts, nb_tx);
			rte_delay_us(1);
			num_pkts -= nb_tx;
			nb_tx = rte_eth_tx_burst(portid, queueid, &mb[tmp],
					num_pkts);
			tmp += nb_tx;
			max_tx_retry--;
		}

		if ((max_tx_retry == 0)) {
			for (i = tmp; i < total_tx; i++)
				rte_pktmbuf_free(mb[i]);
			if (tmp == 0) {
				printf("ERROR: rte_eth_tx_burst failed "
						"for port %d queue %d\n",
						portid, queueid);
				break;
			}
		}

		tdesc = tdesc - tmp;
		printf("\nDMA transmitted number of packets: %d, "
				"on Queue-id:%d\n",
				tmp, queueid);
	}

	if (ld_size) {
		mb[0] = rte_pktmbuf_alloc(mp);
		if (mb[0] == NULL) {
			rte_exit(EXIT_FAILURE, " #####Cannot allocate mbuf "
								"packet\n");
		}
		ret = read(fd, rte_pktmbuf_mtod(mb[0], void *), ld_size);
		if (ret < 0) {
			printf("Error: Could not read the input-file\n");
			return -1;
		}
		mb[0]->nb_segs = 1;
		mb[0]->next = NULL;
		rte_pktmbuf_data_len(mb[0]) = (uint16_t)ret;
		rte_pktmbuf_pkt_len(mb[0])  = (uint16_t)ret;

		nb_tx = rte_eth_tx_burst(portid, queueid, mb, 1);
		if (nb_tx == 0)
			rte_pktmbuf_free(mb[0]);
	}

	reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, portid);
	reg_val &= C2H_CONTROL_REG_MASK;
	if (!(reg_val & ST_LOOPBACK_EN)) {
		reg_val = PciRead(user_bar_idx, H2C_STATUS_REG, portid);
		printf("BAR-%d is the QDMA H2C transfer match: 0x%x,\n",
			user_bar_idx, reg_val);

		/** TO clear H2C DMA write **/
		PciWrite(user_bar_idx, H2C_CONTROL_REG, 0x1, portid);
	}

	return 0;
}

void port_close(int port_id)
{
	struct rte_mempool *mp;

	rte_eth_dev_stop(port_id);
	rte_eth_dev_close(port_id);
	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);

	if (mp != NULL)
		rte_mempool_free(mp);
}

static int mbox_event_callback(uint16_t portid,
				enum rte_eth_event_type type __rte_unused,
				void *param __rte_unused, void *ret_param)
{
	RTE_SET_USED(ret_param);
	printf("%s is received\n", __func__);
	pinfo[portid].num_queues = 0;
	port_close(portid);
	return 0;
}

static struct option const long_opts[] = {
{"filename", 1, 0, 0},
{NULL, 0, 0, 0}
};

int parse_cmdline(int argc, char **argv)
{
	int cmd_opt;
	int option_index;
	char **argvopt;

	argvopt = argv;
	while ((cmd_opt = getopt_long(argc, argvopt, "c:n:b:w", long_opts,
						&option_index)) != EOF) {
		switch (cmd_opt) {
		case 'c':
			/* eal option */
			break;
		case 'n':
			/* eal option */
			break;
		case 'b':
			/* eal option */
			break;
		case 'w':
			/* eal option */
			break;
		case 0:
			if (!strncmp(long_opts[option_index].name,
						"filename",
						sizeof("filename"))) {

				filename = optarg;
			}
			break;
		default:
			printf("please pass valid parameters as follows:\n");
			return -1;
		}
	}
	return 0;
}

int port_init(int portid, int num_queues, int st_queues,
				int nb_descs, int buff_size)
{
	struct rte_mempool *mbuf_pool;
	struct rte_eth_conf	    port_conf;
	struct rte_eth_txconf   tx_conf;
	struct rte_eth_rxconf   rx_conf;
	int                     diag, x;
	uint32_t                queue_base, nb_buff;

	printf("Setting up port :%d.\n", portid);

	snprintf(pinfo[portid].mem_pool, RTE_MEMPOOL_NAMESIZE,
			MBUF_POOL_NAME_PORT, portid);

	/* Mbuf packet pool */
	nb_buff = ((nb_descs) * num_queues * 2);

	/* NUM_RX_PKTS should be added to every queue as that many descriptors
	 * can be pending with application after Rx processing but before
	 * consumed by application or sent to Tx
	 */
	nb_buff += ((NUM_RX_PKTS) * num_queues);

	mbuf_pool = rte_pktmbuf_pool_create(pinfo[portid].mem_pool, nb_buff,
			MP_CACHE_SZ, 0, buff_size +
			RTE_PKTMBUF_HEADROOM,
			rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, " Cannot create mbuf pkt-pool\n");
#ifdef DUMP_MEMPOOL_USAGE_STATS
	printf("%s(): %d: mpool = %p, mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d,"
			"nb_buff = %d\n", __func__, __LINE__, mbuf_pool,
			rte_mempool_avail_count(mbuf_pool),
			rte_mempool_in_use_count(mbuf_pool), nb_buff);
#endif //DUMP_MEMPOOL_USAGE_STATS

	/*
	 * Make sure the port is configured.  Zero everything and
	 * hope for sane defaults
	 */
	memset(&port_conf, 0x0, sizeof(struct rte_eth_conf));
	memset(&tx_conf, 0x0, sizeof(struct rte_eth_txconf));
	memset(&rx_conf, 0x0, sizeof(struct rte_eth_rxconf));
	diag = rte_pmd_qdma_get_bar_details(portid, &(pinfo[portid].config_bar_idx),
			&(pinfo[portid].user_bar_idx), &(pinfo[portid].bypass_bar_idx));

	if (diag < 0)
		rte_exit(EXIT_FAILURE, "rte_pmd_qdma_get_bar_details failed\n");

	printf("QDMA Config bar idx: %d\n", pinfo[portid].config_bar_idx);
	printf("QDMA User bar idx: %d\n", pinfo[portid].user_bar_idx);
	printf("QDMA Bypass bar idx: %d\n", pinfo[portid].bypass_bar_idx);

	/* configure the device to use # queues */
	diag = rte_eth_dev_configure(portid, num_queues, num_queues,
			&port_conf);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "Cannot configure port %d (err=%d)\n",
				portid, diag);

	diag = rte_pmd_qdma_get_queue_base(portid, &queue_base);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "rte_pmd_qdma_get_queue_base : Querying of "
				"QUEUE_BASE failed\n");
	pinfo[portid].queue_base = queue_base;
	pinfo[portid].num_queues = num_queues;
	pinfo[portid].st_queues = st_queues;
	pinfo[portid].buff_size = buff_size;

	for (x = 0; x < num_queues; x++) {
		if (x < st_queues) {
			diag = rte_pmd_qdma_set_queue_mode(portid, x, RTE_PMD_QDMA_STREAMING_MODE);
			if (diag < 0)
				rte_exit(EXIT_FAILURE, "rte_pmd_qdma_set_queue_mode : "
						"Passing of QUEUE_MODE "
						"failed\n");
		} else {
			diag = rte_pmd_qdma_set_queue_mode(portid, x, RTE_PMD_QDMA_MEMORY_MAPPED_MODE);
			if (diag < 0)
				rte_exit(EXIT_FAILURE, "rte_pmd_qdma_set_queue_mode : "
						"Passing of QUEUE_MODE "
						"failed\n");
		}

		diag = rte_eth_tx_queue_setup(portid, x, nb_descs, 0,
				&tx_conf);
		if (diag < 0)
			rte_exit(EXIT_FAILURE, "Cannot setup port %d "
					"TX Queue id:%d "
					"(err=%d)\n", portid, x, diag);
		rx_conf.rx_thresh.wthresh = DEFAULT_RX_WRITEBACK_THRESH;
		diag = rte_eth_rx_queue_setup(portid, x, nb_descs, 0,
				&rx_conf, mbuf_pool);
		if (diag < 0)
			rte_exit(EXIT_FAILURE, "Cannot setup port %d "
					"RX Queue 0 (err=%d)\n", portid, diag);
	}

	diag = rte_eth_dev_start(portid);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "Cannot start port %d (err=%d)\n",
				portid, diag);

	rte_eth_dev_callback_register((uint16_t)portid, RTE_ETH_EVENT_VF_MBOX,
			mbox_event_callback, NULL);

	return 0;
}

static inline void do_sanity_checks(void)
{
#if (!defined(RTE_LIBRTE_QDMA_PMD))
	rte_exit(EXIT_FAILURE, "CONFIG_RTE_LIBRTE_QDMA_PMD must be set "
			"to 'Y' in the .config file\n");
#endif /* RTE_LIBRTE_XDMA_PMD */

}


void load_file_cmds(struct cmdline *cl)
{
	FILE *fp;
	char buff[256];

	cmdline_printf(cl, "load commands from file:%s\n\n", filename);
	fp = fopen((const char *)filename, "r");
	if (fp == NULL) {
		cmdline_printf(cl, "Error: Invalid filename: %s\n", filename);
		return;
	}

	rdline_reset(&cl->rdl);
	{
		cmdline_in(cl, "\r", 1);
		while (fgets(buff, sizeof(buff), fp))
			cmdline_in(cl, buff, strlen(buff));

		cmdline_in(cl, "\r", 1);
	}
	fclose(fp);

}

/** XDMA DPDK testapp */

int main(int argc, char **argv)
{
	int port_id   = 0;
	int ret = 0;
	const struct rte_memzone *mz = 0;
	struct cmdline *cl;

	/* Make sure the port is configured.  Zero everything and
	 * hope for same defaults
	 */

	printf("QDMA testapp rte eal init...\n");

	/* Make sure things are initialized ... */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	rte_log_set_global_level(RTE_LOG_DEBUG);

	printf("Ethernet Device Count: %d\n", (int)rte_eth_dev_count_avail());
	printf("Logical Core Count: %d\n", rte_lcore_count());

	num_ports = rte_eth_dev_count_avail();
	if (num_ports < 1)
		rte_exit(EXIT_FAILURE, "No Ethernet devices found."
			" Try updating the FPGA image.\n");

#if 1
	ret = parse_cmdline(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid argument\n");
#endif

	/* Make sure things are defined ... */
	do_sanity_checks();

	mz = rte_memzone_reserve_aligned("eth_devices", RTE_MAX_ETHPORTS *
					  sizeof(*rte_eth_devices), 0, 0, 4096);

	memcpy(mz->addr, &rte_eth_devices[0], RTE_MAX_ETHPORTS *
					sizeof(*rte_eth_devices));

	cl = cmdline_stdin_new(main_ctx, "xilinx-app> ");
	if (cl == NULL)
		rte_panic("Cannot create cmdline instance\n");

	/* if input commands file exists, then load commands from the file */
	if (filename != NULL) {
		load_file_cmds(cl);
		rte_delay_ms(100);
	} else
		cmdline_interact(cl);
	for (port_id = num_ports - 1; port_id >= 0; port_id--) {
		struct rte_device *dev;

		if (pinfo[port_id].num_queues)
			port_close(port_id);

		printf("Removing a device with port id %d\n", port_id);
		dev = rte_eth_devices[port_id].device;
		if (dev == NULL) {
			printf("Port id %d already removed\n", port_id);
			continue;
		}
		/*Detach the port, it will invoke device remove/uninit */
		if (rte_dev_remove(dev))
			printf("Failed to detach port '%d'\n", port_id);
	}
	cmdline_stdin_exit(cl);

	rte_delay_ms(5000);
	return 0;
}
