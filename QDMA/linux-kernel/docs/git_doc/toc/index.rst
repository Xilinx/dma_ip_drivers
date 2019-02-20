========================
Xilinx QDMA Linux Driver 
========================

Xilinx QDMA Subsystem for PCIe example design is implemented on a Xilinx FPGA, 
which is connected to an X86 host system through PCI Express. 
Xilinx QDMA Linux Driver is implemented as a combination of userspace and 
kernel driver components to control and configure the QDMA subsystem.

QDMA Linux Driver consists of the following three major components:

- **Device Control Tool**: Creates a netlink socket for PCIe device query, queue management, reading the context of queue etc.
- **DMA Tool**: User space application to initiate a DMA transaction.Standard Linux Applications like dd, aio or fio can be used to perform the DMA transctions.
- **Kernel Space Driver**: Creates the descriptors and translates the user space function into low-level command to interact with the FPGA device.


.. image:: /images/qdma_linux_driver_architecture.PNG
   :align: center

----------------------------------------------------------------------------


.. toctree::
   :maxdepth: 1
   :caption: Table of Contents

   features.rst
   system-requirements.rst
   build.rst
   devguide.rst
   userguide.rst
   user-app.rst
   performance.rst
	