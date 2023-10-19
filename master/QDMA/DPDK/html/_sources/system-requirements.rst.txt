.. _sys_req:

System Requirements
===================

Xilinx Accelerator Card
-----------------------

1. VCU1525
2. U200
3. Versal CPM4 XCVC1902
4. Versal CPM5 XCVP1202

Host System
-----------

x86_64 host system with at least one Gen 3x16 PCIe slot and minimum 64GB RAM
on same CPU node for 2K queues. For accomodating 4K queues in Versal CPM5 ~256GB RAM would be required.
For VM testing, host system must support virtualization and it must be enabled in the BIOS.


Host System Configuration
-------------------------

QDMA DPDK driver latest release is verified on following host system configuration for PF and VF functionality

+--------------------------+-------------------------------------------------------------+
| Host System              | Configuration Details                                       |
+==========================+=============================================================+
| Operating System         | Ubuntu 18.04.1 LTS                                          |
+--------------------------+-------------------------------------------------------------+
| Linux Kernel             | 4.15.0-101-generic                                          |
+--------------------------+-------------------------------------------------------------+
| CPU Cores                | 32                                                          |
+--------------------------+-------------------------------------------------------------+
| RAM                      | 64GB on local NUMA node                                     |
+--------------------------+-------------------------------------------------------------+
| Hypervisor               | KVM                                                         |
+--------------------------+-------------------------------------------------------------+
| Qemu Version             | QEMU emulator version 2.11.1(Debian 1:2.11+dfsg-1ubuntu7.26)|
+--------------------------+-------------------------------------------------------------+
| Vivado Version           | 2021.1                                                      |
+--------------------------+-------------------------------------------------------------+

**Notes**: 

- When assigning the 2048 queues to PFs users shall make sure the host system configuration meets the requirement given above. If users want to enable iommu(iommu=on), make sure suffiecient Huge pages are available to support the 2048 queues.

- Versal CPM5 design supports 4096 queues. Users shall make sure the host system configuration meets the memory requirements to accomodate 4096 queues.

- QEMU is a hosted virtual machine monitor which emulates the machine's processor through dynamic binary translation and provides a set of different hardware and device models for the machine, enabling it to run a variety of guest operating systems.This is used to emulate the virtual machines and enables the users to attach virtual functions to virtual machines.

.. _guest_system_cfg:

Guest System Configuration
--------------------------

QDMA DPDK driver latest release is verified on the following guest system configuration for VF functionality

========================= ==================================
Guest System(VM)          Configuration Details
========================= ==================================
Operating System          Ubuntu 18.04 LTS
Linux Kernel              4.15.0-101-generic
RAM 					  20GB
Cores              		  10
========================= ==================================


Other Linux Distributions
-----------------------------

QDMA DPDK driver has also been validated on below other Linux distributions

+-------------------------+---------------------+---------------------------+
| Linux Distribution      | Version             | Kernel Version            |
+=========================+=====================+===========================+
| CentOS                  |8.1-1911             |4.18.0-269.el8.x86_64      |
+-------------------------+---------------------+---------------------------+
| Fedora                  |33                   |5.8.15-301.fc33.x86_64     |
+-------------------------+---------------------+---------------------------+
| Ubuntu                  |20.04                |5.4.0-48-generic           |
+-------------------------+---------------------+---------------------------+
| Ubuntu                  |22.04                |5.15.0-58-generic          |
+-------------------------+---------------------+---------------------------+