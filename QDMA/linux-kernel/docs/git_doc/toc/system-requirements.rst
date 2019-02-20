.. _sys_req:

System Requirements
===================

Xilinx Accelerator Card
-----------------------

1. VCU1525
2. TULVU9P

Host Platform
-------------

1. x86_64


Host System Configuration
-------------------------

Linux QDMA Driver latest release is verified on following Host system configuration for PF anf VF functionality

+--------------------------+-------------------------------------------------------------+
| Host System              | Configuration Details                                       |
+==========================+=============================================================+
| Operating System         | Ubuntu 16.04.3 LTS                                          |
+--------------------------+-------------------------------------------------------------+
| Linux Kernel             | 4.4.0-93-generic                                            |
+--------------------------+-------------------------------------------------------------+
| RAM                      | 32GB                                                        |
+--------------------------+-------------------------------------------------------------+
| Qemu Version             | QEMU emulator version 2.5.0 (Debian 1:2.5+dfsg-5ubuntu10.15)|
+--------------------------+-------------------------------------------------------------+


Guest System Configuration
--------------------------

Linux QDMA VF Driver latest release is verified on following Host system configuration for VF functionality

========================= ==================================
Guest System(VM)          Configuration Details             
========================= ==================================
Operating System          Ubuntu 18.04 LTS
Linux Kernel              4.15.1-20-generic
RAM 					  4GB
Cores              		  4
========================= ==================================


Supported OS List
------------------

Linux QDMA Driver also supported on following OS and kernel versions


+-------------------------+-------------+----------------+
| Operating System        | OS Version  | Kernel Version |          
+=========================+=============+================+
| CentOS                  | 7.2.1511    |                |
+-------------------------+-------------+----------------+
|Fedora                   |24           |4.5.5-300       |
|                         +-------------+----------------+
|                         |26           |4.11.8-300      |
|                         +-------------+----------------+
|                         |27           |4.13.9-300      |
|                         +-------------+----------------+
|                         |28           |4.16.3-301      |
+-------------------------+-------------+----------------+
|Ubuntu                   |16.04        |4.4.0-93        |
|                         +-------------+----------------+
|                         |17.10.1      |4.13.0-21       |
|                         +-------------+----------------+
|                         |18.04        |4.15.0-23       |
|                         +-------------+----------------+
|                         |14.04.5      |3.10.0          |
+-------------------------+-------------+----------------+


Supported Kernel.org Version List
---------------------------------

Linux QDMA Driver verified on following kernel.org versions

+-------------------------+-----------------+
|Kernel.org               | Kernel Version  |
+=========================+=================+
|                         | 3.2.101         |
|                         +-----------------+
|                         | 3.18.108        |
|                         +-----------------+
|                         | 4.4.131         |
|                         +-----------------+
|                         | 4.14.40         |
|                         +-----------------+
|                         | 4.15.18         |
+-------------------------+-----------------+

The following kernel functions shall be included in the OS kernel being used. Make sure that these functions are included in the kernel.

- Timer Functions 
- PCIe Functions 
- Kernel Memory functions
- Kernel threads
- Memory and GFP Functions

