QDMA Features
=============

QDMA DPDK Driver supports the following list of features

Hardware Features
----------------------
* SRIOV with 4 Physical Functions and 252 Virtual Functions
* Memory Mapped (MM) and Stream (ST) interfaces per queue
* 2048 queues sets

   - 2048 H2C (host-to-card) Descriptor rings
   - 2048 C2H (card-to-host) Descriptor rings
   - 2048 Completion rings

* Mailbox communication between PF and VF driver
* Interrupt support for Mailbox events
* HW Error reporting
* Zero byte transfers
* Immediate data transfers
* Descriptor bypass (8, 16, 32, 64 descriptor sizes) support
* Descriptor Prefetching
* Streaming C2H completion entry coalescing
* Disabling overflow check in completion ring
* Streaming H2C to C2H and C2H to H2C loopback support
* Dynamic queue configuration
* Completion ring descriptors of 8, 16, 32, 64 bytes sizes
* ECC support

Features Supported for QDMA3.1
--------------------------------------
* Flexible BAR mapping for QDMA configuration register space
* MM completions

Features Supported for QDMA4.0
--------------------------------------
* Support for more than 256 functions
* Support multiple bus number on single card

Features Supported only in Versal CPM5 Design
-------------------------------------------------
* SRIOV with 4 Physical Functions (PF) and 240 Virtual Functions (VF)
* Debug mode, with extra registers for debug purposes
* Descriptor Engine can be configured in Internal Mode only
* Supports 4096 queue sets for Physical Functions

   - 4096 H2C (Host-to-Card) descriptor rings
   - 4096 C2H (Card-to-Host) descriptor rings
   - 4096 completion rings
* Supports 256 queue sets for Virtual Functions

   - 256 H2C (Host-to-Card) descriptor rings
   - 256 C2H (Card-to-Host) descriptor rings
   - 256 Completion Rings

For details on Hardware Features refer to QDMA_Product_Guide_.

.. _QDMA_Product_Guide: https://www.xilinx.com/support/documentation/ip_documentation/qdma/v4_0/pg302-qdma.pdf

Software Features
---------------------

* Support DPDK v22.11 LTS
* Support DPDK v21.11 LTS
* Support DPDK v20.11 LTS
* Support driver binding to igb_uio and vfio-pci
* Support device arguments (module parameters) for device level configuration
* Allows only Privileged Physical Functions to program the contexts and configuration registers
* Device and queue configuration through additional driver APIs
* Interoperability between Linux driver (as PF/VF) and DPDK driver (as PF/VF)
* SW versioning
* Added support for detailed register dump
* Added support for post processing HW error messages
* Added support for Debug mode and Internal only mode
