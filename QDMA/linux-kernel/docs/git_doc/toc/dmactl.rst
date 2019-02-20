***********************
DMA Control Application
***********************

QDMA driver comes with a command-line configuration utility called ``dmactl`` to manage the driver.

The Xilinx QDMA control tool, ``dmactl`` is a command Line utility which is installed in ``/usr/local/sbin/`` and allows administration of the Xilinx QDMA queues. Make sure that the installation path ``/usr/local/sbin/`` is added to the ``PATH`` variable.

It can perform the following functions:

- Query the list of QDMA functions the driver has bind into 
- Query control and configuration 

   - List all the queues on a device
   - Add/configure a new queue on a device
   - Start an already added/configured queue, i.e., bring the queue to online mode
   - Stop a started queue, i.e., bring the queue to offline mode
   - Delete an already configured queue
   
- Register access

   - Read a register
   - Write a register
   - Dump the qdma config bar and user bar registers
   
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
::

   [xilinx@]# dmactl dev list

   qdma01000       0000:01:00.0    max QP: 512, 0~511
   qdma01001       0000:01:00.1    max QP: 512, 512~1023
   qdma01002       0000:01:00.2    max QP: 512, 1024~1535
   qdma01003       0000:01:00.3    max QP: 512, 1536~2047

=========================
Queue Management Commands
=========================

List Version
------------

command: ``dmactl qdma01000 version``

This command lists hardware and software version details
::

   [xilinx@]# dmactl qdma01000 version

   =============Hardware Version============

   RTL Version       : RTL2
   Vivado ReleaseID  : 2018.3
   Everest IP        : SOFT_IP

   ============Software Version============

   qdma driver version       : 2018.3.97.162.


List Device Statistics
-----------------------

command: ``dmactl qdma01000 stat``

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

command: ``dmactl qdma01000 q add idx <N> [mode <st|mm>] [dir <h2c|c2h|bi>]``

This command allows the user to add a queue.

**Parameters**

- <N> : Queue number
- mode : mode of the queue, streaming\(st\) or memory mapped\(mm\). Mode defaults to mm.
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dmactl qdma01000 q add idx 4 mode mm dir h2c

   qdma01000-MM-4 H2C added.
   Added 1 Queues.

Add a List of Queues
--------------------

command: ``dmactl qdma01000 q add list <start_idx> <N>  [ mode <st|mm> ] [ dir <h2c|c2h|bi> ]``

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

command: 
   dmactl qdma01000 q start idx <N> [dir <h2c|c2h|bi>] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>] \
   [idx_cntr <0:15>] [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [desc_bypass_en] [pfetch_en] \
   [pfetch_bypass_en] [dis_cmpl_status] [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en] [dis_fetch_credit] \ 
   [dis_cmpt_stat] [c2h_cmpl_intr_en] [cmpl_ovf_dis]

This command allows the user to start a queue.

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.
- idx_ringsz: CSR register ring size index
- idx_bufsz : CSR register buffer size index
- idx_tmr : CSR register timer index
- idx_cntr: CSR register counter index
- trigmode: Timer trigger mode \(every, user counter, user, user timer, disabled\)
- cmptsz : Completion size \( 0: 8 bytes, 1: 16 bytes, 2:32 bytes, 3:64 bytes\)
- desc_bypass_en : Enable descriptor bypass
- pfetch_en : Enable prefetch
- pfetch_bypass_en : Enable prefetch bypass
- dis_cmpl_status : Disable completion status update
- dis_cmpl_status_pend_chk : Disable completion status pending check
- c2h_udd_en : Enable immdeiate data\(User Defined Data\)
- dis_fetch_credit: Disable fetch credit
- dis_cmpt_stat : Disable completion status
- c2h_cmpl_intr_en : Enable c2h completion interval
- cmpl_ovf_dis : Disable completion over flow check

::

   [xilinx@]# dmactl qdma01000 q start idx 4 dir h2c
   dmactl: Info: Default ring size set to 2048

   1 Queues started, idx 4 ~ 4.

Start a List of Queues
----------------------

command:
   dmactl qdma01000 q start list <start_idx> <N> [dir <h2c|c2h|bi>] [idx_ringsz <0:15>] [idx_bufsz <0:15>] [idx_tmr <0:15>] [idx_cntr <0:15>] \
   [trigmode <every|usr_cnt|usr|usr_tmr|dis>] [cmptsz <0|1|2|3>] [desc_bypass_en] [pfetch_en] [pfetch_bypass_en] [dis_cmpl_status] \
   [dis_cmpl_status_acc] [dis_cmpl_status_pend_chk] [c2h_udd_en] [dis_fetch_credit] [dis_cmpt_stat] [c2h_cmpl_intr_en] [cmpl_ovf_dis]

This command allows the user to start a list of queues.

**Parameters**

- <start_idx> : Starting queue number
- <N> :Number of queues to delete
- dir : direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.
- idx_ringsz: CSR register ring size index
- idx_bufsz : CSR register buffer size index
- idx_tmr : CSR register timer index
- idx_cntr: CSR register counter index
- trigmode: Timer trigger mode \(every, user counter, user, user timer, disabled\)
- cmptsz : Completion size \( 0: 8 bytes, 1: 16 bytes, 2:32 bytes, 3:64 bytes\)
- desc_bypass_en : Enable descriptor bypass
- pfetch_en : Enable prefetch
- pfetch_bypass_en : Enable prefetch bypass
- dis_cmpl_status : Disable completion status update
- dis_cmpl_status_pend_chk : Disable completion status pending check
- c2h_udd_en : Enable immdeiate data\(User Defined Data\)
- dis_fetch_credit: Disable fetch credit
- dis_cmpt_stat : Disable completion status
- c2h_cmpl_intr_en : Enable c2h completion interval
- cmpl_ovf_dis : Disable completion over flow check

::

   [xilinx@]# dmactl qdma01000 q start list 1 4 dir h2c

   Started Queues 1 -> 4.
   
Stop a Queue
------------

command: ``dmactl qdma01000 q stop idx <N> [dir <h2c|c2h|bi>]``

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

command: ``dmactl qdma01000 q stop list <start_idx> <N> [dir <h2c|c2h|bi>]``

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

command: ``dmactl qdma01000 q del idx <N> [dir <h2c|c2h|bi>]``

This command allows the user to delete a queue.

**Parameters**

- <N> : Queue number
- dir : direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\). Directions defaults to h2c.

::

   [xilinx@]# dmactl qdma01000 q del idx 4 mode mm dir h2c

   Deleted Queues 4 -> 4.
   
Delete a List of Queues
-----------------------

command: ``dmactl qdma01000 q del list <start_idx> <N> [ dir <h2c|c2h|bi> ]``

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

command: ``dmactl qdma01000 q dump idx <N> [dir <h2c|c2h|bi>]``

Dump the queue information

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::

   [xilinx@]# dmactl qdma01000 q dump idx 4
   dmactl: Warn: Default dir set to 'h2c'

   qdma01000-MM-4 H2C online
        hw_ID 4, thp ?, desc 0x00000000c6b67be7/0x34580000, 2048

        cmpl status: 0x0000000007d59836, 00000000 00000000
        SW CTXT:    [4]:0x00000004 [3]:0x00000000 [2]:0x34580000 [1]:0x8032500d [0]:0x00010000
        HW CTXT:    [1]:0x00000200 [0]:0x00000000
        CR CTXT:    0x00000000
        total descriptor processed:    0
   Dumped Queues 4 -> 4.

   
Dump Multiple Queue Information
-------------------------------

command: ``dmactl qdma01000 q dump idx <N> [dir <h2c|c2h|bi>]``

Dumps the information for multiple queues

**Parameters**

- <start_idx> : Starting queue number
- <N> :Number of queues to add
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)

::

   [xilinx@]# dmactl qdma01000 q dump list 1 4 dir h2c

   qdma01000-MM-1 H2C online
        hw_ID 1, thp ?, desc 0x00000000019767a1/0x33800000, 2048

        cmpl status: 0x000000005a1efda1, 00000000 00000000
        SW CTXT:    [4]:0x00000007 [3]:0x00000000 [2]:0x33800000 [1]:0x8032500d [0]:0x00010000
        HW CTXT:    [1]:0x00000200 [0]:0x00000000
        CR CTXT:    0x00000000
        total descriptor processed:    0
   qdma01000-MM-2 H2C online
        hw_ID 2, thp ?, desc 0x00000000bafedba5/0x31b60000, 2048

        cmpl status: 0x00000000026d0732, 00000000 00000000
        SW CTXT:    [4]:0x00000002 [3]:0x00000000 [2]:0x31b60000 [1]:0x8032500d [0]:0x00010000
        HW CTXT:    [1]:0x00000200 [0]:0x00000000
        CR CTXT:    0x00000000
        total descriptor processed:    0
   qdma01000-MM-3 H2C online
        hw_ID 3, thp ?, desc 0x00000000dc0dd592/0x34340000, 2048

        cmpl status: 0x00000000b015638d, 00000000 00000000
        SW CTXT:    [4]:0x00000003 [3]:0x00000000 [2]:0x34340000 [1]:0x8032500d [0]:0x00010000
        HW CTXT:    [1]:0x00000200 [0]:0x00000000
        CR CTXT:    0x00000000
        total descriptor processed:    0
   qdma01000-MM-4 H2C online
        hw_ID 4, thp ?, desc 0x00000000c6b67be7/0x34580000, 2048

        cmpl status: 0x0000000007d59836, 00000000 00000000
        SW CTXT:    [4]:0x00000004 [3]:0x00000000 [2]:0x34580000 [1]:0x8032500d [0]:0x00010000
        HW CTXT:    [1]:0x00000200 [0]:0x00000000
        CR CTXT:    0x00000000
        total descriptor processed:    0
   Dumped Queues 1 -> 4.
   
Dump Queue Decriptor Information
--------------------------------

command: ``dmactl qdma01000 q dump idx <N> [dir <h2c|c2h|bi>] [desc <x> <y>]``

Dump the queue descriptor information

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start
- <y> : Range end

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

command: ``dmactl qdma01000 q dump list idx <N> [dir <h2c|c2h|bi>] [desc <x> <y>]``

Dumps the descriptor information for multiple queues

**Parameters**

- <start_idx> : Starting queue number
- <N> :Number of queues to add
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start
- <y> : Range end

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

command: ``dmactl qdma01000 q dump idx <N> [dir <h2c|c2h|bi>] [cmpt <x> <y>]``

Dump the queue completion information. This command is valid only for streaming c2h.

**Parameters**

- <N> : Queue number
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start
- <y> : Range end

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

command: ``dmactl qdma01000 q dump list idx <N> [dir <h2c|c2h|bi>] [cmpt <x> <y>]``

Dumps the completion information for multiple queues. This command is valid only for streaming c2h.

**Parameters**

- <start_idx> : Starting queue number
- <N> :Number of queues to add
- dir : Direction of the queue, host-to-card\(h2c\), card-to-host \(c2h\) or both \(bi\)
- <x> : Range start
- <y> : Range end

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

command: ``dmactl qdma01000 intring dump vector <N> <start_idx> <end_idx>``

Dump the interrupt ring information

**Parameters**

- <N> : Vector number
- <start_idx> : Range start
- <end_idx> : Range end

::

	[xilinx@]# dmactl qdma17000 intring dump vector 2 0 10


=================
Register Commands
=================

Read a Register
---------------

command: ``dmactl qdma01000 reg read bar <N> <addr>``

Read a register value.

**Parameters**

- <N> : Bar number
- <addr> : Register address

::

	[xilinx@]# dmactl qdma17000 reg read bar 2 0x0
	qdma17000, 17:00.00, bar#2, 0x0 = 0x0.
	
Write a Register
----------------

command: ``dmactl qdma01000 reg write bar <N> <addr>``

Read a register value.

**Parameters**

- <N> : Bar number
- <addr> : Register address

::

	[xilinx@]# dmactl qdma17000 reg write bar 2 0x0 0
	qdma17000, 17:00.00, bar#2, reg 0x0 -> 0x0, read back 0x0.
	
Dump the Queue registers
------------------------

command: ``dmactl qdma01000 reg dump [dmap <Q> <N>]``

This command allows the user to dump the registers. Only dump dmap registers if dmap is specified. Specify dmap range to dump: Q=queue, N=num of queues.

**Parameters**

- <Q> : Queue number
- <N> : Number of queues

::

	[xilinx@]# dmactl qdma17000 reg dump dmap 1 2