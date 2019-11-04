*****************************
QDMA Linux Driver Design Flow
*****************************

=====================
Driver Initialization
=====================

QDMA Linux Driver is designed to configure and control the PCI based QDMA device connected to a x86 Host system. 
It is a loadable kernel module which has three main components

- libqdma
	*libqdma* is a library which provides the APIs to manage the functions, queues and mailbox communication.
	It creates multiple threads per each available core in the x86 system to manage these entities.
	
	- **main thread**: responsible for accepting the request from clients and submit the request to HW
	- **completion status processing thread**: responsible for processing the MM and ST completions
	- **completion status pending monitor thread**: responsible for monitoring the completion status pending requests
	
	Apart from creating the threads, libqdma also initializes the framework required for handling the legacy interrupts and 
	debugfs framework to collect the debug status information during runtime.
	
- drv
	*drv* is a character device created as a wrapper around libqdma to enable the applications to perform read and write operations on the queues.
	It also manages multiple devices attached to a single Host system.
	It creates a list of physical devices attached to the Host system and manages the requests coming from 
	applications to read/write to these devices.
	
- xlnx_nl
	*xlnx_nl* module creates and registers with netlink socket interface to provide various netlink interfaces to perform device, 
	queue and function related operations
	
During the module_init, it initializes these 3 main components and creates a PCI device interface i.e. *struct pci_driver*.

::

	static struct pci_driver pci_driver = {
		.name = DRV_MODULE_NAME,
		
		.id_table = pci_ids,
		
		.probe = probe_one,
		
		.remove = remove_one,
		
	#if defined(CONFIG_PCI_IOV) && !defined(__QDMA_VF__)
		.sriov_configure = sriov_config,
	#endif
		.err_handler = &qdma_err_handler,
		
	};


Using the PCI device interface, driver lists the possible devices as per the ``pci_ids`` and provides the probe and remove handler. 
These handlers are called by each device in the list.
QDMA IP supports SRIOV and error handling functionality as well and QDMA driver enables the functions to configure SRIOV 
using ``sriov_config``  and ``qdma_err_handler`` handlers respectively.

	
.. image:: /images/qdma_mod_init.PNG
   :align: center
   
========================
Driver De-Initialization
========================

During the module_exit, all the resources created during initialization are released.

- *libqdma* deletes all the created threads
- *drv* delete the physical device list
- *xlnx_nl* unregisters the xnl_family socket registered with libnl interface

.. image:: /images/qdma_mod_exit.PNG
   :align: center
   
===========================
QDMA Driver Data Structures
===========================

QDMA Linux Driver manages the all the qdma PCI devices with matching vendor id and device id listed in ``pci_ids`` table listed in ``pci_ids.h``. 
Each physical device connected over PCI to the x86 Host system is identified by the PCI bus number.
Within each physical device, It can have multiple functions and each function is treated as a PCI device and identified by the function number.

``libqdma`` treats each function as a dma device and represents them using ``xlnx_dma_dev(xdev)``. 
During initialization, qdma driver probes each PCI function and creates the corresponding ``xdev``.
``xdev`` is a driver level software structure maintained to preserve the software level details to manage a function.
``libqdma`` also creates another data structure ``qdma_dev(qdev)`` corresponds to each function to keep track of the 
h2c and c2h descriptor lists associated with each function. ``qdma_descq(descq)`` is a data structure maintained 
for each queue in the system. ``qdev`` tracks the the list of queues associated with a function using ``descq`` and it in turn stores ``xdev``.

.. image:: /images/libqdma_data_structures.PNG
   :align: center

``xdev`` is associated with the driver level configuration called ``qdma_dev_conf`` and ``descq`` is associated with 
queue level configuration called ``qdma_queue_conf``.

The detailed containment of the data structures are captured below.

.. image:: /images/qdma_main_datastructures.PNG
   :align: center
 

=====================
Device Initialization
=====================

Each PF or VF is initialized when ``probe`` is invoked by Linux kernel upon identifying the corresponding PCI device.

#. Identifying the Master PF

	QDMA driver enables the users of the QDMA IP to choose one of the available PFs as master_pf. 
	This is a software only feature and the HW does not have any knowledge on master_pf. HW treats all the PFs as equal.

	This PF has the following special software controlled privileges: 

	- Programming the global CSR(Control and Status Registers)
	- Handling the errors reported by HW using a dedicated interrupt vector

	While inserting the QDMA Linux driver, user can pass the choice of the PF number as master_pf for a device 
	using device BDF(BBDDF- PCI Bus, Device, Function) number.

#. Enable the DMA capability in the device

	Marks all PCI regions associated with PCI device (pdev) as being reserved and enables the DMA capability of the device.

#. Mapping the PCI Bars

	QDMA IP design provides the QDMA functionality in AXI DMA Bar. It needs to have a User Bar(AXI4Lite Bar) to realize 
	the QDMA functionality and can have a optional Bypass Bar(AXI Bridge Master Bar) to achieve the bypass functionality. 
	These are 64bit bars.

	For configuring the queues available in the system, QDMA driver needs to identify the DMA Bar from the avilable 
	PCI Bars of the device and memory map them to the Host system.

	The config bar needs to be passed to the driver using module_params:

	- Multiple devices can be connected to a single Host system
	- Each device has 4 PF’s hence module parameter shall support to pass 4 different values for each PF
	- All the VF’s corresponding to a PF has the same config bar hence module parameter shall support to pass 
	
	4 different values for each PF’s VF group

	Based on the above points, ``config_bar`` module parameter is composed as array of values where each entry supports to 
	pass config bar information for one device’s PF or PF’s VF.
	
	config_bar=<bus_num><config_bar_PF0><config_bar_PF1><config_bar_PF2><config_bar_PF3>
	Ex: config_bar_pf=0x0902020202,0x0a00000000


	QDMA IP user has the flexibility to configure any of the PCI Bars as DMA Bar from Vivado GUI when creating QDMA IP design. 
	To indicate the DMA Bar QDMA IP programs 0x0 register with 0x1FD3 identifier.
	Driver can find DMA BAR by reading address 0x0 from each BAR and check for (return value & 0xFFFF0000) >> 16 == 0x1fd3 to find DMA BAR
	By default BAR#0 is considered as DMA Bar by the Driver. 
	If user does not pass the ``config_bar`` module parameter, Driver tries to accesses the PCI Bar#0 and reads the 0x0 register. 
	If it does not have the 1FD3 identifier, driver will fail to initialize the corresponding PF/VF.
	If user passes the ``config_bar`` parameter, driver validates the given bar number with HW configured bar number and 
	if they match, the bar will be memory mapped.

	Upon identifying the DMA Bar, Driver identifies the User Bar using register 
	QDMA_GLBL2_PF_VF_BARLITE_INT((0x10C for PF and 0x1018 for VF) and the remaining bar is marked as bypass bar.

#. Create xdev and add to xdev list

	xdev is a libqdma level bok keeping structure which holds the information required for each function. 
	It is created from pdev and device configuration updated according.
	Once the xdev is created, it is added to the xdev list to keep track of the multiple functions.

#. Mark the device as on line

	In this phase, The allocation of I/O ports and irqs is done via standard kernel functions. 
	First, it clears the HW context memory for all the queues if the current function is marked as a master_pf.
	If the driver is loaded in interrupt mode, interrupt allocation for the set of available vectors for the function is fulfilled.
	``qdev`` is created along with all the descriptor rings for all the queues corresponding to the current function and 
	initialized with default values.
	Global CSR registers are set with default values and mm channels are enabled. 

	If the current function is a VF, message box(mbox) is created and started to communicate with the parent PF.

.. image:: /images/probe_one.PNG
   :align: center

========================
Device De-Initialization
========================

During the driver de-initialization i.e when user performs "rmmmod", all the devices managed by the driver are de-initialized by 
invoking the "remove" handler.

During the device de-initialization, following operations are performed

- Configuration for all the queues corresponding to the device are cleared
- The character device interface created for the device is released
- PCI bars are unmapped
- Set the device to off line and removed the device from the device list

.. image:: /images/remove_one.PNG
   :align: center
   
   
Resource Management
===================

.. toctree::
   :maxdepth: 1

   qdma_resmgmt.rst

Mailbox Communication
=====================

.. toctree::
   :maxdepth: 1

   qdma_mailbox.rst
   

==============================
Configure the Queues for PF/VF
==============================

QDMA driver provides a sysfs interface to change the default distribution of queues according to user choice.
``/sys/bus/pci/devices/<pci_device_bdf>/qdma/qmax`` sysfs entry is created for each function to facilitate this user configuration.
User can set the required number of queues using this sysfs entry.

.. image:: /images/set_qmax.PNG
   :align: center

.. image:: /images/set_qmax_vfs.PNG
   :align: center

===========
Add a Queue
===========

QDMA Linux Driver exposes the ``qdma_queue_add`` API to add a queue to a function. 
``dmactl`` application provided along with QDMA driver enables
the user to add a queue. QDMA driver creates the queue handle for application and 
a character dev interface to read and write to the queue.
FMAP programming is done for a function when a first queue is added to a function. 
Once the FMAP programming is done, Global CSR(Configuration and Status Registers)
updating is freezed by the driver.

.. image:: /images/xpdev_queue_add.PNG
   :align: center
   
=============
Start a Queue
=============

QDMA Linux Driver exposes the ``qdma_queue_start`` API to start an already added queue. 
User can configure the queue using the various configuration options
provided through ``dmactl`` application. During queue start, driver creates the H2C/C2H descriptor rings, 
configures the context for the queue, configures the interrupt context
if driver is loaded in interrupt mode, attaches a thread to the queue.

.. image:: /images/xpdev_nl_queue_start.PNG
   :align: center

============
Stop a Queue
============

QDMA Linux Driver exposes the ``qdma_queue_stop`` API to stop an already started queue. 
All the queue resources are released and context is cleared.

.. image:: /images/qdma_queue_stop.PNG
   :align: center
   
==============
Delete a Queue
==============

QDMA Linux Driver exposes the ``qdma_queue_delete`` API to delete an already added queue. 
Queue handle is released and character device interface is deleted

.. image:: /images/xpdev_queue_delete.PNG
   :align: center

=======================
Read/Write from a Queue
=======================

``dma_from_device`` and ``dma_to_device`` utilities are provided with QDMA Linux driver to perform 
a read(C2H) and write(H2C) operations on a queue.

.. image:: /images/read_write_request.PNG
   :align: center
   
   
   
QDMA Debug Support
==================

.. toctree::
   :maxdepth: 1

   debugfs.rst