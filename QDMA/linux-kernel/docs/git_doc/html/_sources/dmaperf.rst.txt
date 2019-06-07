*********************
DMA Performance Tool
*********************

Xilinx-developed custom tool``dmaperf`` is used to collect the performance metrics for unidirectional and bidirectional traffic. 

The QDMA Linux kernel reference driver is a PCIe device driver, it manages the QDMA queues in the HW. The driver creates a character device for each queue pair configured.

Standard IO tools such as ``fio`` can be used for performing IO operations using the char device interface. 

However, most of the tools are limited to sending / receiving 1 packet at a time and wait for the processing of the packet to complete, so they are not able to keep the driver/ HW busy enough for performance measurement. Although fio also supports asynchronous interfaces, it does not continuously submit IO requests while polling for the completion in parallel.

To overcome this limitation, Xilinx developed dmaperf tool. It leverages the asynchronous functionality provided by libaio library. Using libaio, an application can submit IO request to the driver and the driver returns the control to the caller immediately (i.e., non-blocking). The completion notification is sent separately, so the application can then poll for the completion and free the buffer upon receiving the completion.

::

	usage: dmaperf [OPTIONS]

	  -c (--config) config file that has configuration for IO


dmaperf tool takes a configuration file as input. The configuration file format is as below.

::

	name=mm_1_1
	mode=mm #mode
	dir=bi #dir
	pf_range=0:0 #no spaces
	q_range=0:0 #no spaces
	flags= #no spaces
	wb_acc=5
	tmr_idx=9
	cntr_idx=0
	trig_mode=cntr_tmr
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
- pf_range : Range of the PFs from 0-3 on which the performance metrics to be collected.
- q_range : Range of the Queues from 0-2047 on which the performance metrics to be collected.
- flags : queue flags
- wb_acc : write back accumulation index from CSR register
- tmr_idx : timer index from CSR register
- cntr_idx : Counter index from CSR register
- trig_mode : trigger mode
- rngidx : Ring index from CSR register
- runtime : Duration of the performance runs, time in seconds.
- num_threads : number of threads to be used in dmaperf application to pump the traffic to queues
- bidir_en : Enable the bi-direction or not
- num_pkt : number of packets
- pkt_sz : Packet size