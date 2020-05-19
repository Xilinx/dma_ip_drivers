Building and Installing Software Stack
======================================

For building the Linux QDMA Driver, make sure the :ref:`sys_req` are satisfied.

Updating the PCIe device ID
---------------------------

During the PCIe DMA IP customization in Vivado you can specify a PCIe Device ID. 
This Device ID must be recognized by the driver in order to properly identify the PCIe QDMA device. 
The current driver is designed to recognize the default PCIe Device IDs that get generated with the PCIe example design. 
If the PCIe Device ID is modified during IP customization, one needs to modify QDMA driver to recognize this new ID.

User can also remove PCIe Device IDs that are not be used in their solution.

To modify the PCIe Device ID in the driver, open the ``drv/pci_ids.h`` file from the driver source and search for the pcie_device_id struct. 
This struct defines the PCIe Device IDs that are recognized by the driver in the following format: 

{PCI_DEVICE (0x10ee, 0x9034),}, 

Add, remove, or modify the PCIe Device IDs in this struct.
The PCIe DMA driver will only recognize device IDs identified in this struct as PCIe QDMA devices. 
Once modified, the driver must be un-installed and recompiled.

Building the QDMA Driver Software
---------------------------------

This driver software supports both Physical Functions (PF) and Virtual Functions (VF).

In order to compile the Xilinx QDMA software, configured and compiled Linux kernel source tree is required. 
Linux QDMA driver is dependent on ``libaio``. Hence, make sure to install ``libaio`` before compiling the QDMA driver.
Example command is provided on Ubuntu. Follow the similar procedure for other OS flavors.

::

	[xilinx@]# sudo apt-get install libaio libaio-devel
	

The source tree may contain only header files, or a complete tree. The source tree needs to be configured and the header files need to be compiled.
The Linux kernel must be configured to use modules.

Linux QDMA Driver software can be found on the Xilinx github https://github.com/Xilinx/dma_ip_drivers/tree/master/QDMA/linux-kernel
Below is the directory structure of the Linux QDMA Driver software.

+--------------------------+-------------------------------------------------------------+
| **Directory**            | **Description**                                             |
+==========================+=============================================================+
| docs/        		   | Documentation for the Xilinx QDMA Linux Driver              |
+--------------------------+-------------------------------------------------------------+
| drv/                     | Provides interfaces to manage the PCIe device and           |
|                          | exposes character driver interface to perform QDMA transfers|
+--------------------------+-------------------------------------------------------------+
| libqdma/                 | QDMA library to configure and control the QDMA IP           |
+--------------------------+-------------------------------------------------------------+
| scripts/                 | Sample scripts for perform DMA operations                   |
+--------------------------+-------------------------------------------------------------+
| user/                    | User space application to configure and control QDMA        |
+--------------------------+-------------------------------------------------------------+
| tools/                   | Tools to perform DMA operations                             |
+--------------------------+-------------------------------------------------------------+
| Makefile                 | Makefile to compile the driver                              |
+--------------------------+-------------------------------------------------------------+
| RELEASE.txt              | Release Notes                                               |
+--------------------------+-------------------------------------------------------------+


Once driver sources are downloaded, compile the QDMA Driver software

::

	[xilinx@]# make clean && make

Upon executing make, A sub-directory ``build`` is created in ``linux-kernel`` with the list of executable listed in below tables.
Individual executables can also be built with commands listed against each component in below tables.

**Kernel modules:**

+-------------------+--------------------+---------------+
| Executable        | Description        | Command       |
+===================+====================+===============+
| qdma.ko           | PF Driver          | make pf       |
+-------------------+--------------------+---------------+
| qdma_vf.ko        | VF Driver          | make vf       |
+-------------------+--------------------+---------------+


**Applications:**

+-------------------+--------------------------------------------------+--------------+
| Executable        | Description                                      | Command      |
+===================+==================================================+==============+
| dmactl            | QDMA control and configuration application       | make user    |
+-------------------+--------------------------------------------------+--------------+
| dma_to_device     | Performs a host-to-card transaction for MM or ST | make tools   |
+-------------------+--------------------------------------------------+--------------+
| dma_from_device   | Performs a card-to-host transaction for MM or ST | make tools   |
+-------------------+--------------------------------------------------+--------------+
| dmaperf           | Measures the performance of QDMA IP              | make tools   |
+-------------------+--------------------------------------------------+--------------+


Install the executable by running "make install"

-   The ``dmactl``, ``dma_from_device`` and ``dma_to_device`` tools will be installed in ``/user/local/sbin``.  

-   Individual components can be installed by using the appropriate install target. 

Ex: "make install-user" will install all user applications

-   PF/VF kernel modules are generated in ``build`` and can be individually loaded as below

::

	[xilinx@]# insmod build/qdma.ko 
	[xilinx@]# insmod build/qdma_vf.ko

Loading the QDMA Driver Module
------------------------------	

Before loading the QDMA driver, make sure that the target board is connected to the Host System and required bit stream is loaded on to the board.

QDMA driver supports the following list of module parameters that can be specified while loading the driver into the Linux kernel.
- Mode
- Master PF
- Dynamic config BAR


1. **Mode**
~~~~~~~~~~~

``mode`` module parameter is used to specify how the completions must be processed.
Each PF can be configured to function in following different modes

0. *Auto Mode*

    Driver polls on the status descriptor write back updates for MM and ST H2C Mode and uses Interrupt Aggregation to process the ST C2H completions.

1. *Poll Mode*

    Driver polls on the status descriptor write back updates for all modes.
	
2. *Direct Interrupt Mode*

    A single vector is assigned to each queue and status descriptor write back updates are intimated to SW driver using interrupts.
	
3. *Interrupt Aggregation Mode* or *Indirect Interrupt Mode*

    Each function creates an Interrupt Aggregation ring and status descriptor write back updates of all the queues of the function are intimated to SW driver using interrupts into this ring.

4. *Legacy Interrupt Mode*

    Driver processes the status descriptor write back using legacy interrupts

By default, the mode is set to auto mode for both of the PF and VF drivers. To set other modes, use the ``mode`` module parameter in the following format.

``mode`` takes the input as an array of 32 bit numbers and enables the user to specify the mode for multiple devices connected to the host system.

ex: 0x00011111, 0x00012222, 0x00013333

Each 32bit number is divided as below for PF driver.

.. image:: /images/pf_modes.PNG
   :align: center


Use the below command to load the PF driver and set the modes for different PFs.

::

	[xilinx@]# insmod qdma.ko mode=0x<device_bus_number><PF0_mode><PF1_mode><PF2_mode><PF3_mode>


Each 32 bit number is divided as below for VF driver where all VFs corresponding to PF0 is named as VFG0, the mode of all VFs corresponding to PF1 is named as VFG1 and so on ...

.. image:: /images/vf_modes.PNG
   :align: center

Use the below command to load the VF driver and set the modes for different VFs

::

	
	[xilinx@]# insmod qdma_vf.ko mode=0x<device_bus_number><VFG0_mode><VFG1_mode><VFG2_mode><VFG3_mode>


Refer to the example below:

::

	[xilinx@]# lspci | grep Xilinx
	01:00.1 Memory controller: Xilinx Corporation Device 913f


To set the mode as poll mode for all PFs, use the below command.

Ex: insmod qdma.ko mode=0x011111

To set themode as Interrupt Aggregation Mode for all PFs, use the below command.

Ex: insmod qdma.ko mode=0x013333


To load the PF driver with PF0 in Auto mode, PF1 in poll mode, PF2 in Direct Interrupt mode and PF3 in Interrupt Aggregation Mode, use the below command.

Ex: insmod qdma.ko mode=0x010123

When multiple cards are connected to the same host system and mode needs to be set for each card, use the command as below.

::

	[xilinx@]# lspci | grep Xilinx
	01:00.1 Memory controller: Xilinx Corporation Device 913f
	02:00.1 Memory controller: Xilinx Corporation Device 913f


Ex: insmod qdma.ko mode=0x011111,0x023333

Follow the same for VF driver by appropriately choosing each VFG mode. 

2. **Master PF**
~~~~~~~~~~~~~~~~

``master_pf`` module parameter is used to set the master PF for QDMA driver
By default, ``master_pf`` is set to PF0(First device in the PF list)


master_pf takes the input as an array of 32 bit numbers and enables the user to mention the master_pf for multiple cards connected to the host system by comma seperated values.

ex: 0x00010001, 0x00020002, 0x00030003

Each 32 bit number is divided as below for PF driver.

.. image:: /images/master_pf.PNG
   :align: center

::


	[xilinx@]# insmod qdma.ko master_pf=<device_bus_number><device_master_pf_number>

::

	[xilinx@]# lspci | grep Xilinx
	01:00.1 Memory controller: Xilinx Corporation Device 913f


Ex: insmod qdma.ko master_pf=0x010001

To set any other PF as master_pf, use the module parameter as below

Ex: insmod qdma.ko master_pf=0x010003

When multiple cards are connected to the same host system and master_pf needs to be updated for each card, use the command as below.

::

	[xilinx@]# lspci | grep Xilinx
	01:00.1 Memory controller: Xilinx Corporation Device 913f
	02:00.1 Memory controller: Xilinx Corporation Device 913f


Ex: insmod qdma.ko master_pf=0x010001, 0x020001

3. **Dynamic Config Bar**
~~~~~~~~~~~~~~~~~~~~~~~~~

``config_bar`` module parameter is used to set the DMA bar of the QDMA device. 
QDMA IP supports changing the DMA bar while creating the bit stream.

For 64-bit bars, DMA bar can be 0|2|4 .
By default, the QDMA driver sets BAR0 as the DMA BAR if the ``config_bar`` module parameter is not set. 

If BAR2 or BAR4 is configured as DMA BAR, pass the config_bar as a module number by mentioning the BAR number

config_bar takes the input as an array of 32 bit numbers and enables the user to mention the config_bar for multiple cards connected to the host system.

ex: 0x00010000, 0x00022222, 0x00034444

Each 32 bit number is divided as below for PF driver.

.. image:: /images/pf_configbar.PNG
   :align: center

Each 32bit number is divided as below for VF driver.

.. image:: /images/vf_configbar.PNG
   :align: center
   
Ex: Let uss assume the host system has a single card connected and PF0 has BAR2 as config_bar, PF1 has the BAR0 as config_bar, PF2 has BAR4 as config_bar and PF3 has BAR0 as config_bar.

::

	[xilinx@]# lspci | grep Xilinx
	01:00.1 Memory controller: Xilinx Corporation Device 913f


::


	[xilinx@]# insmod qdma.ko config_bar=0x00012040


When multiple cards are connected to the same host system and config_bar needs to be updated for each device, use the command as below.

::

	[xilinx@]# lspci | grep Xilinx
	01:00.1 Memory controller: Xilinx Corporation Device 913f
	02:00.1 Memory controller: Xilinx Corporation Device 913f


Ex: Let us assume the host system has two cards connected 

- device#1 : PF0 has BAR2 as config_bar, PF1 has BAR0 as config_bar, PF2 has BAR4 as config_bar and PF3 has BAR0 as config_bar

- device#2 : PF0 has BAR4 as config_bar, PF1 has BAR2 as config_bar, PF2 has BAR0 as config_bar and PF3 has BAR2 as config_bar

Ex: insmod qdma.ko config_bar=0x00012040,0x00024202

