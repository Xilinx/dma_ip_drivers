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

To modify the PCIe Device ID in the driver, open the ``driver/src/pci_ids.h`` file from the driver source and search for the ``pci_device_id`` struct. 
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
| apps/        		   | User space application to configure and control QDMA        |
+--------------------------+-------------------------------------------------------------+
| docs/        		   | Documentation for the Xilinx QDMA Linux Driver              |
+--------------------------+-------------------------------------------------------------+
| driver/src               | Provides interfaces to manage the PCIe device and           |
|                          | exposes character driver interface to perform QDMA transfers|
+--------------------------+-------------------------------------------------------------+
| driver/libqdma/          | QDMA library to configure and control the QDMA IP           |
+--------------------------+-------------------------------------------------------------+
| scripts/                 | Sample scripts for perform DMA operations                   |
+--------------------------+-------------------------------------------------------------+
| Makefile                 | Makefile to compile the driver                              |
+--------------------------+-------------------------------------------------------------+
| RELEASE.txt              | Release Notes                                               |
+--------------------------+-------------------------------------------------------------+


Once driver sources are downloaded, compile the QDMA Driver software

::

	[xilinx@]# make clean && make

Upon executing make, A sub-directory ``bin`` is created with the list of executable listed in below tables.
Individual executables can also be built with commands listed against each component in below tables.

Optionally to enable debug mode for more detailed logs, build with the following command 

::
	
	[xilinx@]# make clean & make DEBUG=1

To enable VF 4K queue driver support for CPM5 design, build driver with the following command

::

	[xilinx@]# make clean & make EQDMA_CPM5_VF_GT_256Q_SUPPORTED=1

To enable 10 bit tag driver support for gen5x8 performance measurements for CPM5 design, build driver with the following command

::

	[xilinx@]# make clean & make EQDMA_CPM5_10BIT_TAG_ENABLE=1


**Kernel modules:**

+-------------------+--------------------+--------------------------------+
| Executable        | Description        | Command                        |
+===================+====================+================================+
| qdma-pf.ko        | PF Driver          | make driver MODULE=mod_pf      |
+-------------------+--------------------+--------------------------------+
| qdma-vf.ko        | VF Driver          | make driver MODULE=mod_vf      |
+-------------------+--------------------+--------------------------------+


**Applications:**

Run ``make apps`` to compile all the below applications.
Applications can be compiled individually as mentioned after entering the respective application directory.

+-------------------+--------------------------------------------------+----------------------+
| Executable        | Description                                      | Command              |
+===================+==================================================+======================+
| dma-ctl           | QDMA control and configuration application       | make dma-ctl         |
+-------------------+--------------------------------------------------+----------------------+
| dma-to-device     | Performs a host-to-card transaction for MM or ST | make dma-to-device   |
+-------------------+--------------------------------------------------+----------------------+
| dma-from-device   | Performs a card-to-host transaction for MM or ST | make dma-from-device |
+-------------------+--------------------------------------------------+----------------------+
| dma-perf          | Measures the performance of QDMA IP              | make dma-perf        |
+-------------------+--------------------------------------------------+----------------------+
| dma-latency       | Measures the ping pong latency for ST transfers  | make dma-latency     |
+-------------------+--------------------------------------------------+----------------------+
| dma-xfer          | Sample transfer application  		       | make dma-xfer        |
+-------------------+--------------------------------------------------+----------------------+

Install the executable by running "make install"

-   PF/VF kernel modules and application binaries are generated in ``bin`` and can be individually loaded

-   ``make install-apps`` installs the applications in ``/user/local/sbin``.  

-   ``make install-mods`` copies the ``qdma-pf.ko`` and ``qdma-vf.ko`` in ``/lib/modules/<kernel version>/qdma/``  

Ex: ``/lib/modules/4.15.0-51-generic/qdma/``


Loading the QDMA Driver Module
------------------------------	

Before loading the QDMA driver, make sure that the target board is connected to the Host System and required bit stream is loaded on to the board. 

Make sure to install the ``qdma-pf.ko`` and ``qdma-vf.ko`` modules using the ``make install-mods`` command.

For loading the driver, execute the following command:

PF Driver:
::

	modprobe qdma-pf

VF Driver:
::

	modprobe qdma-vf


QDMA driver supports the following list of module parameters that can be specified while loading the driver into the Linux kernel.
- Mode
- Master PF
- Dynamic config BAR

In order to pass the module parameters while loading a driver, a config file ``qdma.conf`` needs to be placed in /etc/modprobe.d directory.
The format of the conf file would be :

::

	options <module_name> mode=<bus_num>:<pf_num>:<mode>,<bus_num>:<pf_num>:<mode>,<bus_num>:<pf_num>:<mode>,.....
	options <module_name> config_bar=<bus_num>:<pf_num>:<config_bar>,<bus_num>:<pf_num>:<config_bar>,<bus_num>:<pf_num>:<config_bar>,.....
	options <module_name> master_pf=<bus_num>:<master_pf>,<bus_num>:<master_pf>
	
- module_name:  Name of the mode. For PF: qdma-pf and for VF: qdma-vf
- bus_num : Bus number of the PCIe endpoint card
- func_num : Function number of the corressponding bus_num
- mode: Mode in which the driver needs to be loaded
- config_bar: Config bar number
- master_pf: Master PF  
- num_threads: number of threads for monitoring the writeback of completions	

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

By default, the mode is set to auto mode for both of the PF and VF drivers. To set other modes, the ``mode`` entry needs to be added in the ``qdma.conf`` 
file in the following format.

::

	options qdma-pf mode=<bus_num>:<pf_num>:<mode>,<bus_num>:<pf_num>:<mode>,<bus_num>:<pf_num>:<mode>,.....
	options qdma-vf mode=<bus_num>:<pf_num>:<mode>,<bus_num>:<pf_num>:<mode>,<bus_num>:<pf_num>:<mode>,.....

For example, if ``modprobe`` command is executed with the following config file:

::

	options qdma-pf mode=0x06:0:2,0x06:1:3,0x06:2:0,0x07:2:1
	options qdma-vf mode=0x06:0:2,0x06:1:3

- PF0 of bus number 6 is loaded in Direct Interrupt mode (2)
- PF1 of bus number 6 is loaded in Indirect Interrupt mode (3)
- PF2 of bus number 6 is loaded in Auto mode (0)
- PF2 of bus number 7 is loaded in Poll mode (1)
- PF0's VF group of bus number 6 is loaded in Direct Interrupt Mode (2)
- PF1's VF group of bus number 6 is loaded in Indirect Interrupt Mode (3)
- Rest all, which are not specified, are loaded in Auto mode by default.


2. **Master PF**
~~~~~~~~~~~~~~~~

``master_pf`` module parameter is used to set the master PF for QDMA driver
By default, ``master_pf`` is set to PF0(First device in the PF list)

To set other PF as a ``master_pf``, its entry needs to be added in the ``qdma.conf`` file in the following format.

::

	options qdma-pf master_pf=<bus_num>:<master_pf>,<bus_num>:<master_pf>

For example, if ``modprobe`` command is executed with the following config file:

::

	options qdma-pf master_pf=0x06:0,0x07:1

- Master PF of bus number 6 is set as PF0
- Master PF of bus number 7 is set as PF1



3. **Dynamic Config Bar**
~~~~~~~~~~~~~~~~~~~~~~~~~

``config_bar`` module parameter is used to set the DMA bar of the QDMA device. 
QDMA IP supports changing the DMA bar while creating the bit stream.

For 64-bit bars, DMA bar can be 0|2|4 .
By default, the QDMA driver sets BAR0 as the DMA BAR

To set other config bar, the ``config_bar`` entry needs to be added in the ``qdma.conf`` file in the following format.

::

	options qdma-pf config_bar=<bus_num>:<pf_num>:<config_bar>,<bus_num>:<pf_num>:<config_bar>,<bus_num>:<pf_num>:<config_bar>,.....
	options qdma-vf config_bar=<bus_num>:<pf_num>:<config_bar>,<bus_num>:<pf_num>:<config_bar>,<bus_num>:<pf_num>:<config_bar>,.....

For example, if ``modprobe`` command is executed with the following config file:

::

	options qdma-pf config_bar=0x06:0:0,0x06:1:2,0x06:2:4,0x07:2:0
	options qdma-vf config_bar=0x06:0:2,0x06:1:0

- PF0 of bus number 6 is loaded with config bar set as 0
- PF1 of bus number 6 is loaded with config bar set as 2
- PF2 of bus number 6 is loaded with config bar set as 4
- PF2 of bus number 7 is loaded with config bar set as 0
- PF0's VF group of bus number 6 is loaded with config bar set as 2
- PF1's VF group of bus number 6 is loaded with config bar set as 0
- Rest all, which are not specified, are loaded with config bar set as 0


From the examples above, a ``qdma.conf`` file would look like:

::

	options qdma-pf mode=0x06:0:2,0x06:1:3,0x06:2:0,0x07:2:1
	options qdma-pf master_pf=0x06:0,0x07:1
	options qdma-pf config_bar=0x06:0:0,0x06:1:2,0x06:2:4,0x07:2:0
	options qdma-vf mode=0x06:0:2,0x06:1:3
	options qdma-vf config_bar=0x06:0:2,0x06:1:0	

An auxillary script, qdma_generate_conf_file.sh has been added to the scripts folder which helps create the qdma.conf 
config file and copies it to the /etc/modprobe.d location. The script can be used as shown below -

::

	./scripts/qdma_generate_conf_file.sh <bus_num> <num_pfs> <mode> <config_bar> <master_pf>

Please note that having the qdma-pf.ko and qdma-vf.ko files in the /lib/modules/<kernel version>/qdma/ will cause
automatic loading of the driver modules at boot time. To avoid this, it is recommended to have the drivers 
blacklisted. This can be done by adding the below 2 lines in the /etc/modprobe.d/blacklist.conf file -

::


	blacklist qdma-pf
	blacklist qdma-vf
