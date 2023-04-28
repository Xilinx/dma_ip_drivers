.. _sys_req:

System Requirements
===================

Xilinx Accelerator Card
-----------------------

1. VCU1525
2. U200
3. Versal CPM5 XCVP1202

Host Platform
-------------

x86_64 host system with at least one Gen 3x16 PCIe slot and minimum 64GB RAM
on same CPU node for 2K queues. For accomodating 4K queues in Versal CPM5 ~256GB RAM would be required.
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

**Notes**: 

- When assigning the 2048 queues to PFs users shall make sure the host system configuration meets the requirement given above. If users want to enable iommu(iommu=on), make sure suffiecient memory available to support the 2048 queues. If the memory is not sufficient, ``dma_alloc_coherent`` kernel call may fail and driver might not work.

- Versal CPM5 design supports 4096 queues. Users shall make sure the host system configuration meets the memory requirements to accomodate 4096 queues.

- QEMU is a hosted virtual machine monitor which emulates the machine's processor through dynamic binary translation and provides a set of different hardware and device models for the machine, enabling it to run a variety of guest operating systems.This is used to emulate the virtual machines and enables the users to attach virtual functions to virtual machines.

Guest System Configuration
--------------------------

Linux QDMA VF Driver release is verified on following Guest system configuration for VF functionality

========================= ==================================
Guest System(VM)          Configuration Details             
========================= ==================================
Operating System          Ubuntu 18.04 LTS
Linux Kernel              4.15.1-20-generic
RAM 			          4GB
Cores              	      4
========================= ==================================


Supported Linux Distributions
-----------------------------

Linux QDMA Driver is supported on the following linux distributions


+-------------------------+---------------------+----------------+
| Linux Distribution      | Version             | Kernel Version |          
+=========================+=====================+================+
| CentOS                  |7.6-1810 (RHEL 7.6)  |3.10.0-957      |
|                         +---------------------+----------------+
|                         |7.7-1908 (RHEL 7.7)  |3.10.0-1062     |
|                         +---------------------+----------------+
|                         |8.1-1911 (RHEL 8.1)  |4.18.0-348      |
|                         +---------------------+----------------+
|                         |8.3-2011 (RHEL 8.3)  |4.18.0-348      |
|                         +---------------------+----------------+
|                         |9                    |5.14.0-283      |
+-------------------------+---------------------+----------------+
|Fedora                   |30                   |5.0.9-301       |
|                         +---------------------+----------------+
|                         |31                   |5.3.7-301       |
|                         +---------------------+----------------+
|                         |33                   |5.8.15-301      |
|                         +---------------------+----------------+
|                         |36                   |5.17.5-300      |
|                         +---------------------+----------------+
|                         |37                   |6.2.8-200       |
+-------------------------+---------------------+----------------+
|Ubuntu                   |16.04                |4.15.0-101      |
|                         +---------------------+----------------+
|                         |18.04                |4.15.0-20       |
|                         +---------------------+----------------+
|                         |20.04                |5.4.0-31        |
|                         +---------------------+----------------+
|                         |22.04                |5.15.0-41       |
+-------------------------+---------------------+----------------+


Supported Kernel Versions
-------------------------

Linux QDMA Driver is verified on following kernel.org Linux kernel versions

+-------------------------+-----------------+
|Kernel.org               | Kernel Version  |
+=========================+=================+
|                         | 3.16.66         |
|                         +-----------------+
|                         | 4.4.196         |
|                         +-----------------+
|                         | 4.9.174         |
|                         +-----------------+
|                         | 4.14.117        |
|                         +-----------------+
|                         | 5.4             |
|                         +-----------------+
|                         | 5.13            |
|                         +-----------------+
|                         | 5.17            |
+-------------------------+-----------------+

The following kernel functions shall be included in the OS kernel version being used. Make sure that these functions are included in the kernel.

- Timer Functions 
- PCIe Functions 
- Kernel Memory functions
- Memory and GFP Functions

Notes about GCC versions
-------------------------

**Issue#1**: For Kernel Images > 4.9.199 with CONFIG_STACK_VALIDATION=y and GCC version > 8 compiler throws spurious warnings
related to sibling calls and frame pointer save/setup.

**Solution#1**: To suppress these warnings
enable the OBJECT_FILES_NON_STANDARD option in linux/drv/Makefile

**Issue#2**: Compilation failure on GCC 9 

**Solution#2**: GCC 9 release adds the -Wmissing-attributes warnings (enabled by -Wall), which trigger for all the initialization/cleanup_module
aliases in the kernel (defined by the module_init/exit macros), ending up being very noisy.

These aliases point to the __init/__exit functions of a module, which are defined as __cold (among other attributes). However, if
the aliases themselves do not have the __cold attribute, this causes compilation failure for driver if used with Kernel Versions < 3.16.66 mentioned
in the supported kernels list above or if the Kernel Source doesn't have the fix for this which are there in the long term releases.


