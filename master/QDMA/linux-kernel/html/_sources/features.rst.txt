QDMA Features
#############

QDMA Linux Driver supports the following list of features

QDMA Hardware Features
**********************

* SRIOV with 4 Physical Functions (PF) and 252 Virtual Functions (VF)
* Memory Mapped (MM) and Stream (ST) interfaces per queue
* 2048 queue sets

   * 2048 H2C (Host-to-Card) descriptor rings
   * 2048 C2H (Card-to-Host) descriptor rings
   * 2048 completion rings
* Supports MSI-X Interrupts

   * 2048 MSI-X vectors.
   * Up to 32 MSI-X per PF (Minimum 8 MSI-x Per PF)
   * Up to 8  MSI-X per VF (Minimum 8 MSI-x Per VF)
   * Interrupt Aggregation
   * User Interrupts
   * Error Interrupts

* Mailbox communication between PF and VF
* HW Error reporting
* Immediate data transfers
* Streaming H2C to C2H and C2H to H2C loopback support
* Configuring queues in internal mode or bypass mode
* Dynamic queue configuration
* Descriptor bypass(8, 16, 32 descriptor sizes) support
* Descriptor Prefetching
* Completion ring descriptors of 8, 16, 32 bytes sizes
* ECC support


Features Supported only in QDMA3.1 and QDMA4.0 Designs
======================================================

* Interrupt support for Mailbox events
* Flexible interrupt allocation between PF and VF
* Zero byte transfers
* Streaming C2H completion entry coalescing
* Disabling overflow check in completion ring
* Completion ring descriptors of 64 bytes sizes

Features Supported only in QDMA3.1 Design
=========================================

* Flexible BAR mapping for QDMA configuration register space

Features Supported only in QDMA4.0 Design
=========================================

* Debug mode, with extra registers for debug purposes
* Descriptor Engine can be configured in Internal Mode only

Features Supported only in Versal CPM5 Design
=============================================

* SRIOV with 4 Physical Functions (PF) and 240 Virtual Functions (VF)
* Debug mode, with extra registers for debug purposes
* Descriptor Engine can be configured in Internal Mode only
* Supports 4096 queue sets for Physical Functions

   * 4096 H2C (Host-to-Card) descriptor rings
   * 4096 C2H (Card-to-Host) descriptor rings
   * 4096 completion rings
* Supports 256 queue sets for Virtual Functions

   * 256 H2C (Host-to-Card) descriptor rings
   * 256 C2H (Card-to-Host) descriptor rings
   * 256 Completion Rings
* Tandem boot support

Features Supported only in Versal CPM4 Design
=============================================

* SRIOV with 4 Physical Functions (PF) and 252 Virtual Functions (VF)
* Descriptor Engine can be configured in Internal Mode only
* Supports 2048 queue sets for Physical Functions

   - 2048 H2C (Host-to-Card) descriptor rings
   - 2048 C2H (Card-to-Host) descriptor rings
   - 2048 completion rings
* Supports 256 queue sets for Virtual Functions

   - 256 H2C (Host-to-Card) descriptor rings
   - 256 C2H (Card-to-Host) descriptor rings
   - 256 Completion Rings
* Tandem boot support


For details on QDMA Hardware Features refer to QDMA_Soft_Product_Guide_.

.. _QDMA_Soft_Product_Guide: https://docs.xilinx.com/viewer/book-attachment/n6fBd_xlVtlFE96gGACfJA/gbwHDtOsU98Gh7U6BMS5rg

For details on CPM4 and CPM5 Hardware Features refer to QDMA_Hard_Product_Guide_.

.. _QDMA_Hard_Product_Guide: https://docs.xilinx.com/viewer/book-attachment/bjqy7nDIaVBBMR9rpQMFyg/aru18LAWHcbJZJmSRvXf~A


QDMA Software Features
**********************

* Polling and Interrupt Modes
	QDMA software provides 2 different drivers. PF driver for Physical functions and and VF driver for Virtual Functions.
	PF and VF drivers can be inserted in different modes.

   - Polling Mode
		In Poll Mode, Software polls for the write back completions (Status Descriptor Write Back)

   - Direct Interrupt Mode
		In Direct Interrupt mode, each queue is assigned to one of the available interrupt vectors in a round robin fashion to service the requests.
		Interrupt is raised by the HW upon receiving the completions and software reads the completion status.

   - Indirect Interrupt Mode
		In Indirect Interrupt mode or Interrupt Aggregation mode, each vector has an associated Interrupt Aggregation Ring.
		The QID and status of queues requiring service are written into the Interrupt Aggregation Ring.
		When a PCIe MSI-X interrupt is received by the Host, the software reads the Interrupt Aggregation Ring to determine which queue needs service.
		Mapping of queues to vectors is programmable

   - Auto Mode
		Auto mode is mix of Poll and Interrupt Aggregation mode. Driver polls for the write back status updates.
		Interrupt aggregation is used for processing the completion ring.

- Allows only Privileged Physical Function to program the contexts and registers
- Dynamic queue configuration, refer to Interface file, libqdma_export.h (``struct qdma_queue_conf``) for configurable parameters
- Dynamic driver configuration, refer to Interface file, libqdma_export.h (``struct qdma_dev_conf``) for configurable parameters
- Driver configuration through sysfs
- Asynchronous and Synchronous IO support
- Display the Version details for SW and HW
