/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2017-2021 Xilinx, Inc. All rights reserved.
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
#include <rte_spinlock.h>
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

int do_recv_mm(int port_id, int fd, int queueid, int ld_size, int tot_num_desc)
{
	struct rte_mbuf *pkts[NUM_RX_PKTS] = { NULL };
	struct rte_device *dev;
	int nb_rx = 0, i = 0, ret = 0, num_pkts;
	int tdesc;
#ifdef PERF_BENCHMARK
	uint64_t prev_tsc, cur_tsc, diff_tsc;
#endif

	if (tot_num_desc == 0) {
		printf("Error: tot_num_desc : invalid value\n");
		return -1;
	}

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}

	printf("recv start num-desc: %d, with data-len: %u, "
			"last-desc-size:%d\n",
			tot_num_desc, pinfo[port_id].buff_size, ld_size);
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
		nb_rx = rte_eth_rx_burst(port_id, queueid, pkts, num_pkts);

#ifdef PERF_BENCHMARK
		cur_tsc = rte_rdtsc_precise();
		diff_tsc = cur_tsc - prev_tsc;
#endif

		if (nb_rx == 0) {
			printf("Error: dma_from_device failed to "
					"receive packets\n");
			rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
			return -1;
		}
#ifdef PERF_BENCHMARK
	   /* Calculate average operations processed per second */
		double pkts_per_second = ((double)nb_rx * rte_get_tsc_hz() /
							diff_tsc);

		/* Calculate average throughput (Gbps) in bits per second */
		double throughput_gbps = ((pkts_per_second *
				pinfo[port_id].buff_size) / 1000000000);
		printf("Throughput GBps %lf\n", throughput_gbps);
				printf("%16s%16s%16s%16s%16s%16s%16s\n\n",
					"Buf Size", "Burst Size",
					"pps", "Gbps", "freq", "Cycles",
					"Cycles/Buf");

			printf("%16u%16u%16.4lf%16.4lf%16 "
					""PRIu64"%16"PRIu64"%16"PRIu64"\n",
					pinfo[port_id].buff_size,
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
						pinfo[port_id].buff_size);
			rte_pktmbuf_free(mb);
#ifndef PERF_BENCHMARK
			printf("recv count: %d, with data-len: %d\n", i, ret);
#endif
		}
		tdesc = tdesc - nb_rx;
	}
	if (ld_size) {
		struct rte_mbuf *mb;
		nb_rx = rte_eth_rx_burst(port_id, queueid, pkts, 1);
		if (nb_rx != 0) {
			mb = pkts[0];
			ret = write(fd, rte_pktmbuf_mtod(mb, void*), ld_size);
			rte_pktmbuf_free(mb);
		}
	}
	fsync(fd);

	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
	ret = 0;

	printf("DMA received number of packets: %d, on queue-id:%d\n",
							nb_rx, queueid);
	return ret;
}

int do_recv_st(int port_id, int fd, int queueid, int input_size)
{
	struct rte_mbuf *pkts[NUM_RX_PKTS] = { NULL };
	int nb_rx = 0, ret = 0, tmp = 0, num_pkts, nb_pkts;
	int reg_val, loopback_en;
	int regval;
	int user_bar_idx;
	struct rte_mbuf *nxtmb;
	struct rte_device *dev;
	int qbase = pinfo[port_id].queue_base, diag;
	unsigned int max_completion_size, last_pkt_size = 0, total_rcv_pkts = 0;
	unsigned int max_rx_retry, rcv_count = 0, num_pkts_recv = 0;
	unsigned int i = 0, only_pkt = 0;

#ifdef DUMP_MEMPOOL_USAGE_STATS
	struct rte_mempool *mp;
#endif //DUMP_MEMPOOL_USAGE_STATS

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}

#ifdef DUMP_MEMPOOL_USAGE_STATS
	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);

	/* get the mempool from which to acquire buffers */
	if (mp == NULL) {
		printf("Could not find mempool with name %s\n",
			pinfo[port_id].mem_pool);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}
#endif //DUMP_MEMPOOL_USAGE_STATS

	user_bar_idx = pinfo[port_id].user_bar_idx;
	PciWrite(user_bar_idx, C2H_ST_QID_REG, (queueid + qbase), port_id);

	/* As per  hardware design a single completion will point to atmost
	 * 7 descriptors. So If the size of the buffer in descriptor is 4KB ,
	 * then a single completion which corresponds a packet can  give you
	 * atmost 28KB data.
	 *
	 * As per this when testing sizes beyond 28KB, one needs to split it
	 * up in chunks of 28KB, example : to test 56KB data size, set 28KB
	 * as packet length in AXI Master Lite BAR(user bar) 0x04 register and no of packets as 2
	 * in AXI Master Lite BAR(user bar) 0x20 register this would give you completions or
	 * packets, which needs to be combined as one in application.
	 */

	max_completion_size = pinfo[port_id].buff_size * 7;

	/* Calculate number of packets to receive and programming AXI Master Lite bar(user bar) */
	if (input_size == 0) /* zerobyte support uses one descriptor */
		num_pkts = 1;
	else if (input_size % max_completion_size != 0) {
		num_pkts = input_size / max_completion_size;
		last_pkt_size = input_size % max_completion_size;
	} else
		num_pkts = input_size / max_completion_size;

	if ((num_pkts == 0) && last_pkt_size) {
		num_pkts = 1;
		only_pkt = 1;
	}

	reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, port_id);
	reg_val &= C2H_CONTROL_REG_MASK;
	loopback_en = reg_val & ST_LOOPBACK_EN;
	if (!loopback_en) {
		PciWrite(user_bar_idx, C2H_PACKET_COUNT_REG, num_pkts, port_id);

		if (num_pkts > 1)
			PciWrite(user_bar_idx, C2H_ST_LEN_REG,
					max_completion_size, port_id);
		else if ((only_pkt == 1) && (last_pkt_size))
			PciWrite(user_bar_idx, C2H_ST_LEN_REG,
					last_pkt_size, port_id);
		else if (input_size == 0)
			PciWrite(user_bar_idx, C2H_ST_LEN_REG, input_size,
								port_id);
		else if (num_pkts == 1)
			PciWrite(user_bar_idx, C2H_ST_LEN_REG,
					max_completion_size, port_id);

		/* Start the C2H Engine */
		reg_val |= ST_C2H_START_VAL;
		PciWrite(user_bar_idx, C2H_CONTROL_REG, reg_val,
							port_id);
		regval = PciRead(user_bar_idx, C2H_PACKET_COUNT_REG, port_id);
		printf("BAR-%d is the QDMA C2H number of packets:0x%x,\n",
				user_bar_idx, regval);
	}

	while (num_pkts) {
		if (num_pkts > NUM_RX_PKTS)
			nb_pkts = NUM_RX_PKTS;
		else
			nb_pkts = num_pkts;

		max_rx_retry = RX_TX_MAX_RETRY;

		if ((only_pkt == 1) && (last_pkt_size))
			last_pkt_size = 0;

		 /* Immediate data Enabled*/
		if ((reg_val & ST_C2H_IMMEDIATE_DATA_EN)) {
			/* payload received is zero for the immediate data case.
			 * Therefore, no need to call the rx_burst function
			 * again in this case and set the num_pkts to nb_rx
			 * which is always Zero.
			 */
			diag = rte_pmd_qdma_set_immediate_data_state(port_id,
							queueid, 1);
			if (diag < 0) {
				printf("rte_pmd_qdma_set_immediate_data_state : "
						"failed\n");
				rte_spinlock_unlock(
					&pinfo[port_id].port_update_lock);
				return -1;
			}

			nb_rx = rte_eth_rx_burst(port_id, queueid, pkts,
								nb_pkts);
			num_pkts = num_pkts_recv = nb_rx;

			/* Reset the queue's DUMP_IMMEDIATE_DATA mode */
			diag = rte_pmd_qdma_set_immediate_data_state(port_id,
							queueid, 0);
			if (diag < 0) {
				printf("rte_pmd_qdma_set_immediate_data_state : "
						"failed\n");
				rte_spinlock_unlock(
					&pinfo[port_id].port_update_lock);
				return -1;
			}
		} else {
			/* try to receive RX_BURST_SZ packets */

			nb_rx = rte_eth_rx_burst(port_id, queueid, pkts,
							nb_pkts);

			/* For zero byte packets, do not continue looping */
			if (input_size == 0)
				break;

			tmp = nb_rx;
			while ((nb_rx < nb_pkts) && max_rx_retry) {
				rte_delay_us(1);
				nb_pkts -= nb_rx;
				nb_rx = rte_eth_rx_burst(port_id, queueid,
								&pkts[tmp],
								nb_pkts);
				tmp += nb_rx;
				max_rx_retry--;
			}
			num_pkts_recv = tmp;
			if ((max_rx_retry == 0) && (num_pkts_recv == 0)) {
				printf("ERROR: rte_eth_rx_burst failed for "
						"port %d queue id %d, Expected pkts = %d "
						"Received pkts = %u\n",
						port_id, queueid,
						nb_pkts, num_pkts_recv);
				break;
			}
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
			printf("recv count: %u, with data-len: %d\n",
					i + rcv_count, ret);
#endif
			ret = 0;
		}
		rcv_count += i;
#ifdef DUMP_MEMPOOL_USAGE_STATS
		printf("%s(): %d: queue id = %d, mbuf_avail_count = %d, "
				"mbuf_in_use_count = %d, num_pkts_recv = %u\n",
				__func__, __LINE__, queueid,
				rte_mempool_avail_count(mp),
				rte_mempool_in_use_count(mp), num_pkts_recv);
#endif //DUMP_MEMPOOL_USAGE_STATS
		num_pkts = num_pkts - num_pkts_recv;
		total_rcv_pkts += num_pkts_recv;
		if ((num_pkts == 0) && last_pkt_size) {
			num_pkts = 1;
			if (!loopback_en) {
				/* Stop the C2H Engine */
				reg_val = PciRead(user_bar_idx,
							C2H_CONTROL_REG,
							port_id);
				reg_val &= C2H_CONTROL_REG_MASK;
				reg_val &= ~(ST_C2H_START_VAL);
				PciWrite(user_bar_idx, C2H_CONTROL_REG, reg_val,
				port_id);

				/* Update number of packets as 1 and
				 * packet size as last packet length
				 */
				PciWrite(user_bar_idx, C2H_PACKET_COUNT_REG,
					num_pkts, port_id);

				PciWrite(user_bar_idx, C2H_ST_LEN_REG,
					last_pkt_size, port_id);

				/* Start the C2H Engine */
				reg_val = PciRead(user_bar_idx,
							C2H_CONTROL_REG,
							port_id);
				reg_val &= C2H_CONTROL_REG_MASK;
				reg_val |= ST_C2H_START_VAL;
				PciWrite(user_bar_idx, C2H_CONTROL_REG, reg_val,
							port_id);
			}
			last_pkt_size = 0;
			continue;
		}
	}
	/* Stop the C2H Engine */
	if (!loopback_en) {
		reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, port_id);
		reg_val &= C2H_CONTROL_REG_MASK;
		reg_val &= ~(ST_C2H_START_VAL);
		PciWrite(user_bar_idx, C2H_CONTROL_REG, reg_val,
				port_id);
	}
	printf("DMA received number of packets: %u, on queue-id:%d\n",
			total_rcv_pkts, queueid);
	fsync(fd);
	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
	return 0;
}

int do_xmit(int port_id, int fd, int queueid, int ld_size, int tot_num_desc,
				int zbyte)
{
	struct rte_mempool *mp;
	struct rte_mbuf *mb[NUM_RX_PKTS] = { NULL };
	struct rte_device *dev;
	int ret = 0, nb_tx, i = 0, tdesc, num_pkts = 0, total_tx = 0, reg_val;
	int tmp = 0, user_bar_idx;
	int qbase = pinfo[port_id].queue_base;
	uint32_t max_tx_retry;

#ifdef PERF_BENCHMARK
	uint64_t prev_tsc, cur_tsc, diff_tsc;
#endif

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
			rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}

	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);
	/* get the mempool from which to acquire buffers */
	if (mp == NULL) {
		printf("Could not find mempool with name %s\n",
				pinfo[port_id].mem_pool);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}

	tdesc = tot_num_desc;
	user_bar_idx = pinfo[port_id].user_bar_idx;

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
			if (mb[i] == NULL) {
				printf(" #####Cannot "
						"allocate mbuf packet\n");
				rte_spinlock_unlock(
					&pinfo[port_id].port_update_lock);
				return -1;
			}

			if (!zbyte)
				ret = read(fd, rte_pktmbuf_mtod(mb[i], void *),
						pinfo[port_id].buff_size);
			if (ret < 0) {
				printf("Error: Could not the read "
						"input-file\n");
				rte_spinlock_unlock(
					&pinfo[port_id].port_update_lock);
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
				port_id);
		/* try to transmit TX_BURST_SZ packets */

#ifdef PERF_BENCHMARK
		prev_tsc = rte_rdtsc_precise();
#endif
		nb_tx = rte_eth_tx_burst(port_id, queueid, mb, num_pkts);
#ifdef PERF_BENCHMARK
		cur_tsc = rte_rdtsc_precise();
		diff_tsc = cur_tsc - prev_tsc;
	   /* Calculate average operations processed per second */
		double pkts_per_second = ((double)nb_tx * rte_get_tsc_hz() /
								diff_tsc);

		/* Calculate average throughput (Gbps) in bits per second */
		double throughput_gbps = ((pkts_per_second *
				pinfo[port_id].buff_size) / 1000000000);
		printf("Throughput GBps %lf\n", throughput_gbps);
				printf("%12s%12s%12s%12s%12s%12s%12s\n\n",
					"Buf Size", "Burst Size",
					"pps", "Gbps", "freq", "Cycles",
					"Cycles/Buf");

			printf("%12u%12u%12.4lf%12.4lf%12"
					""PRIu64"%12"PRIu64"%12"PRIu64"\n",
					pinfo[port_id].buff_size,
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
			nb_tx = rte_eth_tx_burst(port_id, queueid, &mb[tmp],
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
						port_id, queueid);
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
			printf(" #####Cannot allocate mbuf "
						"packet\n");
			rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
			return -1;
		}
		ret = read(fd, rte_pktmbuf_mtod(mb[0], void *), ld_size);
		if (ret < 0) {
			printf("Error: Could not read the input-file\n");
			rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
			return -1;
		}
		mb[0]->nb_segs = 1;
		mb[0]->next = NULL;
		rte_pktmbuf_data_len(mb[0]) = (uint16_t)ret;
		rte_pktmbuf_pkt_len(mb[0])  = (uint16_t)ret;

		nb_tx = rte_eth_tx_burst(port_id, queueid, mb, 1);
		if (nb_tx == 0)
			rte_pktmbuf_free(mb[0]);
	}

	reg_val = PciRead(user_bar_idx, C2H_CONTROL_REG, port_id);
	reg_val &= C2H_CONTROL_REG_MASK;
	if (!(reg_val & ST_LOOPBACK_EN)) {
		reg_val = PciRead(user_bar_idx, H2C_STATUS_REG, port_id);
		printf("BAR-%d is the QDMA H2C transfer match: 0x%x,\n",
			user_bar_idx, reg_val);

		/** TO clear H2C DMA write **/
		PciWrite(user_bar_idx, H2C_CONTROL_REG, 0x1, port_id);
	}

	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
	return 0;
}

static int dev_reset_callback(uint16_t port_id,
				enum rte_eth_event_type type,
				void *param __rte_unused, void *ret_param)
{
	int ret = 0;

	RTE_SET_USED(ret_param);
	printf("%s is received\n", __func__);

	if (type != RTE_ETH_EVENT_INTR_RESET) {
		printf("Error: Invalid event value. "
				"Expected = %d, Received = %d\n",
				RTE_ETH_EVENT_INTR_RESET, type);
		return -ENOMSG;
	}

	ret = port_reset(port_id, pinfo[port_id].num_queues,
			pinfo[port_id].st_queues,
			pinfo[port_id].nb_descs,
			pinfo[port_id].buff_size);
	if (ret < 0)
		printf("Error: Failed to reset port: %d\n", port_id);

	return ret;
}

static int dev_remove_callback(uint16_t port_id,
				enum rte_eth_event_type type,
				void *param __rte_unused, void *ret_param)
{
	int ret = 0;

	RTE_SET_USED(ret_param);
	printf("%s is received\n", __func__);

	if (type != RTE_ETH_EVENT_INTR_RMV) {
		printf("Error: Invalid event value. "
				"Expected = %d, Received = %d\n",
				RTE_ETH_EVENT_INTR_RMV, type);
		return -ENOMSG;
	}

	ret = port_remove(port_id);
	if (ret < 0)
		printf("Error: Failed to remove port: %d\n", port_id);

	return 0;
}

void port_close(int port_id)
{
	struct rte_mempool *mp;
	struct rte_device  *dev;
	struct rte_pmd_qdma_dev_attributes dev_attr;
	int user_bar_idx;
	int reg_val;
	int ret = 0;

	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		return;
	}

	user_bar_idx = pinfo[port_id].user_bar_idx;
	ret = rte_pmd_qdma_get_device_capabilities(port_id, &dev_attr);
	if (ret < 0) {
		printf("rte_pmd_qdma_get_device_capabilities failed for port: %d\n",
			port_id);
		return;
	}

	if ((dev_attr.device_type == RTE_PMD_QDMA_DEVICE_SOFT)
			&& (dev_attr.ip_type == RTE_PMD_EQDMA_SOFT_IP)) {
		PciWrite(user_bar_idx, C2H_CONTROL_REG,
				C2H_STREAM_MARKER_PKT_GEN_VAL,
				port_id);
		unsigned int retry = 50;
		while (retry) {
			usleep(500);
			reg_val = PciRead(user_bar_idx,
				C2H_STATUS_REG, port_id);
			if (reg_val & MARKER_RESPONSE_COMPLETION_BIT)
				break;

			printf("Failed to receive c2h marker completion, retry count = %u\n",
					(50 - (retry-1)));
			retry--;
		}
	}

	rte_eth_dev_stop(port_id);

	rte_pmd_qdma_dev_close(port_id);

	pinfo[port_id].num_queues = 0;

	mp = rte_mempool_lookup(pinfo[port_id].mem_pool);

	if (mp != NULL)
		rte_mempool_free(mp);
}

int port_reset(int port_id, int num_queues, int st_queues,
				int nb_descs, int buff_size)
{
	int ret = 0;
	struct rte_device   *dev;

	printf("%s is received\n", __func__);

	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		return -1;
	}

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	port_close(port_id);

	ret = rte_eth_dev_reset(port_id);
	if (ret < 0) {
		printf("Error: Failed to reset device for port: %d\n", port_id);
		rte_spinlock_unlock(&pinfo[port_id].port_update_lock);
		return -1;
	}

	ret = port_init(port_id, num_queues, st_queues,
				nb_descs, buff_size);
	if (ret < 0)
		printf("Error: Failed to initialize port: %d\n", port_id);

	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);

	if (!ret)
		printf("Port reset done successfully on port-id: %d\n",
			port_id);

	return ret;
}

int port_remove(int port_id)
{
	struct rte_device *dev;
	int ret = 0;

	printf("%s is received\n", __func__);

	/* Detach the port, it will invoke device remove/uninit */
	printf("Removing a device with port id %d\n", port_id);
	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed\n", port_id);
		return 0;
	}

	rte_spinlock_lock(&pinfo[port_id].port_update_lock);

	port_close(port_id);

	ret = rte_dev_remove(dev);
	if (ret < 0)
		printf("Failed to remove device on port_id: %d\n", port_id);

	rte_spinlock_unlock(&pinfo[port_id].port_update_lock);

	if (!ret)
		printf("Port remove done successfully on port-id: %d\n",
			port_id);

	return ret;
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
		case '?':
			/* Long eal options */
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

int port_init(int port_id, int num_queues, int st_queues,
				int nb_descs, int buff_size)
{
	struct rte_mempool *mbuf_pool;
	struct rte_eth_conf	    port_conf;
	struct rte_eth_txconf   tx_conf;
	struct rte_eth_rxconf   rx_conf;
	int                     diag, x;
	uint32_t                queue_base, nb_buff;
	struct rte_device       *dev;

	printf("Setting up port :%d.\n", port_id);

	dev = rte_eth_devices[port_id].device;
	if (dev == NULL) {
		printf("Port id %d already removed. "
			"Relaunch application to use the port again\n",
			port_id);
		return -1;
	}

	snprintf(pinfo[port_id].mem_pool, RTE_MEMPOOL_NAMESIZE,
			MBUF_POOL_NAME_PORT, port_id);

	/* Mbuf packet pool */
	nb_buff = ((nb_descs) * num_queues * 2);

	/* NUM_RX_PKTS should be added to every queue as that many descriptors
	 * can be pending with application after Rx processing but before
	 * consumed by application or sent to Tx
	 */
	nb_buff += ((NUM_RX_PKTS) * num_queues);

	mbuf_pool = rte_pktmbuf_pool_create(pinfo[port_id].mem_pool, nb_buff,
			MP_CACHE_SZ, 0, buff_size +
			RTE_PKTMBUF_HEADROOM,
			rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, " Cannot create mbuf pkt-pool\n");
#ifdef DUMP_MEMPOOL_USAGE_STATS
	printf("%s(): %d: mpool = %p, mbuf_avail_count = %d,"
			" mbuf_in_use_count = %d,"
			"nb_buff = %u\n", __func__, __LINE__, mbuf_pool,
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
	diag = rte_pmd_qdma_get_bar_details(port_id,
				&(pinfo[port_id].config_bar_idx),
				&(pinfo[port_id].user_bar_idx),
				&(pinfo[port_id].bypass_bar_idx));

	if (diag < 0)
		rte_exit(EXIT_FAILURE, "rte_pmd_qdma_get_bar_details failed\n");

	printf("QDMA Config bar idx: %d\n", pinfo[port_id].config_bar_idx);
	printf("QDMA AXI Master Lite bar idx: %d\n", pinfo[port_id].user_bar_idx);
	printf("QDMA AXI Bridge Master bar idx: %d\n", pinfo[port_id].bypass_bar_idx);

	/* configure the device to use # queues */
	diag = rte_eth_dev_configure(port_id, num_queues, num_queues,
			&port_conf);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "Cannot configure port %d (err=%d)\n",
				port_id, diag);

	diag = rte_pmd_qdma_get_queue_base(port_id, &queue_base);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "rte_pmd_qdma_get_queue_base : Querying of "
				"QUEUE_BASE failed\n");
	pinfo[port_id].queue_base = queue_base;
	pinfo[port_id].num_queues = num_queues;
	pinfo[port_id].st_queues = st_queues;
	pinfo[port_id].buff_size = buff_size;
	pinfo[port_id].nb_descs = nb_descs;

	for (x = 0; x < num_queues; x++) {
		if (x < st_queues) {
			diag = rte_pmd_qdma_set_queue_mode(port_id, x,
					RTE_PMD_QDMA_STREAMING_MODE);
			if (diag < 0)
				rte_exit(EXIT_FAILURE, "rte_pmd_qdma_set_queue_mode : "
						"Passing of QUEUE_MODE "
						"failed\n");
		} else {
			diag = rte_pmd_qdma_set_queue_mode(port_id, x,
					RTE_PMD_QDMA_MEMORY_MAPPED_MODE);
			if (diag < 0)
				rte_exit(EXIT_FAILURE, "rte_pmd_qdma_set_queue_mode : "
						"Passing of QUEUE_MODE "
						"failed\n");
		}

		diag = rte_eth_tx_queue_setup(port_id, x, nb_descs, 0,
				&tx_conf);
		if (diag < 0)
			rte_exit(EXIT_FAILURE, "Cannot setup port %d "
					"TX Queue id:%d "
					"(err=%d)\n", port_id, x, diag);
		rx_conf.rx_thresh.wthresh = DEFAULT_RX_WRITEBACK_THRESH;
		diag = rte_eth_rx_queue_setup(port_id, x, nb_descs, 0,
				&rx_conf, mbuf_pool);
		if (diag < 0)
			rte_exit(EXIT_FAILURE, "Cannot setup port %d "
					"RX Queue 0 (err=%d)\n", port_id, diag);
	}

	diag = rte_eth_dev_start(port_id);
	if (diag < 0)
		rte_exit(EXIT_FAILURE, "Cannot start port %d (err=%d)\n",
				port_id, diag);

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
	const struct rte_memzone *mz = 0;
	struct cmdline *cl;
	int port_id   = 0;
	int ret = 0;
	int curr_avail_ports = 0;

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

	for (port_id = 0; port_id < num_ports; port_id++)
		rte_spinlock_init(&pinfo[port_id].port_update_lock);

	ret = rte_eth_dev_callback_register(RTE_ETH_ALL, RTE_ETH_EVENT_INTR_RMV,
				dev_remove_callback, NULL);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Failed to register dev_remove_callback\n");

	ret = rte_eth_dev_callback_register(RTE_ETH_ALL,
			RTE_ETH_EVENT_INTR_RESET,
			dev_reset_callback, NULL);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Failed to register dev_reset_callback\n");

#if 1
	ret = parse_cmdline(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid argument\n");
#endif

	/* Make sure things are defined ... */
	do_sanity_checks();

	mz = rte_memzone_reserve_aligned("eth_devices", RTE_MAX_ETHPORTS *
					  sizeof(*rte_eth_devices), 0, 0, 4096);
	if (mz == NULL)
		rte_exit(EXIT_FAILURE, "Failed to allocate aligned memzone\n");

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

	rte_eth_dev_callback_unregister(RTE_ETH_ALL, RTE_ETH_EVENT_INTR_RMV,
			dev_remove_callback, NULL);

	rte_eth_dev_callback_unregister(RTE_ETH_ALL, RTE_ETH_EVENT_INTR_RESET,
			dev_reset_callback, NULL);

	curr_avail_ports = rte_eth_dev_count_avail();
	if (!curr_avail_ports)
		printf("Ports already removed\n");
	else {
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
			/* Detach the port, it will invoke
			 * device remove/uninit
			 */
			if (rte_dev_remove(dev))
				printf("Failed to detach port '%d'\n", port_id);
		}
	}

	cmdline_stdin_exit(cl);

	rte_delay_ms(5000);
	return 0;
}
