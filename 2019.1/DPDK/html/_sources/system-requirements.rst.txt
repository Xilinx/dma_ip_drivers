.. _sys_req:

System Requirements
===================

Xilinx Accelerator Card
-----------------------

1. VCU1525
2. U200

Host System
-----------

x86_64 host system with at least one Gen 3 x16 PCIe slot and minimum 32GB RAM
on same CPU node for 2K queues.
For VM testing, host system must support virtualization and it must be enabled in the BIOS.


Host System Configuration
-------------------------

QDMA DPDK driver latest release is verified on following host system configuration for PF and VF functionality

+--------------------------+-------------------------------------------------------------+
| Host System              | Configuration Details                                       |
+==========================+=============================================================+
| Operating System         | Ubuntu 16.04.3 LTS, Ubuntu 18.04.1 LTS                      |
+--------------------------+-------------------------------------------------------------+
| Linux Kernel             | 4.4.0-93-generic                                            |
+--------------------------+-------------------------------------------------------------+
| CPU Cores                | 32                                                          |
+--------------------------+-------------------------------------------------------------+
| RAM                      | 64GB on local NUMA node                                     |
+--------------------------+-------------------------------------------------------------+
| Hypervisor               | KVM                                                         |
+--------------------------+-------------------------------------------------------------+
| Qemu Version             | QEMU emulator version 2.5.0 (Debian 1:2.5+dfsg-5ubuntu10.15)|
+--------------------------+-------------------------------------------------------------+
| Vivado Version           | 2019.1                                                      |
+--------------------------+-------------------------------------------------------------+

.. _guest_system_cfg:

Guest System Configuration
--------------------------

QDMA DPDK driver latest release is verified on the following guest system configuration for VF functionality

========================= ==================================
Guest System(VM)          Configuration Details
========================= ==================================
Operating System          Ubuntu 18.04 LTS
Linux Kernel              4.15.1-20-generic
RAM 					  4GB
Cores              		  4
========================= ==================================


