*************************************
DMA Latency Application (dma-latency)
*************************************

Xilinx-developed custom tool ``dma-latency`` is used to collect the latency metrics for unidirectional and bidirectional traffic. 

::

	usage: dma-latency [OPTIONS

	  -c (--config) config file that has configuration for IO

dma-latency tool takes a configuration file as input. The configuration file format is as below.

::

	name=mm_1_1
	mode=mm #mode
	dir=bi #dir
	pf_range=0:0 #no spaces
	q_range=0:0 #no spaces
	wb_acc=5
	tmr_idx=9
	cntr_idx=0
	trig_mode=usr_cnt
	rngidx=9
	ram_width=15 #31 bits - 2^31 = 2GB
	runtime=30 #secs
	num_threads=8
	bidir_en=1
	num_pkt=64
	pkt_sz=64


**Parameters**

- name : name of the configuration
- mode : mode of the queue, streaming\(st\) or memory mapped\(mm\). Mode defaults to mm.
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\).
- pf_range : Range of the PFs from 0-3 on which the performance metrics are to be collected.
- q_range : Range of the Queues from 0-2047 on which the performance metrics are to be collected.
- flags : queue flags
- wb_acc : write back accumulation index from CSR register \( 0 - 15 \)
- tmr_idx : timer index from CSR register \( 0 - 15 \)
- cntr_idx : Counter index from CSR register \( 0 - 15 \)
- trig_mode : trigger mode \(every, usr_cnt, usr, usr_tmr, dis\)
- rngidx : Ring index from CSR register \( 0 - 15 \)
- runtime : Duration of the performance runs, time in seconds.
- num_threads : number of threads to be used in dma-perf application to pump the traffic to queues
- bidir_en : Enable or Disable the bi-direction mode \( 0: Disable, 1: Enable \)
- num_pkt : number of packets
- pkt_sz : Packet size 