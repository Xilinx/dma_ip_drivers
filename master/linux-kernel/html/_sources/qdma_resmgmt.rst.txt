******************************
QDMA Queue Resource Management
******************************

QDMA IP supports 2K queues. QDMA Resource Manager defines the strategy to allocate the queues across the available PFs and VFs.
Resource Manager maintains a global resource linked list in the driver. It creates a linked list of nodes for each PCIe device (PCIe bus) it manages.
Each device (bus) node in the Resource Manager list is initialized with queue base and number of queues to manage on that device.
Given the request for a number of queues for a PCIe function on a PCIe device, Resource Manager finds the best fit available
queue base with contiguous requested number of queues on that PCIe device.
Resource Manager manages queue base allocation for all the PCIe devices and PCIe functions registered with it by the driver.

Each device node in the global resource linked list maintains a "free list" with all the queues initially assigned to it, and a "device list" initialized to NULL.
When a PCIe function is initialized and requests the required queues, Resource Manager finds the best fit available queue base
with contiguous requested number of queues from the "free list" of the requested device node. If found, it creates a PCIe function node in the "device list"
and assigns the queue base and requested queues to this PCIe function node and removes these queues from the "free list".

Driver can request a specific queue base for a PCIe function and if the requested queue base is available,
Resource Manager honors it, else, allocates the queues at a base determined by the best fit algorithm.

Host system can have drivers in user space or kernel space to manage the devices connected.
E.g. Linux QDMA Driver is a kernel mode driver and DPDK PMD is a User Space driver.
Each driver maintains its own resource manager with queue base and total queues to manage specified for each device it manages.
Below, we demonstrate few examples on how Resource Manager is used in different combinations of drivers managing QDMA devices

Single device managed by Single driver
======================================

.. image:: /images/resmgmt_sdev_sdri.PNG
   :align: center

In this scenario, one PCIe QDMA device is connected to the Host System and at any given time either the Linux kernel driver or the user space DPDK driver is
managing the device. There is a single Resource Manager managing all the queues of the device and assigning the queues to functions based
on the request from each function.

Single device managed by multiple drivers
==========================================

.. image:: /images/resmgmt_sdev_mdri.PNG
   :align: center

In this scenario, one device is connected to the Host System. Linux kernel driver and user space DPDK driver are loaded to manage different PCIe functions of the device.
Each driver will create its own Resource Manager using a queue base and total queues to manage the functions binded to them.
Each Resource Manager has its own pool of queues and the PCIe functions get their queues resourced from this pool.

Multiple devices on single Host managed by Single driver
========================================================

.. image:: /images/resmgmt_mdev_sdri.PNG
   :align: center

In this scenario, more than one devices are connected to the Host System.
Either user space DPDK driver or Linux kernel driver is loaded to manage these devices.
Driver will instantiate Resource Manager and create one node per PCIe device conected,
resourcing each PCIe device (node) with the queue base and total number of queues to manage.
PCIe functions corresponding to each PCIe device are allocated queues from the respective device nodes.

