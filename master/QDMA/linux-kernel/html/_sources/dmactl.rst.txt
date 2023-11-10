*********************************
DMA Control Application (dma-ctl)
*********************************

QDMA driver comes with a command-line configuration utility called ``dma-ctl`` to manage the driver.

The Xilinx QDMA control tool, ``dma-ctl`` is a command Line utility which is installed in ``/usr/local/sbin/`` and allows administration of the Xilinx QDMA queues. Make sure that the installation path ``/usr/local/sbin/`` is added to the ``PATH`` variable.

=====
Note:
=====
The name of the application in previous releases before 2020.1 was ``dmactl``. If user installed ``dmactl`` application aleady in ``/usr/local/sbin`` area, make sure to uninstall the old application(s). Using ``dmactl`` aginst latest driver would lead to undefined behaviour and errors may be observed.


``dma-ctl`` application can perform the following functions:

- Query the list of functions binded QDMA driver 
- Query control and configuration 

   - List all the queues on a device
   - Add/configure a new queue on a device
   - Start an already added/configured queue, i.e., bring the queue to on line mode
   - Stop a started queue, i.e., bring the queue to off line mode
   - Delete an already configured queue
   
- Register access

   - Read a register
   - Write a register
   - Dump the qdma configuration bar and user bar registers
   
- Debug helper

   - Display a queue's configuration parameters
   - Display a queue's descriptor ring entries
   - Display a c2h queue's completion ring entries
   - Display the interrupt ring entries 

For dma-ctl help run ``dma-ctl –h``

For more details on the dma-ctl tool commands and options for each command, refer to dma-ctl man page.

For dma-ctl man page, run ``man dma-ctl``

==========================
Device Management Commands
==========================

List devices
------------

command: ``dma-ctl dev list``

This command lists all the physical and virtual functions available in the system along with the number of queues assigned to each function, queue base and queue max.
Each qdma device name is listed as ``qdma<bbddf>`` where ``<bbddf>`` is bus number, device number and function number.

Initially when the system is started, the queues are not assigned to any of the available functions.

::


	[xilinx@]# dma-ctl dev list

	qdma01000	0000:01:00.0	max QP: 0, -~-
	qdma01001	0000:01:00.1	max QP: 0, -~-
	qdma01002	0000:01:00.2	max QP: 0, -~-
	qdma01003	0000:01:00.3	max QP: 0, -~-


	qdmavf01004	0000:01:00.4	max QP: 0, -~-
	

	[xilinx@]# echo 100 > /sys/bus/pci/devices/0000\:01\:00.0/qdma/qmax 
	[xilinx@]# echo 100 > /sys/bus/pci/devices/0000\:01\:00.1/qdma/qmax 
	[xilinx@]# echo 100 > /sys/bus/pci/devices/0000\:01\:00.2/qdma/qmax 
	[xilinx@]# echo 100 > /sys/bus/pci/devices/0000\:01\:00.3/qdma/qmax 
	[xilinx@]# echo 100 > /sys/bus/pci/devices/0000\:01\:00.4/qdma/qmax 
	[xilinx@]# dma-ctl dev list

	qdma01000	0000:01:00.0	max QP: 100, 0~99
	qdma01001	0000:01:00.1	max QP: 100, 100~199
	qdma01002	0000:01:00.2	max QP: 100, 200~299
	qdma01003	0000:01:00.3	max QP: 100, 300~399


	qdmavf01004	0000:01:00.4	max QP: 100, 400~499



=========================
Queue Management Commands
=========================

List Version
------------

command: ``dma-ctl qdma<bbddf> cap``

This command lists hardware and software version details and device capabilities.
::

	[xilinx@]# dma-ctl qdma01000 cap
	=============Hardware Version============

	RTL Version         : RTL Patch
	Vivado ReleaseID    : vivado 2020.2
	QDMA Device Type    : Soft IP
	QDMA IP Type        : QDMA Soft IP
	============Software Version============

	qdma driver version : 2020.2.0.0

	=============Hardware Capabilities============

	Number of PFs supported                : 4
	Total number of queues supported       : 2048
	MM channels                            : 1
	FLR Present                            : yes
	ST enabled                             : yes
	MM enabled                             : yes
	Mailbox enabled                        : yes
	MM completion enabled                  : no
	Debug Mode enabled                     : no
	Desc Engine Mode                       : Internal and Bypass mode


List Device Statistics
-----------------------

command: ``dma-ctl qdma<bbddf> stat``

This command lists the statistics accumulated for this device
::

	[xilinx@]# dma-ctl qdma01000 stat

	qdma01000:statistics
	Total MM H2C packets processed = 0
	Total MM C2H packets processed = 0
	Total ST H2C packets processed = 0
	Total ST C2H packets processed = 0
	Min Ping Pong Latency = 0
	Max Ping Pong Latency = 0
	Avg Ping Pong Latency = 0

Use ``dma-ctl qdma01000 stat clear`` to clear the statistics collected

Add a Queue
-----------

command: ``dma-ctl qdma<bbddf> q add idx <N> [mode <st|mm>] [dir <h2c|c2h|bi>]``

This command allows the user to add a queue.

**Parameters**

- <N> : Queue number
- mode : mode of the queue, streaming\(st\) or memory mapped\(mm\). Mode defaults to mm.
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Direction defaults to h2c.

::

   [xilinx@]# dma-ctl qdma01000 q add idx 4 mode mm dir h2c

   qdma01000-MM-4 H2C added.
   Added 1 Queues.

Add a List of Queues
--------------------

command: ``dma-ctl qdma<bbddf> q add list <start_idx> <N>  [ mode <st|mm> ] [ dir <h2c|c2h|bi> ]``

This command allows the user to add a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> :Number of queues to add
- mode : mode of the queue, streaming\(st\) or memory mapped\(mm\)
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::

   [xilinx@]# dma-ctl qdma01000 q add list 1 4 mode mm dir h2c

   qdma01000-MM-1 H2C added.
   qdma01000-MM-2 H2C added.
   qdma01000-MM-3 H2C added.
   qdma01000-MM-4 H2C added.
   Added 4 Queues.
   
Start a Queue
-------------
This command allows the user to start a queue.

command: 

   ``dma-ctl qdma<bbddf> q start idx <N> [dir <h2c|c2h|bi>] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>]
   [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en]
   [dis_cmpl_status] [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en] [fetch_credit <h2c|c2h|bi|none>] [c2h_cmpl_intr_en]
   [cmpl_ovf_dis] [mm_chn <0:1>] [aperture_sz <Power of 2 aperture size>]``


**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card \(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.
- idx_ringsz: CSR register ring size index \( 0 - 15 \)
- idx_bufsz : CSR register buffer size index \( 0 - 15 \)
- idx_tmr : CSR register timer index \( 0 - 15 \)
- idx_cntr: CSR register counter index \( 0 - 15 \)
- trigmode: Timer trigger mode \(every, usr_cnt, usr, usr_tmr, dis\)
- cmptsz : Completion size \( 0: 8 bytes, 1: 16 bytes, 2:32 bytes, 3:64 bytes\)
- sw_desc_sz : Descriptor size \( 3: 64 bytes\)
- desc_bypass_en : Enable descriptor bypass
- pfetch_en : Enable prefetch \( 0: Disable, 1: Enable \)
- pfetch_bypass_en : Enable prefetch bypass \( 0: Disable, 1: Enable \)
- dis_cmpl_status : Disable completion status update \( 1: Disable, 0: Enable \)
- dis_cmpl_status_acc : Disable completion status accumulation \( 1: Disable, 0: Enable \)
- dis_cmpl_status_pend_chk : Disable completion status pending check \( 1: Disable, 0: Enable \)
- c2h_udd_en : Enable immediate data\(User Defined Data\) \( 0: Disable, 1: Enable \)
- fetch_credit : Enable fetch credit \(host-to-card \(h2c\), card-to-host \(c2h\), both \(bi\), disable \(none\)\)
- c2h_cmpl_intr_en : Enable c2h completion interval \( 0: Disable, 1: Enable \)
- cmpl_ovf_dis : Disable completion over flow check \( 1: Disable, 0: Enable \)
- mm_chn : MM Channel to be used(0/1)
- aperture_sz :  aperture size power of 2 for keyhole feature

Note on keyhole feature 
  Provides support of programming over PCIe. QDMA SW provides a capability to program a .pdi file to a specific aperture within a Versal device.
  This enables the DMA to write data to a fixed size FPGA aperture. When the end of the user specified aperture size is reached, transactions continue to transfer data by looping back to the start of the aperture window. This is repeated until all data has been programmed into the user specified aperture window. ``dma-to-device`` can be used to write to a specific offset in the RAM with the aperture specified at the time when the Q is started.

::

   [xilinx@]# dma-ctl qdma01000 q start idx 4 dir h2c
   dma-ctl: Info: Default ring size set to 2048

   1 Queues started, idx 4 ~ 4.

Start a List of Queues
----------------------

command:

   ``dma-ctl qdma<bbddf> q start list <start_idx> <N> [dir <h2c|c2h|bi>] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>] 
   [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en]
   [dis_cmpl_status] [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en] [fetch_credit <h2c|c2h|bi|none>] [c2h_cmpl_intr_en]
   [cmpl_ovf_dis] [mm_chn <0:1>] [aperture_sz <Power of 2 aperture size>]``

This command allows the user to start a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to start
- dir : direction of the queue, host-to-card \(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.
- idx_ringsz: CSR register ring size index \( 0 - 15 \)
- idx_bufsz : CSR register buffer size index \( 0 - 15 \)
- idx_tmr : CSR register timer index \( 0 - 15 \)
- idx_cntr: CSR register counter index \( 0 - 15 \)
- trigmode: Timer trigger mode \(every, usr_cnt, usr, usr_tmr, dis\)
- cmptsz : Completion size \( 0: 8 bytes, 1: 16 bytes, 2:32 bytes, 3:64 bytes\)
- desc_bypass_en : Enable descriptor bypass \( 0: Disable, 1: Enable \)
- pfetch_en : Enable prefetch \( 0: Disable, 1: Enable \)
- pfetch_bypass_en : Enable prefetch bypass \( 0: Disable, 1: Enable \)
- dis_cmpl_status : Disable completion status update
- dis_cmpl_status_pend_chk : Disable completion status pending check
- c2h_udd_en : Enable immdeiate data\(User Defined Data\)
- fetch_credit : Enable fetch credit \(host-to-card \(h2c\), card-to-host \(c2h\), both \(bi\), disable \(none\)\)
- c2h_cmpl_intr_en : Enable c2h completion interval \( 0: Disable, 1: Enable \)
- cmpl_ovf_dis : Disable completion over flow check \( 1: Disable, 0: Enable \)
- mm_chn : MM Channel to be used(0/1)\)
- aperture_sz :  aperture size power of 2

::

   [xilinx@]# dma-ctl qdma01000 q start list 1 4 dir h2c

   Started Queues 1 -> 4.

List of Queues added
--------------------

command: ``dma-ctl qdma<bbddf> q list <start_idx> <num_Qs>``

This command allows user to list queues added/configured.

**Parameter**

- <start_idx> : Starting queue number
- <num_Qs> : Number of queues to list

::

   [xilinx@]# dma-ctl qdma01000 q list 1 4
   H2C Q: 4, C2H Q: 0, CMPT Q 0.
   qdma06000-MM-1 H2C online
           hw_ID 1, thp ?, desc 0x0000000050bf1609/0xfffc0000, 2048
   qdma06000-MM-2 H2C online
           hw_ID 2, thp ?, desc 0x00000000e4e32bca/0xfffa0000, 2048
   qdma06000-MM-3 H2C online
           hw_ID 3, thp ?, desc 0x00000000a84d8d83/0xfff80000, 2048
   qdma06000-MM-4 H2C online
           hw_ID 4, thp ?, desc 0x00000000ddf89665/0xfff60000, 2048

Stop a Queue
------------

command: ``dma-ctl qdma<bbddf> q stop idx <N> [dir <h2c|c2h|bi>]``

This command allows the user to stop a queue.

**Parameters**

- <N> : Queue number
- dir : direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dma-ctl qdma01000 q stop idx 4 dir h2c
   dma-ctl: Info: Default ring size set to 2048

   Stopped Queues 4 -> 4.
   
Stop a List of Queues
---------------------

command: ``dma-ctl qdma<bbddf> q stop list <start_idx> <N> [dir <h2c|c2h|bi>]``

This command allows the user to stop a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to delete
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dma-ctl qdma01000 q stop list 1 4 dir h2c

   Stopped Queues 1 -> 4.

Delete a Queue
--------------

command: ``dma-ctl qdma<bbddf> q del idx <N> [dir <h2c|c2h|bi>]``

This command allows the user to delete a queue.

**Parameters**

- <N> : Queue number
- dir : direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dma-ctl qdma01000 q del idx 4 dir h2c

   Deleted Queues 4 -> 4.
   
Delete a List of Queues
-----------------------

command: ``dma-ctl <bbddf> q del list <start_idx> <N> [ dir <h2c|c2h|bi> ]``

This command allows the user to delete a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to delete
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::

   [xilinx@]# dma-ctl qdma01000 q del list 1 4 dir h2c

   Deleted Queues 1 -> 4.
   
Dump Queue Information
----------------------

command: ``dma-ctl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>]``

Dump the queue information

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

Sample output is given below:

::


	[xilinx@]# dma-ctl qdma01000 q dump idx 1 dir bi

	qdma01000-ST-0 H2C online
		hw_ID 0, thp ?, desc 0x000000002faff247/0x2a878000, 1536

		cmpl status: 0x00000000377ea451, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x2a878000 713523200
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x1        1
			IRQ Request                                     0x0        0
			Writeback Error Sent                            0x0        0
			Error                                           0x0        0
			Interrupt No Last                               0x0        0
			Port Id                                         0x0        0
			Interrupt Enable                                0x1        1
			Writeback Enable                                0x1        1
			MM Channel                                      0x0        0
			Bypass Enable                                   0x0        0
			Descriptor Size                                 0x1        1
			Ring Size                                       0x9        9
			Fetch Max                                       0x0        0
			Address Translation                             0x0        0
			Write back/Intr Interval                        0x1        1
			Write back/Intr Check                           0x1        1
			Fetch Credit Enable                             0x0        0
			Queue Enable                                    0x1        1
			Function Id                                     0x0        0
			IRQ Arm                                         0x1        1
			PIDX                                            0x0        0

		HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x0        0
			Credits Consumed                                0x0        0
			CIDX                                            0x0        0

		CREDIT CTXT:
			Credit                                          0x0        0

		INTR CTXT:
			at                                              0x0        0
			pidx                                            0x0        0
			page_size                                       0x0        0
			baddr_4k (High)                                 0x0        0
			baddr_4k (Low)                                  0x2a801000 713035776
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-0 C2H online
		hw_ID 0, thp ?, desc 0x00000000219f976e/0x2a85c000, 1536
		cmpt desc 0x0000000086ae1f7a/0x2a870000, 2048

		cmpl status: 0x00000000a17113db, 00000000 00000000
		CMPT CMPL STATUS: 0x00000000973734af, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x2a85c000 713408512
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x0        0
			IRQ Request                                     0x0        0
			Writeback Error Sent                            0x0        0
			Error                                           0x0        0
			Interrupt No Last                               0x0        0
			Port Id                                         0x0        0
			Interrupt Enable                                0x0        0
			Writeback Enable                                0x0        0
			MM Channel                                      0x0        0
			Bypass Enable                                   0x0        0
			Descriptor Size                                 0x0        0
			Ring Size                                       0x9        9
			Fetch Max                                       0x0        0
			Address Translation                             0x0        0
			Write back/Intr Interval                        0x1        1
			Write back/Intr Check                           0x0        0
			Fetch Credit Enable                             0x1        1
			Queue Enable                                    0x1        1
			Function Id                                     0x0        0
			IRQ Arm                                         0x0        0
			PIDX                                            0x5ff      1535

		HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x0        0
			Credits Consumed                                0x0        0
			CIDX                                            0x0        0

		CREDIT CTXT:
			Credit                                          0x0        0

		CMPT CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Address Translation                             0x0        0
			Over Flow Check Disable                         0x0        0
			Full Update                                     0x0        0
			Timer Running                                   0x0        0
			Trigger Pending                                 0x0        0
			Error                                           0x0        0
			Valid                                           0x1        1
			CIDX                                            0x0        0
			PIDX                                            0x0        0
			Descriptor Size                                 0x0        0
			Base Address (High)                             0x0        0
			Base Address (Low)                              0x2a870000 713490432
			Ring Size                                       0x0        0
			Color                                           0x1        1
			Interrupt State                                 0x1        1
			Timer Index                                     0x0        0
			Counter Index                                   0x0        0
			Function Id                                     0x0        0
			Trigger Mode                                    0x1        1
			Enable Interrupt                                0x1        1
			Enable Status Desc Update                       0x1        1

		PREFETCH CTXT:
			Valid                                           0x1        1
			Software Credit                                 0x5ff      1535
			In Prefetch                                     0x0        0
			Prefetch Enable                                 0x0        0
			Error                                           0x0        0
			Port Id                                         0x0        0
			Buffer Size Index                               0x0        0
			Bypass                                          0x0        0

		INTR CTXT:
			at                                              0x0        0
			pidx                                            0x0        0
			page_size                                       0x0        0
			baddr_4k (High)                                 0x0        0
			baddr_4k (Low)                                  0x2a801000 713035776
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	Dumped Queues 0 -> 0.


   
Dump Multiple Queue Information
-------------------------------

command: ``dma-ctl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>]``

Dumps the information for multiple queues

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to dump
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::



	[xilinx@]# dma-ctl qdma01000 q dump list 1 2 dir bi

	qdma01000-ST-1 H2C online
		hw_ID 1, thp ?, desc 0x000000006c71b76a/0x2a890000, 1536

		cmpl status: 0x00000000ad836574, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x2a890000 713621504
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x1        1
			IRQ Request                                     0x0        0
			Writeback Error Sent                            0x0        0
			Error                                           0x0        0
			Interrupt No Last                               0x0        0
			Port Id                                         0x0        0
			Interrupt Enable                                0x1        1
			Writeback Enable                                0x1        1
			MM Channel                                      0x0        0
			Bypass Enable                                   0x0        0
			Descriptor Size                                 0x1        1
			Ring Size                                       0x9        9
			Fetch Max                                       0x0        0
			Address Translation                             0x0        0
			Write back/Intr Interval                        0x1        1
			Write back/Intr Check                           0x1        1
			Fetch Credit Enable                             0x0        0
			Queue Enable                                    0x1        1
			Function Id                                     0x0        0
			IRQ Arm                                         0x1        1
			PIDX                                            0x0        0

		HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x0        0
			Credits Consumed                                0x0        0
			CIDX                                            0x0        0

		CREDIT CTXT:
			Credit                                          0x0        0

		INTR CTXT:
			at                                              0x0        0
			pidx                                            0x0        0
			page_size                                       0x0        0
			baddr_4k (High)                                 0x0        0
			baddr_4k (Low)                                  0x2a801000 713035776
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-1 C2H online
		hw_ID 1, thp ?, desc 0x000000007d2ed7f0/0x2a880000, 1536
		cmpt desc 0x000000005e745eb2/0x2a888000, 2048

		cmpl status: 0x000000008a822731, 00000000 00000000
		CMPT CMPL STATUS: 0x00000000b4ec91f7, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x2a880000 713555968
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x0        0
			IRQ Request                                     0x0        0
			Writeback Error Sent                            0x0        0
			Error                                           0x0        0
			Interrupt No Last                               0x0        0
			Port Id                                         0x0        0
			Interrupt Enable                                0x0        0
			Writeback Enable                                0x0        0
			MM Channel                                      0x0        0
			Bypass Enable                                   0x0        0
			Descriptor Size                                 0x0        0
			Ring Size                                       0x9        9
			Fetch Max                                       0x0        0
			Address Translation                             0x0        0
			Write back/Intr Interval                        0x1        1
			Write back/Intr Check                           0x0        0
			Fetch Credit Enable                             0x1        1
			Queue Enable                                    0x1        1
			Function Id                                     0x0        0
			IRQ Arm                                         0x0        0
			PIDX                                            0x5ff      1535

		HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x0        0
			Credits Consumed                                0x0        0
			CIDX                                            0x0        0

		CREDIT CTXT:
			Credit                                          0x0        0

		CMPT CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Address Translation                             0x0        0
			Over Flow Check Disable                         0x0        0
			Full Update                                     0x0        0
			Timer Running                                   0x0        0
			Trigger Pending                                 0x0        0
			Error                                           0x0        0
			Valid                                           0x1        1
			CIDX                                            0x0        0
			PIDX                                            0x0        0
			Descriptor Size                                 0x0        0
			Base Address (High)                             0x0        0
			Base Address (Low)                              0x2a888000 713588736
			Ring Size                                       0x0        0
			Color                                           0x1        1
			Interrupt State                                 0x1        1
			Timer Index                                     0x0        0
			Counter Index                                   0x0        0
			Function Id                                     0x0        0
			Trigger Mode                                    0x1        1
			Enable Interrupt                                0x1        1
			Enable Status Desc Update                       0x1        1

		PREFETCH CTXT:
			Valid                                           0x1        1
			Software Credit                                 0x5ff      1535
			In Prefetch                                     0x0        0
			Prefetch Enable                                 0x0        0
			Error                                           0x0        0
			Port Id                                         0x0        0
			Buffer Size Index                               0x0        0
			Bypass                                          0x0        0

		INTR CTXT:
			at                                              0x0        0
			pidx                                            0x0        0
			page_size                                       0x0        0
			baddr_4k (High)                                 0x0        0
			baddr_4k (Low)                                  0x2a801000 713035776
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-2 H2C online
		hw_ID 2, thp ?, desc 0x000000007167a03d/0x2a8a0000, 1536

		cmpl status: 0x000000000d8c788d, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x2a8a0000 713687040
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x1        1
			IRQ Request                                     0x0        0
			Writeback Error Sent                            0x0        0
			Error                                           0x0        0
			Interrupt No Last                               0x0        0
			Port Id                                         0x0        0
			Interrupt Enable                                0x1        1
			Writeback Enable                                0x1        1
			MM Channel                                      0x0        0
			Bypass Enable                                   0x0        0
			Descriptor Size                                 0x1        1
			Ring Size                                       0x9        9
			Fetch Max                                       0x0        0
			Address Translation                             0x0        0
			Write back/Intr Interval                        0x1        1
			Write back/Intr Check                           0x1        1
			Fetch Credit Enable                             0x0        0
			Queue Enable                                    0x1        1
			Function Id                                     0x0        0
			IRQ Arm                                         0x1        1
			PIDX                                            0x0        0

		HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x0        0
			Credits Consumed                                0x0        0
			CIDX                                            0x0        0

		CREDIT CTXT:
			Credit                                          0x0        0

		INTR CTXT:
			at                                              0x0        0
			pidx                                            0x0        0
			page_size                                       0x0        0
			baddr_4k (High)                                 0x0        0
			baddr_4k (Low)                                  0x2a801000 713035776
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-2 C2H online
		hw_ID 2, thp ?, desc 0x000000009c562437/0x2a884000, 1536
		cmpt desc 0x000000006c25b118/0x2a898000, 2048

		cmpl status: 0x0000000099bd702f, 00000000 00000000
		CMPT CMPL STATUS: 0x000000000e1bd43c, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x2a884000 713572352
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x0        0
			IRQ Request                                     0x0        0
			Writeback Error Sent                            0x0        0
			Error                                           0x0        0
			Interrupt No Last                               0x0        0
			Port Id                                         0x0        0
			Interrupt Enable                                0x0        0
			Writeback Enable                                0x0        0
			MM Channel                                      0x0        0
			Bypass Enable                                   0x0        0
			Descriptor Size                                 0x0        0
			Ring Size                                       0x9        9
			Fetch Max                                       0x0        0
			Address Translation                             0x0        0
			Write back/Intr Interval                        0x1        1
			Write back/Intr Check                           0x0        0
			Fetch Credit Enable                             0x1        1
			Queue Enable                                    0x1        1
			Function Id                                     0x0        0
			IRQ Arm                                         0x0        0
			PIDX                                            0x5ff      1535

		HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x0        0
			Credits Consumed                                0x0        0
			CIDX                                            0x0        0

		CREDIT CTXT:
			Credit                                          0x0        0

		CMPT CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Address Translation                             0x0        0
			Over Flow Check Disable                         0x0        0
			Full Update                                     0x0        0
			Timer Running                                   0x0        0
			Trigger Pending                                 0x0        0
			Error                                           0x0        0
			Valid                                           0x1        1
			CIDX                                            0x0        0
			PIDX                                            0x0        0
			Descriptor Size                                 0x0        0
			Base Address (High)                             0x0        0
			Base Address (Low)                              0x2a898000 713654272
			Ring Size                                       0x0        0
			Color                                           0x1        1
			Interrupt State                                 0x1        1
			Timer Index                                     0x0        0
			Counter Index                                   0x0        0
			Function Id                                     0x0        0
			Trigger Mode                                    0x1        1
			Enable Interrupt                                0x1        1
			Enable Status Desc Update                       0x1        1

		PREFETCH CTXT:
			Valid                                           0x1        1
			Software Credit                                 0x5ff      1535
			In Prefetch                                     0x0        0
			Prefetch Enable                                 0x0        0
			Error                                           0x0        0
			Port Id                                         0x0        0
			Buffer Size Index                               0x0        0
			Bypass                                          0x0        0

		INTR CTXT:
			at                                              0x0        0
			pidx                                            0x0        0
			page_size                                       0x0        0
			baddr_4k (High)                                 0x0        0
			baddr_4k (Low)                                  0x2a801000 713035776
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	Dumped Queues 1 -> 2.

   

   
Dump Queue Descriptor Information
---------------------------------

command: ``dma-ctl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>] [desc <x> <y>]``

Dump the queue descriptor information

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dma-ctl qdma17000 q dump idx 0 dir h2c desc 1 10

		qdma17000-ST-0 H2C online
	1: 0x00000000cd4d039c 00000000 00000000 00000000 00000000
	2: 0x000000009c6eaedf 00000000 00000000 00000000 00000000
	3: 0x00000000389a56a7 00000000 00000000 00000000 00000000
	4: 0x00000000b26186e9 00000000 00000000 00000000 00000000
	5: 0x0000000005f26fbf 00000000 00000000 00000000 00000000
	6: 0x00000000f887365d 00000000 00000000 00000000 00000000
	7: 0x000000003dbbdb52 00000000 00000000 00000000 00000000
	8: 0x000000007a05453d 00000000 00000000 00000000 00000000
	9: 0x00000000cc734198 00000000 00000000 00000000 00000000
	CMPL STATUS: 0x00000000377ea451 00000000 00000000
	Dumped descs of queues 0 -> 0.



   
Dump Multiple Queue Descriptor Information
------------------------------------------

command: ``dma-ctl qdma<bbddf> q dump list idx <N> [dir <h2c|c2h|bi>] [desc <x> <y>]``

Dumps the descriptor information for multiple queues

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to dump
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dma-ctl qdma17000 q dump list 1 2 dir h2c desc 1 10

	qdma17000-ST-1 H2C online
	1: 0x00000000c8f9ad60 00000000 00000000 00000000 00000000
	2: 0x000000007c3c25fb 00000000 00000000 00000000 00000000
	3: 0x000000000cc9e006 00000000 00000000 00000000 00000000
	4: 0x00000000087702c3 00000000 00000000 00000000 00000000
	5: 0x000000007619d32f 00000000 00000000 00000000 00000000
	6: 0x000000002bb74438 00000000 00000000 00000000 00000000
	7: 0x000000004f88c3b3 00000000 00000000 00000000 00000000
	8: 0x0000000047737864 00000000 00000000 00000000 00000000
	9: 0x000000004155d087 00000000 00000000 00000000 00000000
	CMPL STATUS: 0x00000000ad836574 00000000 00000000
	qdma17000-ST-2 H2C online
	1: 0x0000000045751dc6 00000000 00000000 00000000 00000000
	2: 0x00000000dd627761 00000000 00000000 00000000 00000000
	3: 0x000000007f1bccd8 00000000 00000000 00000000 00000000
	4: 0x0000000056d14ea0 00000000 00000000 00000000 00000000
	5: 0x0000000038a8a630 00000000 00000000 00000000 00000000
	6: 0x000000003c0b0083 00000000 00000000 00000000 00000000
	7: 0x000000004a9bfbca 00000000 00000000 00000000 00000000
	8: 0x000000003d056cdd 00000000 00000000 00000000 00000000
	9: 0x00000000108760bc 00000000 00000000 00000000 00000000
	CMPL STATUS: 0x000000000d8c788d 00000000 00000000
	Dumped descs of queues 1 -> 2.

Dump Queue Completion Information
---------------------------------

command: ``dma-ctl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>] [cmpt <x> <y>]``

Dump the queue completion information. This command is valid only for streaming c2h.

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dma-ctl qdma17000 q dump idx 0 dir c2h cmpt 0 10

	qdma17000-ST-0 C2H online
	0: 0x0000000086ae1f7a 00000000 00000000
	1: 0x0000000012caf472 00000000 00000000
	2: 0x00000000ce16f6c6 00000000 00000000
	3: 0x0000000030ac515a 00000000 00000000
	4: 0x0000000008a11d76 00000000 00000000
	5: 0x0000000006a465d9 00000000 00000000
	6: 0x00000000cb70afc1 00000000 00000000
	7: 0x00000000ccfe9e7c 00000000 00000000
	8: 0x00000000757eff01 00000000 00000000
	9: 0x000000007ad4593a 00000000 00000000
	CMPL STATUS: 0x00000000973734af 00000000 00000000
	Dumped descs of queues 0 -> 0.

Dump Multiple Queue Completion Information
------------------------------------------

command: ``dma-ctl qdma<bbddf> q dump list idx <N> [dir <h2c|c2h|bi>] [cmpt <x> <y>]``

Dumps the completion information for multiple queues. This command is valid only for streaming c2h.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to list
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dma-ctl qdma17000 q dump list 1 2 dir c2h cmpt 0 10

	qdma17000-ST-1 C2H online
	0: 0x000000005e745eb2 00000000 00000000
	1: 0x00000000c10487fb 00000000 00000000
	2: 0x000000002c76c0b0 00000000 00000000
	3: 0x000000005539c512 00000000 00000000
	4: 0x00000000bf8aeb9c 00000000 00000000
	5: 0x000000005ae055c9 00000000 00000000
	6: 0x00000000c8ec4586 00000000 00000000
	7: 0x000000009b833fb5 00000000 00000000
	8: 0x000000001e812b8c 00000000 00000000
	9: 0x0000000036bd937c 00000000 00000000
	CMPL STATUS: 0x00000000b4ec91f7 00000000 00000000
	qdma17000-ST-2 C2H online
	0: 0x000000006c25b118 00000000 00000000
	1: 0x000000004c4292df 00000000 00000000
	2: 0x00000000dd0e4974 00000000 00000000
	3: 0x00000000bf7da6cb 00000000 00000000
	4: 0x000000004e38eef2 00000000 00000000
	5: 0x00000000b5d7a04d 00000000 00000000
	6: 0x000000002758e395 00000000 00000000
	7: 0x00000000701f7830 00000000 00000000
	8: 0x000000000e71a726 00000000 00000000
	9: 0x00000000d45254b5 00000000 00000000
	CMPL STATUS: 0x000000000e1bd43c 00000000 00000000
	Dumped descs of queues 1 -> 2.

Dump the Interrupt Ring Information
-----------------------------------

command: ``dma-ctl qdma<bbddf> intring dump vector <N> <start_idx> <end_idx>``

Dump the interrupt ring information

**Parameters**

- <N> : Vector number
- <start_idx> : Range start index
- <end_idx> : Range end index

::

	[xilinx@]# dma-ctl qdma17000 intring dump vector 3 0 10

	intr_ring_entry = 0: 0x00000000 0x00000000
	intr_ring_entry = 1: 0x00000000 0x00000000
	intr_ring_entry = 2: 0x00000000 0x00000000
	intr_ring_entry = 3: 0x00000000 0x00000000
	intr_ring_entry = 4: 0x00000000 0x00000000
	intr_ring_entry = 5: 0x00000000 0x00000000
	intr_ring_entry = 6: 0x00000000 0x00000000
	intr_ring_entry = 7: 0x00000000 0x00000000
	intr_ring_entry = 8: 0x00000000 0x00000000
	intr_ring_entry = 9: 0x00000000 0x00000000
	intr_ring_entry = 10: 0x00000000 0x00000000


=================
Register Commands
=================

Read a Register
---------------

command: ``dma-ctl qdma<bbddf> reg read bar <N> <addr>``

Read a register value.

**Parameters**

- <N> : Bar number
- <addr> : Register address

::

	[xilinx@]# dma-ctl qdma17000 reg read bar 2 0x0
	qdma17000, 17:00.00, bar#2, 0x0 = 0x0.
	
Write a Register
----------------

command: ``dma-ctl qdma<bbddf> reg write bar <N> <addr>``

Write a value to into the given register.

**Parameters**

- <N> : Bar number
- <addr> : Register address

::

	[xilinx@]# dma-ctl qdma17000 reg write bar 2 0x0 0
	qdma17000, 17:00.00, bar#2, reg 0x0 -> 0x0, read back 0x0.
	
Dump the function registers
---------------------------

command: ``dma-ctl qdma<bbddf> reg dump``

This command allows the user to dump the DMA BAR and User BAR registers.

::

	[xilinx@]# dma-ctl qdma17000 reg dump
	################################################################################
	###		qdma17000, pci 17:00.00, reg dump
	################################################################################
	USER BAR #2
	[      0] ST_C2H_QID                                      0x1        1
	[    0x4] ST_C2H_PKTLEN                                   0x80       128
	[    0x8] ST_C2H_CONTROL                                  0          0
	[    0xc] ST_H2C_CONTROL                                  0          0
	[   0x10] ST_H2C_STATUS                                   0          0
	[   0x14] ST_H2C_XFER_CNT                                 0          0
	[   0x20] ST_C2H_PKT_CNT                                  0x1        1
	[   0x30] ST_C2H_CMPT_DATA_0                              0          0
	[   0x34] ST_C2H_CMPT_DATA_1                              0          0
	[   0x38] ST_C2H_CMPT_DATA_2                              0          0
	[   0x3c] ST_C2H_CMPT_DATA_3                              0          0
	[   0x40] ST_C2H_CMPT_DATA_4                              0          0
	[   0x44] ST_C2H_CMPT_DATA_5                              0          0
	[   0x48] ST_C2H_CMPT_DATA_6                              0          0
	[   0x4c] ST_C2H_CMPT_DATA_7                              0          0
	[   0x50] ST_C2H_CMPT_SIZE                                0          0
	[   0x60] ST_SCRATCH_REG_0                                0          0
	[   0x64] ST_SCRATCH_REG_1                                0          0
	[   0x88] ST_C2H_PKT_DROP                                 0          0
	[   0x8c] ST_C2H_PKT_ACCEPT                               0          0
	[   0x90] DSC_BYPASS_LOOP                                 0          0
	[   0x94] USER_INTERRUPT                                  0          0
	[   0x98] USER_INTERRUPT_MASK                             0          0
	[   0x9c] USER_INTERRUPT_VEC                              0          0
	[   0xa0] DMA_CONTROL                                     0          0
	[   0xa4] VDM_MSG_READ                                    0          0

	CONFIG BAR #0
	[    0x0] CFG_BLOCK_ID_0                                  0x1fd30000 533921792
	[    0x4] CFG_BUSDEV_0                                    0x0        0
	[    0x8] CFG_PCIE_MAX_PL_SZ_0                            0x51       81
	[    0xc] CFG_PCIE_MAX_RDRQ_SZ_0                          0x52       82
	[   0x10] CFG_SYS_ID_0                                    0x1234     4660
	[   0x14] CFG_MSI_EN_0                                    0x2020202  33686018
	[   0x18] CFG_PCIE_DATA_WIDTH_0                           0x3        3
	[   0x1c] CFG_PCIE_CTRL_0                                 0x1        1
	[   0x40] CFG_AXI_USR_MAX_PL_SZ_0                         0x55       85
	[   0x44] CFG_AXI_USR_MAX_RDRQ_SZ_0                       0x55       85
	[   0x4c] CFG_MISC_CTRL_0                                 0x10009    65545
	[   0x80] CFG_SCRATCH_REG_0                               0x0        0
	[   0x84] CFG_SCRATCH_REG_1                               0x0        0
	[   0x88] CFG_SCRATCH_REG_2                               0x0        0
	[   0x8c] CFG_SCRATCH_REG_3                               0x0        0
	[   0x90] CFG_SCRATCH_REG_4                               0x0        0
	[   0x94] CFG_SCRATCH_REG_5                               0x0        0
	[   0x98] CFG_SCRATCH_REG_6                               0x0        0
	[   0x9c] CFG_SCRATCH_REG_7                               0x0        0
	[   0xf0] QDMA_RAM_SBE_MSK_A_0                            0xffffff11 4294967057
	[   0xf4] QDMA_RAM_SBE_STS_A_0                            0x0        0
	[   0xf8] QDMA_RAM_DBE_MSK_A_0                            0xffffff11 4294967057
	[   0xfc] QDMA_RAM_DBE_STS_A_0                            0x0        0
	[  0x100] GLBL2_ID_0                                      0x1fd70000 534183936
	[  0x104] GLBL2_PF_BL_INT_0                               0x41041    266305
	[  0x108] GLBL2_PF_VF_BL_INT_0                            0x41041    266305
	[  0x10c] GLBL2_PF_BL_EXT_0                               0x104104   1065220
	[  0x110] GLBL2_PF_VF_BL_EXT_0                            0x104104   1065220
	[  0x114] GLBL2_CHNL_INST_0                               0x30101    196865
	[  0x118] GLBL2_CHNL_QDMA_0                               0x30f0f    200463
	[  0x11c] GLBL2_CHNL_STRM_0                               0x30000    196608
	[  0x120] GLBL2_QDMA_CAP_0                                0x800      2048
	[  0x128] GLBL2_PASID_CAP_0                               0x0        0
	[  0x12c] GLBL2_FUNC_RET_0                                0x0        0
	[  0x130] GLBL2_SYS_ID_0                                  0x0        0
	[  0x134] GLBL2_MISC_CAP_0                                0x1000003  16777219
	[  0x1b8] GLBL2_DBG_PCIE_RQ_0                             0x7f50003  133496835
	[  0x1bc] GLBL2_DBG_PCIE_RQ_1                             0x6024     24612
	[  0x1c0] GLBL2_DBG_AXIMM_WR_0                            0x600021   6291489
	[  0x1c4] GLBL2_DBG_AXIMM_WR_1                            0x0        0
	[  0x1c8] GLBL2_DBG_AXIMM_RD_0                            0x1        1
	[  0x1cc] GLBL2_DBG_AXIMM_RD_1                            0x0        0
	[  0x204] GLBL_RNGSZ_0                                    0x801      2049
	[  0x208] GLBL_RNGSZ_1                                    0x41       65
	[  0x20c] GLBL_RNGSZ_2                                    0x81       129
	[  0x210] GLBL_RNGSZ_3                                    0xc1       193
	[  0x214] GLBL_RNGSZ_4                                    0x101      257
	[  0x218] GLBL_RNGSZ_5                                    0x181      385
	[  0x21c] GLBL_RNGSZ_6                                    0x201      513
	[  0x220] GLBL_RNGSZ_7                                    0x301      769
	[  0x224] GLBL_RNGSZ_8                                    0x401      1025
	[  0x228] GLBL_RNGSZ_9                                    0x601      1537
	[  0x22c] GLBL_RNGSZ_10                                   0xc01      3073
	[  0x230] GLBL_RNGSZ_11                                   0x1001     4097
	[  0x234] GLBL_RNGSZ_12                                   0x1801     6145
	[  0x238] GLBL_RNGSZ_13                                   0x2001     8193
	[  0x23c] GLBL_RNGSZ_14                                   0x3001     12289
	[  0x240] GLBL_RNGSZ_15                                   0x4001     16385
	[  0x248] GLBL_ERR_STAT_0                                 0x0        0
	[  0x24c] GLBL_ERR_MASK_0                                 0x90f      2319
	[  0x250] GLBL_DSC_CFG_0                                  0x35       53
	[  0x254] GLBL_DSC_ERR_STS_0                              0x0        0
	[  0x258] GLBL_DSC_ERR_MSK_0                              0x1f9023f  33096255
	[  0x25c] GLBL_DSC_ERR_LOG_0                              0x0        0
	[  0x260] GLBL_DSC_ERR_LOG_1                              0x0        0
	[  0x264] GLBL_TRQ_ERR_STS_0                              0x0        0
	[  0x268] GLBL_TRQ_ERR_MSK_0                              0xf        15
	[  0x26c] GLBL_TRQ_ERR_LOG_0                              0x0        0
	[  0x270] GLBL_DSC_DBG_DAT_0                              0x0        0
	[  0x274] GLBL_DSC_DBG_DAT_1                              0x8080     32896
	[  0x27c] GLBL_DSC_ERR_LOG2_0                             0x0        0
	[  0x288] GLBL_INTERRUPT_CFG_0                            0x0        0
	[  0x400] TRQ_SEL_FMAP_0                                  0x0        0
	[  0x404] TRQ_SEL_FMAP_1                                  0x0        0
	[  0x408] TRQ_SEL_FMAP_2                                  0x0        0
	[  0x40c] TRQ_SEL_FMAP_3                                  0x0        0
	[  0x804] IND_CTXT_DATA_0                                 0x0        0
	[  0x808] IND_CTXT_DATA_1                                 0x0        0
	[  0x80c] IND_CTXT_DATA_2                                 0x0        0
	[  0x810] IND_CTXT_DATA_3                                 0x0        0
	[  0x814] IND_CTXT_DATA_4                                 0x0        0
	[  0x818] IND_CTXT_DATA_5                                 0x0        0
	[  0x81c] IND_CTXT_DATA_6                                 0x0        0
	[  0x820] IND_CTXT_DATA_7                                 0x0        0
	[  0x824] IND_CTXT_MASK_0                                 0xffffffff 4294967295
	[  0x828] IND_CTXT_MASK_1                                 0xffffffff 4294967295
	[  0x82c] IND_CTXT_MASK_2                                 0xffffffff 4294967295
	[  0x830] IND_CTXT_MASK_3                                 0xffffffff 4294967295
	[  0x834] IND_CTXT_MASK_4                                 0xffffffff 4294967295
	[  0x838] IND_CTXT_MASK_5                                 0xffffffff 4294967295
	[  0x83c] IND_CTXT_MASK_6                                 0xffffffff 4294967295
	[  0x840] IND_CTXT_MASK_7                                 0xffffffff 4294967295
	[  0x844] IND_CTXT_CMD_0                                  0x238      568
	[  0xa00] C2H_TIMER_CNT_0                                 0x1        1
	[  0xa04] C2H_TIMER_CNT_1                                 0x2        2
	[  0xa08] C2H_TIMER_CNT_2                                 0x4        4
	[  0xa0c] C2H_TIMER_CNT_3                                 0x5        5
	[  0xa10] C2H_TIMER_CNT_4                                 0x8        8
	[  0xa14] C2H_TIMER_CNT_5                                 0xa        10
	[  0xa18] C2H_TIMER_CNT_6                                 0xf        15
	[  0xa1c] C2H_TIMER_CNT_7                                 0x14       20
	[  0xa20] C2H_TIMER_CNT_8                                 0x19       25
	[  0xa24] C2H_TIMER_CNT_9                                 0x1e       30
	[  0xa28] C2H_TIMER_CNT_10                                0x32       50
	[  0xa2c] C2H_TIMER_CNT_11                                0x4b       75
	[  0xa30] C2H_TIMER_CNT_12                                0x64       100
	[  0xa34] C2H_TIMER_CNT_13                                0x7d       125
	[  0xa38] C2H_TIMER_CNT_14                                0x96       150
	[  0xa3c] C2H_TIMER_CNT_15                                0xc8       200
	[  0xa40] C2H_CNT_THRESH_0                                0x40       64
	[  0xa44] C2H_CNT_THRESH_1                                0x2        2
	[  0xa48] C2H_CNT_THRESH_2                                0x4        4
	[  0xa4c] C2H_CNT_THRESH_3                                0x8        8
	[  0xa50] C2H_CNT_THRESH_4                                0x10       16
	[  0xa54] C2H_CNT_THRESH_5                                0x18       24
	[  0xa58] C2H_CNT_THRESH_6                                0x20       32
	[  0xa5c] C2H_CNT_THRESH_7                                0x30       48
	[  0xa60] C2H_CNT_THRESH_8                                0x50       80
	[  0xa64] C2H_CNT_THRESH_9                                0x60       96
	[  0xa68] C2H_CNT_THRESH_10                               0x70       112
	[  0xa6c] C2H_CNT_THRESH_11                               0x80       128
	[  0xa70] C2H_CNT_THRESH_12                               0x90       144
	[  0xa74] C2H_CNT_THRESH_13                               0xa0       160
	[  0xa78] C2H_CNT_THRESH_14                               0xb0       176
	[  0xa7c] C2H_CNT_THRESH_15                               0xc0       192
	[  0xa88] C2H_STAT_S_AXIS_C2H_ACCEPTED_0                  0x0        0
	[  0xa8c] C2H_STAT_S_AXIS_CMPT_ACCEPTED_0                 0x0        0
	[  0xa90] C2H_STAT_DESC_RSP_PKT_ACCEPTED_0                0x0        0
	[  0xa94] C2H_STAT_AXIS_PKG_CMP_0                         0x0        0
	[  0xa98] C2H_STAT_DESC_RSP_ACCEPTED_0                    0x0        0
	[  0xa9c] C2H_STAT_DESC_RSP_CMP_0                         0x0        0
	[  0xaa0] C2H_STAT_WRQ_OUT_0                              0x0        0
	[  0xaa4] C2H_STAT_WPL_REN_ACCEPTED_0                     0x0        0
	[  0xaa8] C2H_STAT_TOTAL_WRQ_LEN_0                        0x0        0
	[  0xaac] C2H_STAT_TOTAL_WPL_LEN_0                        0x0        0
	[  0xab0] C2H_BUF_SZ_0                                    0x1000     4096
	[  0xab4] C2H_BUF_SZ_1                                    0x100      256
	[  0xab8] C2H_BUF_SZ_2                                    0x200      512
	[  0xabc] C2H_BUF_SZ_3                                    0x400      1024
	[  0xac0] C2H_BUF_SZ_4                                    0x800      2048
	[  0xac4] C2H_BUF_SZ_5                                    0xf80      3968
	[  0xac8] C2H_BUF_SZ_6                                    0x1000     4096
	[  0xacc] C2H_BUF_SZ_7                                    0x1000     4096
	[  0xad0] C2H_BUF_SZ_8                                    0x1000     4096
	[  0xad4] C2H_BUF_SZ_9                                    0x1000     4096
	[  0xad8] C2H_BUF_SZ_10                                   0x1000     4096
	[  0xadc] C2H_BUF_SZ_11                                   0x1000     4096
	[  0xae0] C2H_BUF_SZ_12                                   0x1000     4096
	[  0xae4] C2H_BUF_SZ_13                                   0x2000     8192
	[  0xae8] C2H_BUF_SZ_14                                   0x233a     9018
	[  0xaec] C2H_BUF_SZ_15                                   0x4000     16384
	[  0xaf0] C2H_ERR_STAT_0                                  0x0        0
	[  0xaf4] C2H_ERR_MASK_0                                  0xfedb     65243
	[  0xaf8] C2H_FATAL_ERR_STAT_0                            0x0        0
	[  0xafc] C2H_FATAL_ERR_MASK_0                            0x7df1b    515867
	[  0xb00] C2H_FATAL_ERR_ENABLE_0                          0x0        0
	[  0xb04] GLBL_ERR_INT_0                                  0x1002000  16785408
	[  0xb08] C2H_PFCH_CFG_0                                  0xc201100  203428096
	[  0xb0c] C2H_INT_TIMER_TICK_0                            0x19       25
	[  0xb10] C2H_STAT_DESC_RSP_DROP_ACCEPTED_0               0x0        0
	[  0xb14] C2H_STAT_DESC_RSP_ERR_ACCEPTED_0                0x0        0
	[  0xb18] C2H_STAT_DESC_REQ_0                             0x0        0
	[  0xb1c] C2H_STAT_DEBUG_DMA_ENG_0                        0x0        0
	[  0xb20] C2H_STAT_DEBUG_DMA_ENG_1                        0x80000000 2147483648
	[  0xb24] C2H_STAT_DEBUG_DMA_ENG_2                        0xc0000000 3221225472
	[  0xb28] C2H_STAT_DEBUG_DMA_ENG_3                        0x100000   1048576
	[  0xb2c] C2H_DBG_PFCH_ERR_CTXT_0                         0x0        0
	[  0xb30] C2H_FIRST_ERR_QID_0                             0x0        0
	[  0xb34] STAT_NUM_CMPT_IN_0                              0x0        0
	[  0xb38] STAT_NUM_CMPT_OUT_0                             0x0        0
	[  0xb3c] STAT_NUM_CMPT_DRP_0                             0x0        0
	[  0xb40] STAT_NUM_STAT_DESC_OUT_0                        0x0        0
	[  0xb44] STAT_NUM_DSC_CRDT_SENT_0                        0x0        0
	[  0xb48] STAT_NUM_FCH_DSC_RCVD_0                         0x0        0
	[  0xb4c] STAT_NUM_BYP_DSC_RCVD_0                         0x0        0
	[  0xb50] C2H_CMPT_COAL_CFG_0                             0x40064014 1074151444
	[  0xb54] C2H_INTR_H2C_REQ_0                              0x0        0
	[  0xb58] C2H_INTR_C2H_MM_REQ_0                           0x0        0
	[  0xb5c] C2H_INTR_ERR_INT_REQ_0                          0x0        0
	[  0xb60] C2H_INTR_C2H_ST_REQ_0                           0x0        0
	[  0xb64] C2H_INTR_H2C_ERR_MM_MSIX_ACK_0                  0x0        0
	[  0xb68] C2H_INTR_H2C_ERR_MM_MSIX_FAIL_0                 0x0        0
	[  0xb6c] C2H_INTR_H2C_ERR_MM_NO_MSIX_0                   0x0        0
	[  0xb70] C2H_INTR_H2C_ERR_MM_CTXT_INVAL_0                0x0        0
	[  0xb74] C2H_INTR_C2H_ST_MSIX_ACK_0                      0x0        0
	[  0xb78] C2H_INTR_C2H_ST_MSIX_FAIL_0                     0x0        0
	[  0xb7c] C2H_INTR_C2H_ST_NO_MSIX_0                       0x0        0
	[  0xb80] C2H_INTR_C2H_ST_CTXT_INVAL_0                    0x0        0
	[  0xb84] C2H_STAT_WR_CMP_0                               0x0        0
	[  0xb88] C2H_STAT_DEBUG_DMA_ENG_4_0                      0x40000000 1073741824
	[  0xb8c] C2H_STAT_DEBUG_DMA_ENG_5_0                      0x0        0
	[  0xb90] C2H_DBG_PFCH_QID_0                              0x0        0
	[  0xb94] C2H_DBG_PFCH_0                                  0x0        0
	[  0xb98] C2H_INT_DEBUG_0                                 0x0        0
	[  0xb9c] C2H_STAT_IMM_ACCEPTED_0                         0x0        0
	[  0xba0] C2H_STAT_MARKER_ACCEPTED_0                      0x0        0
	[  0xba4] C2H_STAT_DISABLE_CMP_ACCEPTED_0                 0x0        0
	[  0xba8] C2H_C2H_PAYLOAD_FIFO_CRDT_CNT_0                 0x0        0
	[  0xbac] C2H_INTR_DYN_REQ_0                              0x0        0
	[  0xbb0] C2H_INTR_DYN_MSIX_0                             0x0        0
	[  0xbb4] C2H_DROP_LEN_MISMATCH_0                         0x0        0
	[  0xbb8] C2H_DROP_DESC_RSP_LEN_0                         0x0        0
	[  0xbbc] C2H_DROP_QID_FIFO_LEN_0                         0x0        0
	[  0xbc0] C2H_DROP_PAYLOAD_CNT_0                          0x0        0
	[  0xbc4] QDMA_C2H_CMPT_FORMAT_0                          0x20001    131073
	[  0xbc8] QDMA_C2H_CMPT_FORMAT_1                          0x0        0
	[  0xbcc] QDMA_C2H_CMPT_FORMAT_2                          0x0        0
	[  0xbd0] QDMA_C2H_CMPT_FORMAT_3                          0x0        0
	[  0xbd4] QDMA_C2H_CMPT_FORMAT_4                          0x0        0
	[  0xbd8] QDMA_C2H_CMPT_FORMAT_5                          0x0        0
	[  0xbdc] QDMA_C2H_CMPT_FORMAT_6                          0x0        0
	[  0xbe0] C2H_PFCH_CACHE_DEPTH_0                          0x10       16
	[  0xbe4] C2H_CMPT_COAL_BUF_DEPTH_0                       0x10       16
	[  0xbe8] C2H_PFCH_CRDT_0                                 0x0        0
	[  0xe00] H2C_ERR_STAT_0                                  0x0        0
	[  0xe04] H2C_ERR_MASK_0                                  0x1f       31
	[  0xe08] H2C_FIRST_ERR_QID_0                             0x0        0
	[  0xe0c] H2C_DBG_REG_0                                   0x0        0
	[  0xe10] H2C_DBG_REG_1                                   0x0        0
	[  0xe14] H2C_DBG_REG_2                                   0x0        0
	[  0xe18] H2C_DBG_REG_3                                   0x44008025 1140883493
	[  0xe1c] H2C_DBG_REG_4                                   0x0        0
	[  0xe20] H2C_FATAL_ERR_EN_0                              0x0        0
	[  0xe24] H2C_REQ_THROT_0                                 0xc14000   12664832
	[  0xe28] H2C_ALN_DBG_REG0_0                              0x0        0
	[ 0x1004] C2H_MM_CONTROL_0                                0x1        1
	[ 0x1008] C2H_MM_CONTROL_1                                0x1        1
	[ 0x100c] C2H_MM_CONTROL_2                                0x1        1
	[ 0x1040] C2H_MM_STATUS_0                                 0x1        1
	[ 0x1044] C2H_MM_STATUS_1                                 0x1        1
	[ 0x1048] C2H_MM_CMPL_DSC_CNT_0                           0x0        0
	[ 0x1054] C2H_MM_ERR_CODE_EN_MASK_0                       0x0        0
	[ 0x1058] C2H_MM_ERR_CODE_0                               0x0        0
	[ 0x105c] C2H_MM_ERR_INFO_0                               0x0        0
	[ 0x10c0] C2H_MM_PERF_MON_CTRL_0                          0x0        0
	[ 0x10c4] C2H_MM_PERF_MON_CY_CNT_0                        0x0        0
	[ 0x10c8] C2H_MM_PERF_MON_CY_CNT_1                        0x0        0
	[ 0x10cc] C2H_MM_PERF_MON_DATA_CNT_0                      0x0        0
	[ 0x10d0] C2H_MM_PERF_MON_DATA_CNT_1                      0x0        0
	[ 0x10e8] C2H_MM_DBG_INFO_0                               0x10002    65538
	[ 0x10ec] C2H_MM_DBG_INFO_1                               0x0        0
	[ 0x1204] H2C_MM_CONTROL_0                                0x1        1
	[ 0x1208] H2C_MM_CONTROL_1                                0x1        1
	[ 0x120c] H2C_MM_CONTROL_2                                0x1        1
	[ 0x1240] H2C_MM_STATUS_0                                 0x1        1
	[ 0x1248] H2C_MM_CMPL_DSC_CNT_0                           0x0        0
	[ 0x1254] H2C_MM_ERR_CODE_EN_MASK_0                       0x0        0
	[ 0x1258] H2C_MM_ERR_CODE_0                               0x0        0
	[ 0x125c] H2C_MM_ERR_INFO_0                               0x0        0
	[ 0x12c0] H2C_MM_PERF_MON_CTRL_0                          0x0        0
	[ 0x12c4] H2C_MM_PERF_MON_CY_CNT_0                        0x0        0
	[ 0x12c8] H2C_MM_PERF_MON_CY_CNT_1                        0x0        0
	[ 0x12cc] H2C_MM_PERF_MON_DATA_CNT_0                      0x0        0
	[ 0x12d0] H2C_MM_PERF_MON_DATA_CNT_1                      0x0        0
	[ 0x12e8] H2C_MM_DBG_INFO_0                               0x10002    65538
	[ 0x12ec] H2C_MM_REQ_THROT_0                              0x8000     32768
	[ 0x2400] FUNC_STATUS_0                                   0x44       68
	[ 0x2404] FUNC_CMD_0                                      0xffffffff 4294967295
	[ 0x2408] FUNC_INTR_VEC_0                                 0x0        0
	[ 0x240c] TARGET_FUNC_0                                   0x4        4
	[ 0x2410] INTR_CTRL_0                                     0x1        1
	[ 0x2420] PF_ACK_0                                        0x10       16
	[ 0x2424] PF_ACK_1                                        0x0        0
	[ 0x2428] PF_ACK_2                                        0x0        0
	[ 0x242c] PF_ACK_3                                        0x0        0
	[ 0x2430] PF_ACK_4                                        0x0        0
	[ 0x2434] PF_ACK_5                                        0x0        0
	[ 0x2438] PF_ACK_6                                        0x0        0
	[ 0x243c] PF_ACK_7                                        0x0        0
	[ 0x2500] FLR_CTRL_STATUS_0                               0x0        0
	[ 0x2800] MSG_IN_0                                        0xffffffff 4294967295
	[ 0x2804] MSG_IN_1                                        0xffffffff 4294967295
	[ 0x2808] MSG_IN_2                                        0xffffffff 4294967295
	[ 0x280c] MSG_IN_3                                        0xffffffff 4294967295
	[ 0x2810] MSG_IN_4                                        0xffffffff 4294967295
	[ 0x2814] MSG_IN_5                                        0xffffffff 4294967295
	[ 0x2818] MSG_IN_6                                        0xffffffff 4294967295
	[ 0x281c] MSG_IN_7                                        0xffffffff 4294967295
	[ 0x2820] MSG_IN_8                                        0xffffffff 4294967295
	[ 0x2824] MSG_IN_9                                        0xffffffff 4294967295
	[ 0x2828] MSG_IN_10                                       0xffffffff 4294967295
	[ 0x282c] MSG_IN_11                                       0xffffffff 4294967295
	[ 0x2830] MSG_IN_12                                       0xffffffff 4294967295
	[ 0x2834] MSG_IN_13                                       0xffffffff 4294967295
	[ 0x2838] MSG_IN_14                                       0xffffffff 4294967295
	[ 0x283c] MSG_IN_15                                       0xffffffff 4294967295
	[ 0x2840] MSG_IN_16                                       0xffffffff 4294967295
	[ 0x2844] MSG_IN_17                                       0xffffffff 4294967295
	[ 0x2848] MSG_IN_18                                       0xffffffff 4294967295
	[ 0x284c] MSG_IN_19                                       0xffffffff 4294967295
	[ 0x2850] MSG_IN_20                                       0xffffffff 4294967295
	[ 0x2854] MSG_IN_21                                       0xffffffff 4294967295
	[ 0x2858] MSG_IN_22                                       0xffffffff 4294967295
	[ 0x285c] MSG_IN_23                                       0xffffffff 4294967295
	[ 0x2860] MSG_IN_24                                       0xffffffff 4294967295
	[ 0x2864] MSG_IN_25                                       0xffffffff 4294967295
	[ 0x2868] MSG_IN_26                                       0xffffffff 4294967295
	[ 0x286c] MSG_IN_27                                       0xffffffff 4294967295
	[ 0x2870] MSG_IN_28                                       0xffffffff 4294967295
	[ 0x2874] MSG_IN_29                                       0xffffffff 4294967295
	[ 0x2878] MSG_IN_30                                       0xffffffff 4294967295
	[ 0x287c] MSG_IN_31                                       0xffffffff 4294967295
	[ 0x2c00] MSG_OUT_0                                       0x0        0
	[ 0x2c04] MSG_OUT_1                                       0x0        0
	[ 0x2c08] MSG_OUT_2                                       0x0        0
	[ 0x2c0c] MSG_OUT_3                                       0x0        0
	[ 0x2c10] MSG_OUT_4                                       0x0        0
	[ 0x2c14] MSG_OUT_5                                       0x0        0
	[ 0x2c18] MSG_OUT_6                                       0x0        0
	[ 0x2c1c] MSG_OUT_7                                       0x0        0
	[ 0x2c20] MSG_OUT_8                                       0x0        0
	[ 0x2c24] MSG_OUT_9                                       0x0        0
	[ 0x2c28] MSG_OUT_10                                      0x0        0
	[ 0x2c2c] MSG_OUT_11                                      0x0        0
	[ 0x2c30] MSG_OUT_12                                      0x0        0
	[ 0x2c34] MSG_OUT_13                                      0x0        0
	[ 0x2c38] MSG_OUT_14                                      0x0        0
	[ 0x2c3c] MSG_OUT_15                                      0x0        0
	[ 0x2c40] MSG_OUT_16                                      0x0        0
	[ 0x2c44] MSG_OUT_17                                      0x0        0
	[ 0x2c48] MSG_OUT_18                                      0x0        0
	[ 0x2c4c] MSG_OUT_19                                      0x0        0
	[ 0x2c50] MSG_OUT_20                                      0x0        0
	[ 0x2c54] MSG_OUT_21                                      0x0        0
	[ 0x2c58] MSG_OUT_22                                      0x0        0
	[ 0x2c5c] MSG_OUT_23                                      0x0        0
	[ 0x2c60] MSG_OUT_24                                      0x0        0
	[ 0x2c64] MSG_OUT_25                                      0x0        0
	[ 0x2c68] MSG_OUT_26                                      0x0        0
	[ 0x2c6c] MSG_OUT_27                                      0x0        0
	[ 0x2c70] MSG_OUT_28                                      0x0        0
	[ 0x2c74] MSG_OUT_29                                      0x0        0
	[ 0x2c78] MSG_OUT_30                                      0x0        0
	[ 0x2c7c] MSG_OUT_31                                      0x0        0


	
Dump detailed register info
---------------------------

command: ``dma-ctl qdma<BDF> reg info bar <N> <addr> [num_regs <M>]``

For QDMA4.0 designs there is an additional option to print detailed breakdown of 
individual registers

::

        [xilinx@]# dma-ctl qdma06000 reg info bar 0 0x00 num_regs 5
        CFG_BLK_IDENTIFIER                       0x0       0x1fd30001 533921793
        CFG_BLK_IDENTIFIER                       [31,20]   0x1fd
        CFG_BLK_IDENTIFIER_1                     [19,16]   0x3
        CFG_BLK_IDENTIFIER_RSVD_1                [15, 8]   0x0
        CFG_BLK_IDENTIFIER_VERSION               [ 7, 0]   0x1

        CFG_BLK_PCIE_MAX_PLD_SIZE                0x8       0x51       81
        CFG_BLK_IDENTIFIER                       [31, 7]   0x0
        CFG_BLK_IDENTIFIER_1                     [ 6, 4]   0x5
        CFG_BLK_IDENTIFIER_RSVD_1                [    3]   0x0
        CFG_BLK_IDENTIFIER_VERSION               [ 2, 0]   0x1

        CFG_BLK_PCIE_MAX_READ_REQ_SIZE           0xc       0x52       82
        CFG_BLK_IDENTIFIER                       [31, 7]   0x0
        CFG_BLK_IDENTIFIER_1                     [ 6, 4]   0x5
        CFG_BLK_IDENTIFIER_RSVD_1                [    3]   0x0
        CFG_BLK_IDENTIFIER_VERSION               [ 2, 0]   0x2
        
        CFG_BLK_SYSTEM_ID                        0x10      0xff01     65281
        CFG_BLK_IDENTIFIER                       [31,17]   0x0
        CFG_BLK_IDENTIFIER_1                     [   16]   0x0
        CFG_BLK_IDENTIFIER_RSVD_1                [15, 0]   0xff01
        
        CFG_BLK_MSIX_ENABLE                      0x14      0xf        15
        CFG_BLK_IDENTIFIER                       [31, 0]   0xf

