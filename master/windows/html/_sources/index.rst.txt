==========================
Xilinx QDMA Windows Driver 
==========================

Xilinx QDMA Subsystem for PCIe example design is implemented on a Xilinx FPGA, 
which is connected to an X64 host system through PCI Express. 
Xilinx QDMA Windows Driver package consists of user space applications and 
kernel driver components to control and configure the QDMA subsystem.

QDMA Windows Driver consists of the following four major components:

- **QDMA HW Access**: QDMA HW Access module handles all the QDMA IP register access functionality and exposes a set of APIs for register read/writes. 

- **Libqdma**: Libqdma module exposes a set of APIs for QDMA IP configuration and management. It uses the QDMA HW Access layer for interacting with the QDMA HW and facilitates the upper layers to achieve the following QDMA functionalities

	- Physical Function(PF) Management
	- QDMA Queue Configuration and Control
	- Descriptor Rings Creation and Control
	- Transfer Management

- **Driver Interface**: This layer create a Windows wdf device \( WDFDEVICE \) object and enables corresponding driver interface to demonstrate the QDMA IP functionalities using the Windows QDMA IP driver.

- **Applications**: QDMA IP Driver provides the following sample applications.

	- dma-ctl : This application provides set of commands to configure and control the queues in the system
	- dma-rw  : This application enables the users to perform synchronous read and write operations
	- dma-arw : This application enables the users to perform asynchronous read and write operations

----------------------------------------------------------------------------


.. toctree::
   :maxdepth: 1
   :caption: Table of Contents

   features.rst
   system-requirements.rst
   build.rst
   user-app.rst
   devguide.rst