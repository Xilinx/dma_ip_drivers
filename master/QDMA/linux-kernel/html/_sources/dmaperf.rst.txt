*******************************
DMA Performance Tool (dma-perf)
*******************************

Xilinx-developed custom tool``dma-perf`` is used to collect the performance metrics for unidirectional and bidirectional traffic. This tool is used with AXI Stream Loopback Example Design only.

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


Example

::

	[xilinx@]# dma-perf -c perf_config.txt
	qdma65000-MM-0 H2C added.
	Added 1 Queues.
	Queues started, idx 0 ~ 0.
	qdma65000-MM-0 C2H added.
	Added 1 Queues.
	Queues started, idx 0 ~ 0.
	dmautils(16) threads
	Exit Check: tid =8, req_sbmitted=1495488 req_completed=1495488 dir=H2C, intime=0 loop_count=0,
	Exit Check: tid =13, req_sbmitted=1482752 req_completed=1482752 dir=C2H, intime=0 loop_count=0,
	Exit Check: tid =14, req_sbmitted=1494720 req_completed=1494720 dir=H2C, intime=0 loop_count=0,
	Exit Check: tid =8, req_sbmitted=1495488 req_completed=1495488 dir=H2C, intime=0 loop_count=0,
	Exit Check: tid =14, req_sbmitted=1494720 req_completed=1494720 dir=H2C, intime=0 loop_count=0,
	Exit Check: tid =6, req_sbmitted=1495488 req_completed=1495488 dir=H2C, intime=1495360 loop_count=1,
	Exit Check: tid =5, req_sbmitted=1485568 req_completed=1485568 dir=C2H, intime=1485440 loop_count=1,
	Exit Check: tid =11, req_sbmitted=1454208 req_completed=1454208 dir=C2H, intime=1454080 loop_count=1,
	Exit Check: tid =13, req_sbmitted=1482944 req_completed=1482944 dir=C2H, intime=1482752 loop_count=1,
	Exit Check: tid =0, req_sbmitted=1495168 req_completed=1495168 dir=H2C, intime=1494976 loop_count=2,
	Exit Check: tid =10, req_sbmitted=1495104 req_completed=1495104 dir=H2C, intime=1494912 loop_count=2,
	Exit Check: tid =12, req_sbmitted=1494592 req_completed=1494592 dir=H2C, intime=1494400 loop_count=2,
	Exit Check: tid =9, req_sbmitted=1486784 req_completed=1486784 dir=C2H, intime=1486592 loop_count=2,
	Exit Check: tid =15, req_sbmitted=1485248 req_completed=1485248 dir=C2H, intime=1485056 loop_count=2,
	Exit Check: tid =1, req_sbmitted=1486656 req_completed=1486656 dir=C2H, intime=1486592 loop_count=1,
	Exit Check: tid =4, req_sbmitted=1495872 req_completed=1495872 dir=H2C, intime=1495744 loop_count=1,
	Exit Check: tid =3, req_sbmitted=1486336 req_completed=1486336 dir=C2H, intime=1486208 loop_count=2,
	Exit Check: tid =7, req_sbmitted=1486400 req_completed=1486400 dir=C2H, intime=1486208 loop_count=2,
	Exit Check: tid =2, req_sbmitted=1495744 req_completed=1495744 dir=H2C, intime=1495616 loop_count=2,
	Exit Check: tid =10, req_sbmitted=1495296 req_completed=1495104 dir=H2C, intime=1494912 loop_count=10000,
	Exit Check: tid =11, req_sbmitted=1454464 req_completed=1454336 dir=C2H, intime=1454080 loop_count=10000,
	Exit Check: tid =5, req_sbmitted=1485632 req_completed=1485504 dir=C2H, intime=1485440 loop_count=10000,
	Exit Check: tid =0, req_sbmitted=1495616 req_completed=1495424 dir=H2C, intime=1494976 loop_count=10000,
	Exit Check: tid =12, req_sbmitted=1494912 req_completed=1494720 dir=H2C, intime=1494400 loop_count=10000,
	Exit Check: tid =6, req_sbmitted=1495616 req_completed=1495488 dir=H2C, intime=1495360 loop_count=10000,
	Stopped Queues 0 -> 0.
	Exit Check: tid =9, req_sbmitted=1486912 req_completed=1486720 dir=C2H, intime=1486592 loop_count=10000,
	Exit Check: tid =15, req_sbmitted=1485952 req_completed=1485760 dir=C2H, intime=1485056 loop_count=10000,
	Exit Check: tid =13, req_sbmitted=1483456 req_completed=1483264 dir=C2H, intime=1482752 loop_count=10000,
	Stopped Queues 0 -> 0.
	Deleted Queues 0 -> 0.
	Deleted Queues 0 -> 0.
	WRITE: total pps = 3987072 BW = 255.172608 MB/sec
	READ: total pps = 3950976 BW = 252.862464 MB/sec
	[xilinx@]#


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
