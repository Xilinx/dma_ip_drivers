========================
Xilinx QDMA Linux Driver 
========================

Xilinx QDMA Subsystem for PCIe example design is implemented on a Xilinx FPGA, 
which is connected to an X86 host system through PCI Express. 
Xilinx QDMA Linux Driver is implemented as a combination of user space and 
kernel driver components to control and configure the QDMA subsystem.

QDMA Linux Driver consists of the following four major components:

- **QDMA HW Access**: QDMA HW Access module handles all the QDMA IP register access functionality and exposes a set of APIs for register read/writes. 

- **Libqdma**: Libqdma module exposes a set of APIs for QDMA IP configuration and management. It uses the QDMA HW Access layer for interacting with the QDMA HW and facilitates the upper layers to achieve the following QDMA functionalities

	- Physical Function(PF) Management
	- Virtual Function(VF) Management
	- Communication Management between PF and VF
	- QDMA Queue Configuration and Control
	- Descriptor Rings Creation and Control
	- Transfer Management
	- DebugFS Support
	
- **Driver Interface**: This layer create a simple linux pci_driver interface and a character driver interface to demonstrate the QDMA IP functionalities using the Linux QDMA IP driver. It creates a NL interface to facilitate the user applications to interact with the ``Libqdma`` module. It also creates ``sysfs`` interface to enable users to configure and control various QDMA IP parameters.

- **Applications**: QDMA IP Driver provides the following sample applications.

	- Dmactl : This application provides set of commands to configure and control the queues in the system
	- dma_to_device : This application enables the users to perform Host to Card(H2C) transfer
	- dma_from_device : This application enables the users to perform Card to Host(C2H) transfer
	- Dmaperf : This application provides interfaces to extract performance numbers for QDMA IP in MM and ST modes


.. image:: /images/qdma_linux_driver_architecture.PNG
   :align: center

----------------------------------------------------------------------------


.. toctree::
   :maxdepth: 1
   :caption: Table of Contents

   features.rst
   system-requirements.rst
   build.rst
   userguide.rst
   user-app.rst
   devguide.rst
   performance.rst
	