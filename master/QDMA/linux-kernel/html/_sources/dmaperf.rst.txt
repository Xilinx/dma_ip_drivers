*******************************
DMA Performance Tool (dma-perf)
*******************************

Xilinx-developed custom tool``dma-perf`` is used to collect the performance metrics for unidirectional and bidirectional traffic. 

=====
Note:
=====
The name of the application in previous releases before 2020.1 was ``dmaperf``. If user installed ``dmaperf`` application aleady in ``/usr/local/sbin`` area, make sure to uninstall the old application(s). Using ``dmaperf`` aginst latest driver would lead to undefined behaviour and errors may be observed.

Standard IO tools such as ``fio`` can be used for performing IO operations using the char device interface. 

However, most of the tools are limited to sending / receiving 1 packet at a time and wait for the processing of the packet to complete, so they are not able to keep the driver/ HW busy enough for performance measurement. Although fio also supports asynchronous interfaces, it does not continuously submit IO requests while polling for the completion in parallel.

To overcome this limitation, Xilinx developed dma-perf tool. It leverages the asynchronous functionality provided by libaio library. Using libaio, an application can submit IO request to the driver and the driver returns the control to the caller immediately (i.e., non-blocking). The completion notification is sent separately, so the application can then poll for the completion and free the buffer upon receiving the completion.

::

	usage: dma-perf [OPTIONS]

	  -c (--config) config file that has configuration for IO


dma-perf tool takes a configuration file as input. The configuration file format is as below.

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
	offset_q_en=1
	h2c_q_start_offset=0x100
	h2c_q_offset_intvl=10
	c2h_q_start_offset=0x200
	c2h_q_offset_intvl=20
	pci_bus=06
	pci_device=00


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
- mm_chnl : MM Channel \( 0 - 1 \) for Versal devices
- keyhole_en :  Enable the Keyhole feature
- offset : Offset to be written to for MM Performance Use cases
- aperture_sz : Size of aperture when using the keyhole feature
- offset_q_en : Offset queue enable (0-1) to enable H2C/C2H queues offsets.
- h2c_q_start_offset : Start address of H2C queue.
- h2c_q_offset_intvl : Fixed interval for subsequent H2C queues offsets.
- c2h_q_start_offset : Start address of C2H queue.
- c2h_q_offset_intvl : Fixed interval for subsequent C2H queues offsets.
- pci_bus : pci bus id.
- pci_device : pci device id.
