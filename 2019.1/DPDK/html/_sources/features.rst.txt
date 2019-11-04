QDMA Features
=============

QDMA DPDK Driver supports the following list of features

Hardware Features
-----------------
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
* Flexible BAR mapping for QDMA configuration register space
* ECC support
* Completions in Memory mapped mode

For details on Hardware Features refer to QDMA_Product_Guide_.

.. _QDMA_Product_Guide: https://www.xilinx.com/support/documentation/ip_documentation/qdma/v3_0/pg302-qdma.pdf

Software Features
-----------------

* Support DPDK v18.11 LTS
* Support driver binding to igb_uio and vfio-pci
* Support device arguments (module parameters) for device level configuration
* Allows only Privileged Physical Functions to program the contexts and configuration registers
* Device configuration through additional driver APIs
* Interoperability between Linux driver (as PF/VF) and DPDK driver (as PF/VF)
* SW versioning
