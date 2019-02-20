QDMA Features
#############

QDMA Linux Driver supports the following list of features

QDMA Hardware Features
**********************

* SRIOV with 4 Physical Functions(PF) and 252 Virtual Functions(VF)
* AXI4 Memory Mapped(MM) and AXI4-Stream(ST) interfaces per queue
* 2048 queue sets

   * 2048 H2C(Host-to-Card) descriptor rings
   * 2048 C2H (Card-to-Host) descriptor rings
   * 2048 completion rings

* Supports Legacy and MSI-X Interrupts

   - MSI-X Interrupts
   
	   - 2048 MSI-X vectors.
	   - Up to 8 MSI-X per function.
	   - Interrupt Aggregation
	   - User Interrupts
	   - Error Interrupts
	   
   - Legacy Interupts
   
		- Supported only for PF0 with single queue
   
* Flexible interrupt allocation between PF/VF
		
- Descriptor Prefetching
  In ST C2H, HW supports to prefetch a set of descriptors prior to the availability of the data on the Stream Engine.
  As the descriptors are prefetched in advance, it improves the performance.
  
* Different Queue Modes 

	A queue can be configured to operate in Descriptor Bypass mode or Internal Mode by setting the software context bypass field.

	- Internal Mode: 
		In Internal Mode, descriptors that are fetched by the descriptor engine are delivered directly to the appropriate DMA engine and processed. 

	- Bypass Mode: 
		In Bypass Mode, descriptors fetched by the descriptor engine are delivered to user logic through the descriptor bypass interface.
		This allows user logic to pre-process or store the descriptors, if desired.
		On the bypass out interface, the descriptors can be a custom format (adhering to the descriptor size). 
		To perform DMA operations, the user logic drives descriptors into the descriptor bypass in interface.
		
		For configurating the queue in different modes, user shall pass the appropriate configuration options as described below.

		+--------------------------+-----------------------+----------------------------+----------------------------+
		| SW_Desc_Ctxt.bypass[50]  | Pftch_ctxt.bypass[0]  | Pftch_ctxt.pftch_en[27]    | Description                |
		+==========================+=======================+============================+============================+
		|          1               |        1              |              0             | Simple Bypass Mode         |
		+--------------------------+-----------------------+----------------------------+----------------------------+
		|          1               |        1              |              1             | Simple Bypass Mode         |
		+--------------------------+-----------------------+----------------------------+----------------------------+
		|          1               |        0              |              0             | Cache Bypass Mode          |
		+--------------------------+-----------------------+----------------------------+----------------------------+
		|          1               |        0              |              1             | Cache Bypass Mode          |
		+--------------------------+-----------------------+----------------------------+----------------------------+
		|          0               |        0              |              0             | Cache Internal Mode        |
		+--------------------------+-----------------------+----------------------------+----------------------------+
		|          0               |        0              |              1             | Cache Internal Mode        |
		+--------------------------+-----------------------+----------------------------+----------------------------+


		- Simple Bypass Mode: 
			In Simple Bypass Mode, the engine does not track anything for the queue, and the user logic
			can define its own method to receive descriptors. The user logic is then responsible for
			delivering the packet and associated descriptor in simple bypass interface.

		- Cached Bypass Mode: 
			In Cached Bypass Mode, the PFCH module offers storage for up to
			512 descriptors and these descriptors can be used by up to 64 different queues. In this mode,
			the engine controls the descriptors to be fetched by managing the C2H descriptor queue
			credit on demand, based on received packets in the pipeline.
	
- Mailbox communication between PF and VF driver

- Zero byte transfers
	In Stream H2C direction, The descriptors can have no buffer and the length field in a descriptor can be zero. 
	In this case, the H2C Stream Engine will issue a zero byte read request on PCIe. 
	After the QDMA receives the completion for the request, the H2C Stream Engine will send out one beat of data 
	with tlast on the QDMA H2C AXI Stream interface. 
	The user logic must set both the SOP and EOP for a zero byte descriptor. 
	If not done, an error will be flagged by the H2C Stream Engine.

- Immediate data transfers
   In ST C2H direction, the completion Engine can be used to pass immediate data where the descriptor is not associated with any buffer
   but the descriptor contents itself is treated as the data.Imeediate data can be of 8,16,32 and 64 bytes in sizes.
   
- ST C2H completion entry coalescing
- Function Level Reset(FLR) Support
- Disabling overflow check in completion ring

- ST H2C to C2H and C2H to H2C loopback support
- Driver configuration through sysfs

- ECC Support

- Completion queue descriptors of 64 bytes size
- Flexible BAR mapping for QDMA configuration register space

For mode details on Hardware Features refer to 
.. _QDMA_PG: https://www.xilinx.com/support/documentation/ip_documentation/qdma/v3_0/pg302-qdma.pdf

QDMA Software Features
**********************

* Polling and Interrupt Modes
	QDMA software provides 2 differnt drivers. PF driver for Physical functions and and VF driver for Virtual Functions.
	PF and VF drivers can be inserted in differnt modes.

   - Polling Mode 
		In Poll Mode, Sofware polls for the writeback completions(Status Descriptor Write Back) 
		
   - Direct Interrupt Mode
		In Direct Interrupt moe, Each queue is assigned to one of the available interrupt vectors in a round robin fashion to service the requests. 
		Interrupt is raised by the HW upon receing the completions and software reads the completion status.
		
   - Indirect Interrupt Mode
		In Indirect Interrupt mode or Interrupt Aggregation mode, each vector has an associated Interrupt Aggregation Ring. 
		The QID and status of queues requiring service are written into the Interrupt Aggregation Ring. 
		When a PCIe MSI-X interrupt is received by the Host, the software reads the Interrupt Aggregation Ring to determine which queue needs service. 
		Mapping of queues to vectors is programmable
		
   - Auto Mode
		Auto mode is mix of Poll and Interrupt Aggregation mode. Driver polls for the writeback status updates.
		Interrupt aggregation is used for processing the completion ring.
		
- Allows only Privileged Physical Functions to program the contexts and registers
- Dynamic queue configuration
- Dynamic driver configuration
- Asynchronous and Synchronous IO support
- Display the Version details for SW and HW