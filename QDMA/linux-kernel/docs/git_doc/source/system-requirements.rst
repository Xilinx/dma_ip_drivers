.. _sys_req:

System Requirements
===================

Xilinx Accelerator Card
-----------------------

1. VCU1525
2. TULVU9P

Host Platform
-------------

x86_64 host system with at least one Gen 3x16 PCIe slot and minimum 32GB RAM
on same CPU node for 2K queues.
For VM testing, host system must support virtualization and it must be enabled in the BIOS.


Host System Configuration
-------------------------

Linux QDMA Driver release is verified on following Host system configuration for PF and VF functionality

+--------------------------+-------------------------------------------------------------+
| Host System              | Configuration Details                                       |
+==========================+=============================================================+
| Operating System         | Ubuntu 18.04 LTS                                            |
+--------------------------+-------------------------------------------------------------+
| Linux Kernel             | 4.15.0-23-generic                                           |
+--------------------------+-------------------------------------------------------------+
| RAM                      | 64GB on local NUMA node                                     |
+--------------------------+-------------------------------------------------------------+
| Hypervisor               | KVM                                                         |
+--------------------------+-------------------------------------------------------------+
| Qemu Version             | QEMU emulator version 2.5.0 (Debian 1:2.5+dfsg-5ubuntu10.15)|
+--------------------------+-------------------------------------------------------------+

Note: QEMU is a hosted virtual machine monitor which emulates the machine's processor through dynamic binary translation and provides a set of different hardware and device models for the machine, enabling it to run a variety of guest operating systems.This is used to emulate the virtual machines and enables the users to attach virtual functions to virtual machines.

Guest System Configuration
--------------------------

Linux QDMA VF Driver release is verified on following Guest system configuration for VF functionality

========================= ==================================
Guest System(VM)          Configuration Details             
========================= ==================================
Operating System          Ubuntu 18.04 LTS
Linux Kernel              4.15.1-20-generic
RAM 			  4GB
Cores              	  4
========================= ==================================


Supported Linux Distributions
-----------------------------

Linux QDMA Driver is supported on the following linux distributions


+-------------------------+---------------------+----------------+
| Linux Distribution      | Version             | Kernel Version |          
+=========================+=====================+================+
| CentOS                  |7.4-1708             |3.10.0-693      |
|                         +---------------------+----------------+
|                         |7.5-1804             |3.10.0-862      |
|                         +---------------------+----------------+
|                         |7.6-1810             |3.10.0-957      |
+-------------------------+---------------------+----------------+
|Fedora                   |28                   |4.16            |
|                         +---------------------+----------------+
|                         |29                   |4.18            |
|                         +---------------------+----------------+
|                         |30                   |5.0             |
+-------------------------+---------------------+----------------+
|Ubuntu                   |14.04.06             |4.4.0-93        |
|                         +---------------------+----------------+
|                         |16.04                |4.10            |
|                         +---------------------+----------------+
|                         |18.04                |4.15.0-23       |
|                         +---------------------+----------------+
|                         |18.04.2              |3.10.0          |
|                         +---------------------+----------------+
|                         |18.04.2              |3.10.0          |
+-------------------------+---------------------+----------------+


Supported Kernel Versions
-------------------------

Linux QDMA Driver is verified on following kernel.org Linux kernel versions

+-------------------------+-----------------+
|Kernel.org               | Kernel Version  |
+=========================+=================+
|                         | 3.16.66         |
|                         +-----------------+
|                         | 4.4.179         |
|                         +-----------------+
|                         | 4.19.41         |
|                         +-----------------+
|                         | 4.14.117        |
|                         +-----------------+
|                         | 4.9.174         |
|                         +-----------------+
|                         | 5.0.14          |
+-------------------------+-----------------+

The following kernel functions shall be included in the OS kernel version being used. Make sure that these functions are included in the kernel.

- Timer Functions 
- PCIe Functions 
- Kernel Memory functions
- Memory and GFP Functions

