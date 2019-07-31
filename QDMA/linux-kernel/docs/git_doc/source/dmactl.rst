********************************
DMA Control Application (dmactl)
********************************

QDMA driver comes with a command-line configuration utility called ``dmactl`` to manage the driver.

The Xilinx QDMA control tool, ``dmactl`` is a command Line utility which is installed in ``/usr/local/sbin/`` and allows administration of the Xilinx QDMA queues. Make sure that the installation path ``/usr/local/sbin/`` is added to the ``PATH`` variable.

It can perform the following functions:

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

For dmactl help run ``dmactl –h``

For more details on the dmactl tool commands and options for each command, refer to dmactl man page.

For dmactl man page, run ``man dmactl``

==========================
Device Management Commands
==========================

List devices
------------

command: ``dmactl dev list``

This command lists all the physical and virtual functions available in the system along with the number of queues assigned to each function, queue base and queue max.
Each qdma device name is listed as ``qdma<bbddf>`` where ``<bbddf>`` is bus number, device number and function number.

Initially when the system is started, the queues are not assigned to any of the available functions.

::


	[xilinx@]# dmactl dev list

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
	[xilinx@]# dmactl dev list

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

command: ``dmactl qdma<bbddf> version``

This command lists hardware and software version details and device capabilities.
::

	[xilinx@]# dmactl qdma01000 devinfo
	=============Hardware Version============

	RTL Version         : RTL Base
	Vivado ReleaseID    : vivado 2019.1
	Everest IP          : Soft IP

	============Software Version============

	qdma driver version : 2019.1.121.205.

	=============Hardware Capabilities============

	Number of PFs supported                : 4
	Total number of queues supported       : 2048
	MM channels                            : 1
	FLR Present                            : yes
	ST enabled                             : yes
	MM enabled                             : yes
	Mailbox enabled                        : yes
	MM completion enabled                  : no



List Device Statistics
-----------------------

command: ``dmactl qdma<bbddf> stat``

This command lists the statistics accumulated for this device
::

   [xilinx@]# dmactl qdma01000 stat

   qdma01000:statistics
   Total MM H2C packets processed = 312220
   Total MM C2H packets processed = 312220
   Total ST H2C packets processed = 64127
   Total ST C2H packets processed = 100954

Use ``dmactl qdma01000 stat clear`` to clear the statistics collected

Add a Queue
-----------

command: ``dmactl qdma<bbddf> q add idx <N> [mode <st|mm>] [dir <h2c|c2h|bi>]``

This command allows the user to add a queue.

**Parameters**

- <N> : Queue number
- mode : mode of the queue, streaming\(st\) or memory mapped\(mm\). Mode defaults to mm.
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Direction defaults to h2c.

::

   [xilinx@]# dmactl qdma01000 q add idx 4 mode mm dir h2c

   qdma01000-MM-4 H2C added.
   Added 1 Queues.

Add a List of Queues
--------------------

command: ``dmactl qdma<bbddf> q add list <start_idx> <N>  [ mode <st|mm> ] [ dir <h2c|c2h|bi> ]``

This command allows the user to add a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> :Number of queues to add
- mode : mode of the queue, streaming\(st\) or memory mapped\(mm\)
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::

   [xilinx@]# dmactl qdma01000 q add list 1 4 mode mm dir h2c

   qdma01000-MM-1 H2C added.
   qdma01000-MM-2 H2C added.
   qdma01000-MM-3 H2C added.
   qdma01000-MM-4 H2C added.
   Added 4 Queues.
   
Start a Queue
-------------
This command allows the user to start a queue.

command: 

   ``dmactl qdma<bbddf> q start idx <N> [dir <h2c|c2h|bi>]  [en_mm_cmpl] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>] \
   [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en]\
   [dis_cmpl_status] [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en] [dis_fetch_credit] [dis_cmpt_stat] [c2h_cmpl_intr_en] \ [cmpl_ovf_dis]``


**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card \(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.
- en_mm_cmpl : Enable MM completions. This is valid only for MM Completion example design.
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
- dis_fetch_credit: Disable fetch credit \( 1: Disable, 0: Enable \)
- dis_cmpt_stat : Disable completion status \( 1: Disable, 0: Enable \)
- c2h_cmpl_intr_en : Enable c2h completion interval \( 0: Disable, 1: Enable \)
- cmpl_ovf_dis : Disable completion over flow check \( 1: Disable, 0: Enable \)

::

   [xilinx@]# dmactl qdma01000 q start idx 4 dir h2c
   dmactl: Info: Default ring size set to 2048

   1 Queues started, idx 4 ~ 4.

Start a List of Queues
----------------------

command:

   ``dmactl qdma<bbddf> q start list <start_idx> <N> [dir <h2c|c2h|bi>]  [en_mm_cmpl] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>] \
   [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en]\
   [dis_cmpl_status] [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en] [dis_fetch_credit] [dis_cmpt_stat] [c2h_cmpl_intr_en] \ [cmpl_ovf_dis]``

This command allows the user to start a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to start
- dir : direction of the queue, host-to-card \(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.
- en_mm_cmpl : Enable MM completions. This is valid only for MM Completion example design.
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
- dis_fetch_credit: Disable fetch credit \( 1: Disable, 0: Enable \)
- dis_cmpt_stat : Disable completion status \( 1: Disable, 0: Enable \)
- c2h_cmpl_intr_en : Enable c2h completion interval \( 0: Disable, 1: Enable \)
- cmpl_ovf_dis : Disable completion over flow check \( 1: Disable, 0: Enable \)

::

   [xilinx@]# dmactl qdma01000 q start list 1 4 dir h2c

   Started Queues 1 -> 4.
   
Stop a Queue
------------

command: ``dmactl qdma<bbddf> q stop idx <N> [dir <h2c|c2h|bi>]``

This command allows the user to stop a queue.

**Parameters**

- <N> : Queue number
- dir : direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dmactl qdma01000 q stop idx 4 dir h2c
   dmactl: Info: Default ring size set to 2048

   Stopped Queues 4 -> 4.
   
Stop a List of Queues
---------------------

command: ``dmactl qdma<bbddf> q stop list <start_idx> <N> [dir <h2c|c2h|bi>]``

This command allows the user to stop a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to delete
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dmactl qdma01000 q stop list 1 4 dir h2c

   Stopped Queues 1 -> 4.

Delete a Queue
--------------

command: ``dmactl qdma<bbddf> q del idx <N> [dir <h2c|c2h|bi>]``

This command allows the user to delete a queue.

**Parameters**

- <N> : Queue number
- dir : direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dmactl qdma01000 q del idx 4 mode mm dir h2c

   Deleted Queues 4 -> 4.
   
Delete a List of Queues
-----------------------

command: ``dmactl <bbddf> q del list <start_idx> <N> [ dir <h2c|c2h|bi> ]``

This command allows the user to delete a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to delete
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::

   [xilinx@]# dmactl qdma01000 q del list 1 4 dir h2c

   Deleted Queues 1 -> 4.
   
Dump Queue Information
----------------------

command: ``dmactl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>]``

Dump the queue information

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

Sample output is given below:

::


	[xilinx@]# dmactl qdma01000 q dump idx 1 dir bi

	qdma01000-ST-1 C2H online
		hw_ID 1, thp ?, desc 0xffff880084140000/0x84140000, 1536
		cmpt desc 0xffff8800842a0000/0x842a0000, 2048

		cmpl status: 0xffff880084143000, 00000000 00000000
		CMPT CMPL STATUS: 0xffff8800842a4000, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x84140000 2215903232
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
			Base Address (Low)                              0x842a0000 2217345024
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
			baddr_4k (Low)                                  0x8414c000 2215952384
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-1 H2C online
		hw_ID 1, thp ?, desc 0xffff880084288000/0x84288000, 1536

		cmpl status: 0xffff88008428e000, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x84288000 2217246720
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x0        0
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
			baddr_4k (Low)                                  0x8414c000 2215952384
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	Dumped Queues 1 -> 1.


   
Dump Multiple Queue Information
-------------------------------

command: ``dmactl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>]``

Dumps the information for multiple queues

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to dump
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::



	[xilinx@]# dmactl qdma01000 q dump list 1 2 dir bi

	qdma01000-ST-1 C2H online
		hw_ID 1, thp ?, desc 0xffff880084140000/0x84140000, 1536
		cmpt desc 0xffff8800842a0000/0x842a0000, 2048

		cmpl status: 0xffff880084143000, 00000000 00000000
		CMPT CMPL STATUS: 0xffff8800842a4000, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x84140000 2215903232
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
			Base Address (Low)                              0x842a0000 2217345024
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
			baddr_4k (Low)                                  0x8414c000 2215952384
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-1 H2C online
		hw_ID 1, thp ?, desc 0xffff880084288000/0x84288000, 1536

		cmpl status: 0xffff88008428e000, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x84288000 2217246720
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x0        0
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
			baddr_4k (Low)                                  0x8414c000 2215952384
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-2 C2H online
		hw_ID 2, thp ?, desc 0xffff880084274000/0x84274000, 1536
		cmpt desc 0xffff880084398000/0x84398000, 2048

		cmpl status: 0xffff880084277000, 00000000 00000000
		CMPT CMPL STATUS: 0xffff88008439c000, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x84274000 2217164800
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
			Base Address (Low)                              0x84398000 2218360832
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
			baddr_4k (Low)                                  0x8414c000 2215952384
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	qdma01000-ST-2 H2C online
		hw_ID 2, thp ?, desc 0xffff8800843a0000/0x843a0000, 1536

		cmpl status: 0xffff8800843a6000, 00000000 00000000
		SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x843a0000 2218393600
			Is Memory Mapped                                0x0        0
			Marker Disable                                  0x0        0
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
			baddr_4k (Low)                                  0x8414c000 2215952384
			color                                           0x1        1
			int_st                                          0x0        0
			vec                                             0x3        3
			valid                                           0x1        1

		total descriptor processed:    0
	Dumped Queues 1 -> 2.


   

   
Dump Queue Descriptor Information
---------------------------------

command: ``dmactl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>] [desc <x> <y>]``

Dump the queue descriptor information

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dmactl qdma17000 q dump idx 1 dir h2c desc 1 10

	qdma17000-MM-1 H2C online
	1: 0x0000000075e985a1 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	2: 0x000000009fa51b7d 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	3: 0x0000000088024b26 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	4: 0x0000000003e7e32a 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	5: 0x0000000017908b59 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	6: 0x000000006010e5f5 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	7: 0x00000000ea16b7aa 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	8: 0x00000000f49eab9e 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	9: 0x000000005867272e 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	CMPL STATUS: 0x000000005a1efda1 00000000 00000000
	Dumped descs of queues 1 -> 1.


   
Dump Multiple Queue Descriptor Information
------------------------------------------

command: ``dmactl qdma<bbddf> q dump list idx <N> [dir <h2c|c2h|bi>] [desc <x> <y>]``

Dumps the descriptor information for multiple queues

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to dump
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dmactl qdma17000 q dump list 1 2 dir h2c desc 1 10

	qdma17000-MM-1 H2C online
	1: 0x0000000075e985a1 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	2: 0x000000009fa51b7d 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	3: 0x0000000088024b26 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	4: 0x0000000003e7e32a 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	5: 0x0000000017908b59 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	6: 0x000000006010e5f5 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	7: 0x00000000ea16b7aa 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	8: 0x00000000f49eab9e 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	9: 0x000000005867272e 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	CMPL STATUS: 0x000000005a1efda1 00000000 00000000
	qdma17000-MM-2 H2C online
	1: 0x0000000088caff6d 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	2: 0x0000000023211cbf 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	3: 0x000000003468cd41 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	4: 0x00000000ad729161 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	5: 0x00000000ee3b9e4b 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	6: 0x000000009d302231 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	7: 0x0000000013d70540 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	8: 0x000000004d2f1fe2 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	9: 0x00000000d59589f0 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	CMPL STATUS: 0x00000000026d0732 00000000 00000000
	Dumped descs of queues 1 -> 2.

Dump Queue Completion Information
---------------------------------

command: ``dmactl qdma<bbddf> q dump idx <N> [dir <h2c|c2h|bi>] [cmpt <x> <y>]``

Dump the queue completion information. This command is valid only for streaming c2h.

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dmactl qdma17000 q dump idx 5 dir c2h cmpt 0 10

	qdma17000-ST-5 C2H online
	0: 0x000000006d62f1aa 00000000 00000000
	1: 0x000000007a07b4ba 00000000 00000000
	2: 0x000000000f158857 00000000 00000000
	3: 0x00000000489003ed 00000000 00000000
	4: 0x0000000054d4b084 00000000 00000000
	5: 0x000000001e3d17d8 00000000 00000000
	6: 0x000000001e09b4d9 00000000 00000000
	7: 0x000000002cb94242 00000000 00000000
	8: 0x00000000dd831ff4 00000000 00000000
	9: 0x000000006a4748c3 00000000 00000000
	CMPL STATUS: 0x00000000074d569c 00000000 00000000
	Dumped descs of queues 5 -> 5.

Dump Multiple Queue Completion Information
------------------------------------------

command: ``dmactl qdma<bbddf> q dump list idx <N> [dir <h2c|c2h|bi>] [cmpt <x> <y>]``

Dumps the completion information for multiple queues. This command is valid only for streaming c2h.

**Parameters**

- <start_idx> : Starting queue number
- <N> : Number of queues to list
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start index
- <y> : Range end index

::

	[xilinx@]# dmactl qdma17000 q dump list 5 2 dir c2h cmpt 0 10

	qdma17000-ST-5 C2H online
	0: 0x000000006d62f1aa 00000000 00000000
	1: 0x000000007a07b4ba 00000000 00000000
	2: 0x000000000f158857 00000000 00000000
	3: 0x00000000489003ed 00000000 00000000
	4: 0x0000000054d4b084 00000000 00000000
	5: 0x000000001e3d17d8 00000000 00000000
	6: 0x000000001e09b4d9 00000000 00000000
	7: 0x000000002cb94242 00000000 00000000
	8: 0x00000000dd831ff4 00000000 00000000
	9: 0x000000006a4748c3 00000000 00000000
	CMPL STATUS: 0x00000000074d569c 00000000 00000000
	qdma17000-ST-6 C2H online
	0: 0x000000004ca5cbb0 00000000 00000000
	1: 0x000000003b6478d7 00000000 00000000
	2: 0x000000007dc4c8a1 00000000 00000000
	3: 0x000000003ad66591 00000000 00000000
	4: 0x00000000aad20103 00000000 00000000
	5: 0x00000000f102be8c 00000000 00000000
	6: 0x0000000046cc60b8 00000000 00000000
	7: 0x000000003dd14944 00000000 00000000
	8: 0x000000004c825f31 00000000 00000000
	9: 0x0000000026f2e4f8 00000000 00000000
	CMPL STATUS: 0x000000007bcad59e 00000000 00000000
	Dumped descs of queues 5 -> 6.

Dump the Interrupt Ring Information
-----------------------------------

command: ``dmactl qdma<bbddf> intring dump vector <N> <start_idx> <end_idx>``

Dump the interrupt ring information

**Parameters**

- <N> : Vector number
- <start_idx> : Range start index
- <end_idx> : Range end index

::

	[xilinx@]# dmactl qdma17000 intring dump vector 3 0 10

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

command: ``dmactl qdma<bbddf> reg read bar <N> <addr>``

Read a register value.

**Parameters**

- <N> : Bar number
- <addr> : Register address

::

	[xilinx@]# dmactl qdma17000 reg read bar 2 0x0
	qdma17000, 17:00.00, bar#2, 0x0 = 0x0.
	
Write a Register
----------------

command: ``dmactl qdma<bbddf> reg write bar <N> <addr>``

Write a value to into the given register.

**Parameters**

- <N> : Bar number
- <addr> : Register address

::

	[xilinx@]# dmactl qdma17000 reg write bar 2 0x0 0
	qdma17000, 17:00.00, bar#2, reg 0x0 -> 0x0, read back 0x0.
	
Dump the Queue registers
------------------------

command: ``dmactl qdma<bbddf> reg dump``

This command allows the user to dump the DMA BAR and User BAR registers.

::

	[xilinx@]# dmactl qdma17000 reg dump
	################################################################################
	###				qdma06000, reg dump
	################################################################################

	USER BAR #2
	[    0x0] ST_C2H_QID                                      0x1fd30000 533921792
	[    0x4] ST_C2H_PKTLEN                                   0x0        0
	[    0x8] ST_C2H_CONTROL                                  0x51       81
	[    0xc] ST_H2C_CONTROL                                  0x52       82
	[   0x10] ST_H2C_STATUS                                   0x1234     4660
	[   0x14] ST_H2C_XFER_CNT                                 0x2020202  33686018
	[   0x20] ST_C2H_PKT_CNT                                  0x0        0
	[   0x30] ST_C2H_CMPT_DATA_0                              0x0        0
	[   0x34] ST_C2H_CMPT_DATA_1                              0x0        0
	[   0x38] ST_C2H_CMPT_DATA_2                              0x0        0
	[   0x3c] ST_C2H_CMPT_DATA_3                              0x0        0
	[   0x40] ST_C2H_CMPT_DATA_4                              0x55       85
	[   0x44] ST_C2H_CMPT_DATA_5                              0x55       85
	[   0x48] ST_C2H_CMPT_DATA_6                              0x0        0
	[   0x4c] ST_C2H_CMPT_DATA_7                              0x10009    65545
	[   0x50] ST_C2H_CMPT_SIZE                                0x0        0
	[   0x60] ST_SCRATCH_REG_0                                0x0        0
	[   0x64] ST_SCRATCH_REG_1                                0x0        0
	[   0x88] ST_C2H_PKT_DROP                                 0x0        0
	[   0x8c] ST_C2H_PKT_ACCEPT                               0x0        0
	[   0x90] DSC_BYPASS_LOOP                                 0x0        0
	[   0x94] USER_INTERRUPT                                  0x0        0
	[   0x98] USER_INTERRUPT_MASK                             0x0        0
	[   0x9c] USER_INTERRUPT_VEC                              0x0        0
	[   0xa0] DMA_CONTROL                                     0x0        0
	[   0xa4] VDM_MSG_READ                                    0x0        0

	CONFIG BAR #0
	[    0x0] CFG_BLOCK_ID                                    0x1fd30000 533921792
	[    0x4] CFG_BUSDEV                                      0x0        0
	[    0x8] CFG_PCIE_MAX_PL_SZ                              0x51       81
	[    0xc] CFG_PCIE_MAX_RDRQ_SZ                            0x52       82
	[   0x10] CFG_SYS_ID                                      0x1234     4660
	[   0x14] CFG_MSI_EN                                      0x2020202  33686018
	[   0x18] CFG_PCIE_DATA_WIDTH                             0x3        3
	[   0x1c] CFG_PCIE_CTRL                                   0x1        1
	[   0x40] CFG_AXI_USR_MAX_PL_SZ                           0x55       85
	[   0x44] CFG_AXI_USR_MAX_RDRQ_SZ                         0x55       85
	[   0x4c] CFG_MISC_CTRL                                   0x10009    65545
	[   0x80] CFG_SCRATCH_REG_0                               0x0        0
	[   0x84] CFG_SCRATCH_REG_1                               0x0        0
	[   0x88] CFG_SCRATCH_REG_2                               0x0        0
	[   0x8c] CFG_SCRATCH_REG_3                               0x0        0
	[   0x90] CFG_SCRATCH_REG_4                               0x0        0
	[   0x94] CFG_SCRATCH_REG_5                               0x0        0
	[   0x98] CFG_SCRATCH_REG_6                               0x0        0
	[   0x9c] CFG_SCRATCH_REG_7                               0x0        0
	[   0xf0] QDMA_RAM_SBE_MSK_A                              0xffffff11 4294967057
	[   0xf4] QDMA_RAM_SBE_STS_A                              0x0        0
	[   0xf8] QDMA_RAM_DBE_MSK_A                              0xffffff11 4294967057
	[   0xfc] QDMA_RAM_DBE_STS_A                              0x0        0
	[  0x100] GLBL2_ID                                        0x1fd70000 534183936
	[  0x104] GLBL2_PF_BL_INT                                 0x41041    266305
	[  0x108] GLBL2_PF_VF_BL_INT                              0x41041    266305
	[  0x10c] GLBL2_PF_BL_EXT                                 0x104104   1065220
	[  0x110] GLBL2_PF_VF_BL_EXT                              0x104104   1065220
	[  0x114] GLBL2_CHNL_INST                                 0x30101    196865
	[  0x118] GLBL2_CHNL_QDMA                                 0x30f0f    200463
	[  0x11c] GLBL2_CHNL_STRM                                 0x30000    196608
	[  0x120] GLBL2_QDMA_CAP                                  0x800      2048
	[  0x128] GLBL2_PASID_CAP                                 0x0        0
	[  0x12c] GLBL2_FUNC_RET                                  0x0        0
	[  0x130] GLBL2_SYS_ID                                    0x0        0
	[  0x134] GLBL2_MISC_CAP                                  0x20003    131075
	[  0x1b8] GLBL2_DBG_PCIE_RQ_0                             0x7c50003  130351107
	[  0x1bc] GLBL2_DBG_PCIE_RQ_1                             0x6024     24612
	[  0x1c0] GLBL2_DBG_AXIMM_WR_0                            0x600021   6291489
	[  0x1c4] GLBL2_DBG_AXIMM_WR_1                            0x0        0
	[  0x1c8] GLBL2_DBG_AXIMM_RD_0                            0x1        1
	[  0x1cc] GLBL2_DBG_AXIMM_RD_1                            0x0        0
	[  0x204] GLBL_RNGSZ_0                                    0x41       65
	[  0x208] GLBL_RNGSZ_1                                    0x81       129
	[  0x20c] GLBL_RNGSZ_2                                    0x101      257
	[  0x210] GLBL_RNGSZ_3                                    0x201      513
	[  0x214] GLBL_RNGSZ_4                                    0x401      1025
	[  0x218] GLBL_RNGSZ_5                                    0x801      2049
	[  0x21c] GLBL_RNGSZ_6                                    0x1001     4097
	[  0x220] GLBL_RNGSZ_7                                    0x2001     8193
	[  0x224] GLBL_RNGSZ_8                                    0x4001     16385
	[  0x228] GLBL_RNGSZ_9                                    0x8001     32769
	[  0x22c] GLBL_RNGSZ_10                                   0x100      256
	[  0x230] GLBL_RNGSZ_11                                   0x200      512
	[  0x234] GLBL_RNGSZ_12                                   0x400      1024
	[  0x238] GLBL_RNGSZ_13                                   0x800      2048
	[  0x23c] GLBL_RNGSZ_14                                   0x1000     4096
	[  0x240] GLBL_RNGSZ_15                                   0x2000     8192
	[  0x248] GLBL_ERR_STAT                                   0x0        0
	[  0x24c] GLBL_ERR_MASK                                   0xfff      4095
	[  0x250] GLBL_DSC_CFG                                    0x1d       29
	[  0x254] GLBL_DSC_ERR_STS                                0x0        0
	[  0x258] GLBL_DSC_ERR_MSK                                0x1f9023f  33096255
	[  0x25c] GLBL_DSC_ERR_LOG_0                              0x0        0
	[  0x260] GLBL_DSC_ERR_LOG_1                              0x0        0
	[  0x264] GLBL_TRQ_ERR_STS                                0x0        0
	[  0x268] GLBL_TRQ_ERR_MSK                                0xf        15
	[  0x26c] GLBL_TRQ_ERR_LOG                                0x0        0
	[  0x270] GLBL_DSC_DBG_DAT_0                              0xe0000    917504
	[  0x274] GLBL_DSC_DBG_DAT_1                              0x8080     32896
	[  0x27c] GLBL_DSC_ERR_LOG2                               0x0        0
	[  0x288] GLBL_INTERRUPT_CFG                              0x0        0
	[  0x400] TRQ_SEL_FMAP_0                                  0x5000     20480
	[  0x404] TRQ_SEL_FMAP_1                                  0x0        0
	[  0x408] TRQ_SEL_FMAP_2                                  0x0        0
	[  0x40c] TRQ_SEL_FMAP_3                                  0x0        0
	[  0x804] IND_CTXT_DATA_0                                 0x9ed64007 2664841223
	[  0x808] IND_CTXT_DATA_1                                 0x1        1
	[  0x80c] IND_CTXT_DATA_2                                 0xc0       192
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
	[  0x844] IND_CTXT_CMD                                    0x50       80
	[  0xa00] C2H_TIMER_CNT_0                                 0x1        1
	[  0xa04] C2H_TIMER_CNT_1                                 0x2        2
	[  0xa08] C2H_TIMER_CNT_2                                 0x4        4
	[  0xa0c] C2H_TIMER_CNT_3                                 0x8        8
	[  0xa10] C2H_TIMER_CNT_4                                 0x10       16
	[  0xa14] C2H_TIMER_CNT_5                                 0x20       32
	[  0xa18] C2H_TIMER_CNT_6                                 0x40       64
	[  0xa1c] C2H_TIMER_CNT_7                                 0x80       128
	[  0xa20] C2H_TIMER_CNT_8                                 0x5        5
	[  0xa24] C2H_TIMER_CNT_9                                 0xa        10
	[  0xa28] C2H_TIMER_CNT_10                                0x14       20
	[  0xa2c] C2H_TIMER_CNT_11                                0x32       50
	[  0xa30] C2H_TIMER_CNT_12                                0x64       100
	[  0xa34] C2H_TIMER_CNT_13                                0x0        0
	[  0xa38] C2H_TIMER_CNT_14                                0x0        0
	[  0xa3c] C2H_TIMER_CNT_15                                0x0        0
	[  0xa40] C2H_CNT_THRESH_0                                0x1        1
	[  0xa44] C2H_CNT_THRESH_1                                0x2        2
	[  0xa48] C2H_CNT_THRESH_2                                0x4        4
	[  0xa4c] C2H_CNT_THRESH_3                                0x8        8
	[  0xa50] C2H_CNT_THRESH_4                                0x10       16
	[  0xa54] C2H_CNT_THRESH_5                                0x20       32
	[  0xa58] C2H_CNT_THRESH_6                                0x40       64
	[  0xa5c] C2H_CNT_THRESH_7                                0x80       128
	[  0xa60] C2H_CNT_THRESH_8                                0x0        0
	[  0xa64] C2H_CNT_THRESH_9                                0x0        0
	[  0xa68] C2H_CNT_THRESH_10                               0x0        0
	[  0xa6c] C2H_CNT_THRESH_11                               0x0        0
	[  0xa70] C2H_CNT_THRESH_12                               0x0        0
	[  0xa74] C2H_CNT_THRESH_13                               0x0        0
	[  0xa78] C2H_CNT_THRESH_14                               0x0        0
	[  0xa7c] C2H_CNT_THRESH_15                               0x0        0
	[  0xa88] C2H_STAT_S_AXIS_C2H_ACCEPTED                    0x1        1
	[  0xa8c] C2H_STAT_S_AXIS_CMPT_ACCEPTED                   0x3        3
	[  0xa90] C2H_STAT_DESC_RSP_PKT_ACCEPTED                  0x1        1
	[  0xa94] C2H_STAT_AXIS_PKG_CMP                           0x1        1
	[  0xa98] C2H_STAT_DESC_RSP_ACCEPTED                      0x1        1
	[  0xa9c] C2H_STAT_DESC_RSP_CMP                           0x1        1
	[  0xaa0] C2H_STAT_WRQ_OUT                                0x4        4
	[  0xaa4] C2H_STAT_WPL_REN_ACCEPTED                       0x10       16
	[  0xaa8] C2H_STAT_TOTAL_WRQ_LEN                          0x400      1024
	[  0xaac] C2H_STAT_TOTAL_WPL_LEN                          0x400      1024
	[  0xab0] C2H_BUF_SZ_0                                    0x1000     4096
	[  0xab4] C2H_BUF_SZ_1                                    0x100      256
	[  0xab8] C2H_BUF_SZ_2                                    0x200      512
	[  0xabc] C2H_BUF_SZ_3                                    0x400      1024
	[  0xac0] C2H_BUF_SZ_4                                    0x800      2048
	[  0xac4] C2H_BUF_SZ_5                                    0xf80      3968
	[  0xac8] C2H_BUF_SZ_6                                    0x2000     8192
	[  0xacc] C2H_BUF_SZ_7                                    0x233a     9018
	[  0xad0] C2H_BUF_SZ_8                                    0x4000     16384
	[  0xad4] C2H_BUF_SZ_9                                    0x1000     4096
	[  0xad8] C2H_BUF_SZ_10                                   0x1000     4096
	[  0xadc] C2H_BUF_SZ_11                                   0x1000     4096
	[  0xae0] C2H_BUF_SZ_12                                   0x1000     4096
	[  0xae4] C2H_BUF_SZ_13                                   0x1000     4096
	[  0xae8] C2H_BUF_SZ_14                                   0x1000     4096
	[  0xaec] C2H_BUF_SZ_15                                   0x1000     4096
	[  0xaf0] C2H_ERR_STAT                                    0x0        0
	[  0xaf4] C2H_ERR_MASK                                    0xfedb     65243
	[  0xaf8] C2H_FATAL_ERR_STAT                              0x0        0
	[  0xafc] C2H_FATAL_ERR_MASK                              0x7df1b    515867
	[  0xb00] C2H_FATAL_ERR_ENABLE                            0x0        0
	[  0xb04] GLBL_ERR_INT                                    0x1002000  16785408
	[  0xb08] C2H_PFCH_CFG                                    0x4101100  68161792
	[  0xb0c] C2H_INT_TIMER_TICK                              0x19       25
	[  0xb10] C2H_STAT_DESC_RSP_DROP_ACCEPTED                 0x0        0
	[  0xb14] C2H_STAT_DESC_RSP_ERR_ACCEPTED                  0x0        0
	[  0xb18] C2H_STAT_DESC_REQ                               0x1        1
	[  0xb1c] C2H_STAT_DEBUG_DMA_ENG_0                        0x0        0
	[  0xb20] C2H_STAT_DEBUG_DMA_ENG_1                        0x1004001  16793601
	[  0xb24] C2H_STAT_DEBUG_DMA_ENG_2                        0x40100401 1074791425
	[  0xb28] C2H_STAT_DEBUG_DMA_ENG_3                        0x1004     4100
	[  0xb2c] C2H_DBG_PFCH_ERR_CTXT                           0x1d002    118786
	[  0xb30] C2H_FIRST_ERR_QID                               0x0        0
	[  0xb34] STAT_NUM_CMPT_IN                                0x1        1
	[  0xb38] STAT_NUM_CMPT_OUT                               0x1        1
	[  0xb3c] STAT_NUM_CMPT_DRP                               0x0        0
	[  0xb40] STAT_NUM_STAT_DESC_OUT                          0x1        1
	[  0xb44] STAT_NUM_DSC_CRDT_SENT                          0x1        1
	[  0xb48] STAT_NUM_FCH_DSC_RCVD                           0x1        1
	[  0xb4c] STAT_NUM_BYP_DSC_RCVD                           0x0        0
	[  0xb50] C2H_CMPT_COAL_CFG                               0x40064014 1074151444
	[  0xb54] C2H_INTR_H2C_REQ                                0xb4ba     46266
	[  0xb58] C2H_INTR_C2H_MM_REQ                             0xac99     44185
	[  0xb5c] C2H_INTR_ERR_INT_REQ                            0x0        0
	[  0xb60] C2H_INTR_C2H_ST_REQ                             0x1        1
	[  0xb64] C2H_INTR_H2C_ERR_MM_MSIX_ACK                    0x1614e    90446
	[  0xb68] C2H_INTR_H2C_ERR_MM_MSIX_FAIL                   0x0        0
	[  0xb6c] C2H_INTR_H2C_ERR_MM_NO_MSIX                     0x5        5
	[  0xb70] C2H_INTR_H2C_ERR_MM_CTXT_INVAL                  0x0        0
	[  0xb74] C2H_INTR_C2H_ST_MSIX_ACK                        0x1        1
	[  0xb78] C2H_INTR_C2H_ST_MSIX_FAIL                       0x0        0
	[  0xb7c] C2H_INTR_C2H_ST_NO_MSIX                         0x0        0
	[  0xb80] C2H_INTR_C2H_ST_CTXT_INVAL                      0x0        0
	[  0xb84] C2H_STAT_WR_CMP                                 0x1        1
	[  0xb88] C2H_STAT_DEBUG_DMA_ENG_4                        0x80300402 2150630402
	[  0xb8c] C2H_STAT_DEBUG_DMA_ENG_5                        0x0        0
	[  0xb90] C2H_DBG_PFCH_QID                                0x0        0
	[  0xb94] C2H_DBG_PFCH                                    0x0        0
	[  0xb98] C2H_INT_DEBUG                                   0x0        0
	[  0xb9c] C2H_STAT_IMM_ACCEPTED                           0x0        0
	[  0xba0] C2H_STAT_MARKER_ACCEPTED                        0x0        0
	[  0xba4] C2H_STAT_DISABLE_CMP_ACCEPTED                   0x0        0
	[  0xba8] C2H_C2H_PAYLOAD_FIFO_CRDT_CNT                   0x0        0
	[  0xbac] C2H_INTR_DYN_REQ                                0x16150    90448
	[  0xbb0] C2H_INTR_DYN_MSIX                               0x1        1
	[  0xbb4] C2H_DROP_LEN_MISMATCH                           0x0        0
	[  0xbb8] C2H_DROP_DESC_RSP_LEN                           0x0        0
	[  0xbbc] C2H_DROP_QID_FIFO_LEN                           0x0        0
	[  0xbc0] C2H_DROP_PAYLOAD_CNT                            0x0        0
	[  0xbc4] QDMA_C2H_CMPT_FORMAT_0                          0x20001    131073
	[  0xbc8] QDMA_C2H_CMPT_FORMAT_1                          0x0        0
	[  0xbcc] QDMA_C2H_CMPT_FORMAT_2                          0x0        0
	[  0xbd0] QDMA_C2H_CMPT_FORMAT_3                          0x0        0
	[  0xbd4] QDMA_C2H_CMPT_FORMAT_4                          0x0        0
	[  0xbd8] QDMA_C2H_CMPT_FORMAT_5                          0x0        0
	[  0xbdc] QDMA_C2H_CMPT_FORMAT_6                          0x0        0
	[  0xbe0] C2H_PFCH_CACHE_DEPTH                            0x10       16
	[  0xbe4] C2H_CMPT_COAL_BUF_DEPTH                         0x10       16
	[  0xbe8] C2H_PFCH_CRDT                                   0x0        0
	[  0xe00] H2C_ERR_STAT                                    0x0        0
	[  0xe04] H2C_ERR_MASK                                    0x7        7
	[  0xe08] H2C_FIRST_ERR_QID                               0x0        0
	[  0xe0c] H2C_DBG_REG_0                                   0x8650821  140838945
	[  0xe10] H2C_DBG_REG_1                                   0x42de16d4 1121851092
	[  0xe14] H2C_DBG_REG_2                                   0x0        0
	[  0xe18] H2C_DBG_REG_3                                   0x44008025 1140883493
	[  0xe1c] H2C_DBG_REG_4                                   0x7583b000 1971564544
	[  0xe20] H2C_FATAL_ERR_EN                                0x0        0
	[  0xe24] H2C_REQ_THROT                                   0x14000    81920
	[  0xe28] H2C_ALN_DBG_REG0                                0x865      2149
	[ 0x1004] C2H_MM_CONTROL_0                                0x1        1
	[ 0x1008] C2H_MM_CONTROL_1                                0x1        1
	[ 0x100c] C2H_MM_CONTROL_2                                0x1        1
	[ 0x1040] C2H_MM_STATUS_0                                 0x1        1
	[ 0x1044] C2H_MM_STATUS_1                                 0x1        1
	[ 0x1048] C2H_MM_CMPL_DSC_CNT                             0x4        4
	[ 0x1054] C2H_MM_ERR_CODE_EN_MASK                         0x0        0
	[ 0x1058] C2H_MM_ERR_CODE                                 0x0        0
	[ 0x105c] C2H_MM_ERR_INFO                                 0x0        0
	[ 0x10c0] C2H_MM_PERF_MON_CTRL                            0x0        0
	[ 0x10c4] C2H_MM_PERF_MON_CY_CNT_0                        0x0        0
	[ 0x10c8] C2H_MM_PERF_MON_CY_CNT_1                        0x0        0
	[ 0x10cc] C2H_MM_PERF_MON_DATA_CNT_0                      0x0        0
	[ 0x10d0] C2H_MM_PERF_MON_DATA_CNT_1                      0x0        0
	[ 0x10e8] C2H_MM_DBG_INFO_0                               0x10002    65538
	[ 0x10ec] C2H_MM_DBG_INFO_1                               0x0        0
	[ 0x1204] H2C_MM_CONTROL_0                                0x1        1
	[ 0x1208] H2C_MM_CONTROL_1                                0x1        1
	[ 0x120c] H2C_MM_CONTROL_2                                0x1        1
	[ 0x1240] H2C_MM_STATUS                                   0x1        1
	[ 0x1248] H2C_MM_CMPL_DSC_CNT                             0xaa909    698633
	[ 0x1254] H2C_MM_ERR_CODE_EN_MASK                         0x0        0
	[ 0x1258] H2C_MM_ERR_CODE                                 0x0        0
	[ 0x125c] H2C_MM_ERR_INFO                                 0x0        0
	[ 0x12c0] H2C_MM_PERF_MON_CTRL                            0x0        0
	[ 0x12c4] H2C_MM_PERF_MON_CY_CNT_0                        0x0        0
	[ 0x12c8] H2C_MM_PERF_MON_CY_CNT_1                        0x0        0
	[ 0x12cc] H2C_MM_PERF_MON_DATA_CNT_0                      0x0        0
	[ 0x12d0] H2C_MM_PERF_MON_DATA_CNT_1                      0x0        0
	[ 0x12e8] H2C_MM_DBG_INFO                                 0x10002    65538
	[ 0x12ec] H2C_MM_REQ_THROT                                0x8000     32768
	[ 0x2400] FUNC_STATUS                                     0x0        0
	[ 0x2404] FUNC_CMD                                        0xffffffff 4294967295
	[ 0x2408] FUNC_INTR_VEC                                   0x0        0
	[ 0x240c] TARGET_FUNC                                     0x0        0
	[ 0x2410] INTR_CTRL                                       0x1        1
	[ 0x2420] PF_ACK_0                                        0x0        0
	[ 0x2424] PF_ACK_1                                        0x0        0
	[ 0x2428] PF_ACK_2                                        0x0        0
	[ 0x242c] PF_ACK_3                                        0x0        0
	[ 0x2430] PF_ACK_4                                        0x0        0
	[ 0x2434] PF_ACK_5                                        0x0        0
	[ 0x2438] PF_ACK_6                                        0x0        0
	[ 0x243c] PF_ACK_7                                        0x0        0
	[ 0x2500] FLR_CTRL_STATUS                                 0x0        0
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