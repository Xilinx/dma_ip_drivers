******************************
QDMA Debug File System Support
******************************

debugfs is a file system interface, which the kernel drivers uses to expose debug information to the user space. Unlike sysfs, there is no structure or rules on what to publish through this interface.debugfs files are created in /sys/kernel/debug/ folder.

debugfs supports:

- Files with assigned file operations. These file interfaces can be either read-only or read/write. 
- Files to export single integer that can be read-only or read/write
- Read-only data blob files.

QDMA Kernel driver uses this facility and exposes qdma information similar to ``dmactl`` user space application. The advantage of debugfs is that users need not depend on user space application but if they integrate the ``libqdma`` in kernel space driver, it comes along with this debug facility.


During PF/VF module loading, QDMA driver creates PF/VF directories in ``/sys/kernel/debug`` by the names ``qdma_pf`` and ``qdma_vf``. qdma_pf directory will list all the detected primary functions, similarly qdma_vf will list all virtual functions.

::
 
	/sys/kernel/debug/qdma_pf
	/sys/kernel/debug/qdma_vf
	
In driver module initialization, during PCIe probe, when a device that is to be handled by QDMA driver is detected, a directory with the name b:d.f format will be created in qdma_<df> directory. Ex: when qdma device 01000 is detected, a directory is created in qdma_pf ``/sys/kernel/debug/qdma_pf/01:00.0/``

Below files and directories are created during the device initialization.

- **info** : This file will give all the information specific to the device. For example, function number, is master PF, Q base and Qmax configured for the device etc. 
- **regs**: This file will dump all the registers corresponding to the device. The registers are the configuration space and user space of the device. These also include global registers, which are common across all the devices.
- **queues**: This is the directory and is created to hold queues information attached to the device. The qdma_dev_conf structure will also hold pointer to this directory in dbgfs_queues_root, which would help to create queue information when a queue is added to the device.

::

	/sys/kernel/debug/qdma_pf/01:00.0/info
	/sys/kernel/debug/qdma_pf/01:00.0/regs
	/sys/kernel/debug/qdma_pf/01:00.0/queues



When a queue is added to a QDMA device, per say, through dmactl, a directory with the qid is created in the corresponding ``queues`` directory of the device. This will be used to create further directories under ‘qid’ directory.

For example, if a queue with qid 2 is added to device qdma01000, the directory with name ‘2’ is created in queues directory

::

	/sys/kernel/debug/qdma_pf/01:00.0/queues/2
	
When a queue is added to a QDMA device, a direction directory with the name corresponding to the direction, c2h, h2c and cmpt, is created under qid directory.

- If the queue is added in H2C direction, directory with name ``h2c`` is created in the corresponding qid directory. 
- If C2H queue is added, then directory with name ``c2h`` is created in the corresponding queue directory. If C2H queue is added in streaming mode, then a ``cmpt`` directory is also added to the qid directory.
- If queue is added in bidirectional mode, then both ``c2h`` and ``h2c`` directories are created in the corresponding qid directory
- Lastly, if a bidirectional queues are added in streaming mode, then a ‘cmpt’ directory is also created in the respective qid directory.


For example, if a queue with qid 2 is created in bi direction ST, then all c2h, h2c and cmpt directories are created 

::

	/sys/kernel/debug/qdma_pf/01:00.0/queues/2/h2c
	/sys/kernel/debug/qdma_pf/01:00.0/queues/2/c2h
	/sys/kernel/debug/qdma_pf/01:00.0/queues/2/cmpt



- **info**: This file holds the information specific to the queue, for example if queue is on line etc
- **cntxt**: This file will dump the contexts corresponding the queue. ``cntxt`` in ``cmpt`` directory will only dump the C2H ST completion context.
- **desc**:  This file will dump the descriptor contents of the ring.

::

	/sys/kernel/debug/qdma_pf/01:00.0/queues/2/h2c/info
	/sys/kernel/debug/qdma_pf/01:00.0/queues/2/h2c/cntxt
	/sys/kernel/debug/qdma_pf/01:00.0/queues/2/h2c/desc



use ``cat`` command for displaying the contents of these leaf node debug files. Example is provided below.

::

	[xilinx@]# cat cntxt 
	SOFTWARE CTXT:
			Interrupt Aggregation                           0x1        1
			Ring Index                                      0x0        0
			Descriptor Ring Base Addr (High)                0x0        0
			Descriptor Ring Base Addr (Low)                 0x34048000 872710144
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
			PIDX                                            0x40a      1034

	HARDWARE CTXT:
			Fetch Pending                                   0x0        0
			Eviction Pending                                0x0        0
			Queue Invalid No Desc Pending                   0x1        1
			Descriptors Pending                             0x1        1
			Credits Consumed                                0x0        0
			CIDX                                            0x40a      1034
 

