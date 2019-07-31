User Guide
==========

This section describes the details on controlling and configuring the QDMA IP

System Level Configurations
---------------------------

QDMA driver provides the sysfs interface to enable to user to perform system level configurations. QDMA ``PF`` and ``VF`` drivers expose several ``sysfs`` nodes under the ``pci`` device root node. ``sysfs`` provides an interface to configure  the module.

::

	[xilinx@]# lspci | grep -i Xilinx

	01:00.0 Memory controller: Xilinx Corporation Device 903f
	01:00.1 Memory controller: Xilinx Corporation Device 913f
	01:00.2 Memory controller: Xilinx Corporation Device 923f
	01:00.3 Memory controller: Xilinx Corporation Device 933f

Based on the above lspci output, traverse to ``/sys/bus/pci/devices/<device node>/qdma`` to find the list of configurable parameters specific to PF or VF driver.

1. **Instantiate the Virtual Functions**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QDMA IP supports 252 Virtual Functions (VFs). ``/sys/bus/pci/devices/<device node>`` provides two configurable entries

- ``sriov_totalvfs`` : Indicates the maximum number of VFs supported for PF. This is a read only entry having the value the that was configured during bit stream generation.

- ``sriov_numvfs`` : Enables the user to specify the number of VFs required for a PF

Display the currently supported max VFs:

::

	[xilinx@]# cat /sys/bus/pci/devices/0000:01:00.0/sriov_totalvfs

Instantiate the required number of VFs for a PF:

::

	[xilinx@]# echo 3 > /sys/bus/pci/devices/0000:01:00.0/sriov_numvfs

Once the VFS are instantiated, required number of queues can be allocated the VF using ``qmax`` sysfs entry available in VF at
/sys/bus/pci/devices/<VF function number>/qdma/qmax


2. **Allocate the Queues to a function**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QDMA IP supports maximum of 2048 queues. By default, all functions have 0 queues assigned. 

``qmax`` configuration parameter enables the user to update the number of queues for a PF. This configuration parameter indicates "Maximum number of queues associated for the current pf".

If the queue allocation needs to be different for any PF, access the qmax sysfs entry and set the required number.

Display the current value:

::

	[xilinx@]# cat /sys/bus/pci/devices/0000:01:00.0/qdma/qmax
	0

To set 1024 as qmax for PF0:

::

	[xilinx@]# echo 1024 > /sys/bus/pci/devices/0000:01:00.0/qdma/qmax

	[xilinx@]# dmactl dev list

	qdma01000	0000:01:00.0	max QP: 1024, 0~1023
	qdma01001	0000:01:00.1	max QP: 0, -~-
	qdma01002	0000:01:00.2	max QP: 0, -~-
	qdma01003	0000:01:00.3	max QP: 0, -~-

To set 1770 as qmax for PF0, 8 as q max for PF1, PF2, PF3:

::

	[xilinx@]# echo 1770 >  /sys/bus/pci/devices/0000\:01\:00.0/qdma/qmax
	[xilinx@]# echo 8 >  /sys/bus/pci/devices/0000\:01\:00.1/qdma/qmax
	[xilinx@]# echo 8 >  /sys/bus/pci/devices/0000\:01\:00.2/qdma/qmax
	[xilinx@]# echo 8 >  /sys/bus/pci/devices/0000\:01\:00.3/qdma/qmax

	[xilinx@]# dmactl dev list

	qdma01000	0000:01:00.0	max QP: 1770, 0~1769
	qdma01001	0000:01:00.1	max QP: 8, 1770~1777
	qdma01002	0000:01:00.2	max QP: 8, 1778~1785
	qdma01003	0000:01:00.3	max QP: 8, 1786~1793


``qmax`` configuration parameter is available for virtual functions as well.


3. **Reserve the Queues for VFs**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QDMA IP supports 2048 queues and from the set of 2048 queues, use the ``qmax`` sysfs entry to allocate queues to VFs similar to PFs.


Display the current value:

::

	[xilinx@] #cat /sys/bus/pci/devices/0000:81:00.4/qdma/qmax
	0
	
To set 1024 as qmax for VF0 of PF0:

::

	[xilinx@] #echo 1024 > /sys/bus/pci/devices/0000:81:00.4/qdma/qmax


4. **Set Interrupt Ring Size**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Interrupt ring size is associated with indirect interrupt mode. 

When the mode is set to indirect interrupt mode, by default the interrupt aggregation ring size is set 0 index value.
Each interrupt aggregation ring entry size is 64 Bytes. Index size 0 refers to 4KB size, i.e 4KB/64 = 512 entries.

User can configure the interrupt ring entries in multiples of 512 hence set the ``intr_rngsz`` with multiplication factor

| 0 - INTR_RING_SZ_4KB, Accommodates 512 entries
| 1 - INTR_RING_SZ_8KB, Accommodates 1024 entries
| 2 - INTR_RING_SZ_12KB, Accommodates 1536 entries
| 3 - INTR_RING_SZ_16KB, Accommodates 2048 entries
| 4 - INTR_RING_SZ_20KB, Accommodates 2560 entries
| 5 - INTR_RING_SZ_24KB, Accommodates 3072 entries
| 6 - INTR_RING_SZ_28KB, Accommodates 3584 entries
| 7 - INTR_RING_SZ_32KB, Accommodates 4096 entries

Display the current value:

::

	[xilinx@]# cat /sys/bus/pci/devices/0000:81:00.0/qdma/intr_rngsz
	0
	
To set value 2 to intr_rngsz:

::

	[xilinx@]# echo 2 > /sys/bus/pci/devices/0000:81:00.0/qdma/intr_rngsz


5. **Set Write Back Interval**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``cmpt_intrvl`` indicated the interval at which write backs are generated for an MM or H2C Stream queue running in non-bypass mode.
User can set any of the following list of values for this configuration parameter.

| 3'h0: 4
| 3'h1: 8
| 3'h2: 16
| 3'h3: 32
| 3'h4: 64
| 3'h5: 128
| 3'h6: 256
| 3'h7: 512

Accumulation can be disabled via queue context.


Display the current value:

::

	[xilinx@]# cat /sys/bus/pci/devices/0000:81:00.0/qdma/cmpt_intrvl
	0
	
To set value 2 to cmpt_intrvl:

::

	[xilinx@]# echo 2 > /sys/bus/pci/devices/0000:81:00.0/qdma/cmpt_intrvl


Queue Management
----------------

QDMA driver comes with a command-line configuration utility called ``dmactl`` to manage the queues in the system.

 
