**************************
QDMA Linux Driver UseCases
**************************

QDMA IP is released with five example designs in the Vivado® Design Suite. They are

#. AXI4 Memory Mapped And AXI-Stream with Completion
#. AXI Memory Mapped
#. AXI Memory Mapped with Completion
#. AXI Stream with Completion
#. AXI Stream Loopback
#. Descriptor Bypass In/Out Loopback

Refer to QDMA_Product_Guide_ for more details on these example designs.

.. _QDMA_Product_Guide: https://www.xilinx.com/support/documentation/ip_documentation/qdma/v3_0/pg302-qdma.pdf


The driver functionality remains same for all the example designs.
For ``Descriptor Bypass In/Out Loopback`` example design, application has to enable the bypass mode in driver.

All the flows described below are with respect to QDMA internal mode of operation.
Also, the changes required in driver for bypass mode of operation are specified.

====================================================
AXI4 Memory Mapped And AXI-Stream with Completion
====================================================

This is the default example design used to test the MM and ST functionality using QDMA driver. This example design provides blocks to interface with the AXI4 Memory Mapped and AXI4-Stream interfaces. This example design covers most of the functionality provided by QDMA.

Refer to QDMA Product Guide for more details on the example design and its registers.

Below sections describes C2H and H2C data flow for ST and MM mode required in all the example designs.

====================
MM H2C(Host-to-Card)
====================

This Example Design provides BRAM with AXI-MM interface to achieve the MM H2C functionality.
The driver comes with ``dmactl`` and ``dma_to_device`` applications which helps to realize the MM H2C functionality. QDMA driver takes care of HW updates.

The complete flow between the Host components and HW components is depicted in below sequence diagram.

- Application needs to configure the queue to start in MM mode and H2C direction 
- dma_to_device command takes a file containing the contents to be transmitted to FPGA memory as input
- Pass the  buffer to be transferred as an input to ``dma_to_device`` application which in turn passes it to the QDMA Driver
- QDMA driver divides buffer as 4KB chunks for each descriptor and programs the descriptors with buffer base address and updates the H2C ring PIDX
- Upon H2C ring PIDX update, DMA engine fetches the descriptors and passes them to H2C MM Engine for processing
- H2C MM Engine reads the buffer contents from the Host and writes to the BRAM
- Upon transfer completion, DMA Engine updates the CIDX in H2C ring completion status and generates interrupt if required. In poll mode, QDMA driver polls on CIDX update.
- QDMA driver processes the completion status and sends the response back to the application


.. image:: /images/MM_H2C_Flow.PNG
   :align: center

For MM (H2C and C2H) bypass mode, application needs to enable the bypass mode on the required queues.
Application can enable C2H/H2C bypass mode while starting the queue using ``dmactl`` application by specifying ``desc_bypass_en`` flag .

The MM descriptor format used by the example design is defined in QDMA Driver code base at ``linux-kernel/libqdma/descq.c``

::

	struct qdma_mm_desc {
		/** source address */
		__be64 src_addr;
		/** flags */
		__be32 flag_len;
		/** reserved 32 bits */
		__be32 rsvd0;
		/** destination address */
		__be64 dst_addr;
		/** reserved 64 bits */
		__be64 rsvd1;
	};

Update this structure if any changes required in the descriptor format for bypass mode.
Accordingly, update the data flow functionality in ``descq_mm_proc_request()`` functions in ``linux-kernel/libqdma/descq.c``.

====================
MM C2H(Card-to-Host)
====================

This Example Design provides BRAM with AXI-MM interface to achieve the MM C2H functionality.
The current driver with dmactl tool and dma_from_device application helps achieve the MM C2H functionality and QDMA driver takes care of HW updates.

The complete flow between the Host components and HW components is depicted in below sequence diagram.

- User needs to start the queue in MM mode and C2H direction 
- Pass the  buffer to copy the data as an input to ``dma_from_device`` application which inturn passes it to the QDMA Driver
- QDMA driver programs the required descriptors based on the length of the required buffer in multiples of 4KB chunks and programs the descriptors with buffer base address and updates the C2H ring PIDX
- Upon C2H ring PIDX update, DMA engine fetches the descriptors and passes them to C2H MM Engine for processing
- C2H MM Engine reads the BRAM contents writes to the Host buffers
- Upon transfer completion, DMA Engine updates the PIDX in C2H ring completion status and generates interrupt if required. In poll mode, QDMA driver polls on PIDX update.
- QDMA driver processes the completion status and sends the response back to the application with the data received.

.. image:: /images/MM_C2H_Flow.PNG
   :align: center
   
====================
ST H2C(Host-to-Card)
====================

In ST H2C, data is moved from Host to Device through H2C stream engine.The H2C stream engine moves data from the Host to the H2C Stream interface. The engine is
responsible for breaking up DMA reads to MRRS size, guaranteeing the space for completions,
and also makes sure completions are reordered to ensure H2C stream data is delivered to user
logic in-order.The engine has sufficient buffering for up to 256 DMA reads and up to 32 KB of data. DMA
fetches the data and aligns to the first byte to transfer on the AXI4 interface side. This allows
every descriptor to have random offset and random length. The total length of all descriptors put
to gather must be less than 64 KB.

There is no dependency on user logic for this use case.

The complete flow between the Host components and HW components is depicted in below sequence diagram.

- User needs to start the queue in ST mode and H2C direction 
- Pass the  buffer to be transferred as an input to ``dma_to_device`` application which in turn passes it to the QDMA Driver
- QDMA driver divides buffer as 4KB chunks for each descriptor and programs the descriptors with buffer base address and updates the H2C ring PIDX
- Upon H2C ring PIDX update, DMA engine fetches the descriptors and passes them to H2C Stream Engine for processing
- H2C Stream Engine reads the buffer contents from the Host buffers the data
- Upon transfer completion, DMA Engine updates the CIDX in H2C ring completion status and generates interrupt if required. In poll mode, QDMA driver polls on CIDX update.
- QDMA driver processes the completion status and sends the response back to the application

.. image:: /images/ST_H2C_Flow.PNG
   :align: center
   
For ST H2C bypass mode, application needs to enable the bypass mode on the required queues.
Application can enable H2C bypass mode while starting the queue using ``dmactl`` application by specifying ``desc_bypass_en`` flag .

The ST H2C descriptor format used by the example design is defined in QDMA Driver code base at ``linux-kernel/libqdma/descq.c``

::

	struct qdma_h2c_desc {
		__be16 cdh_flags;	/**< cdh flags */
		__be16 pld_len;		/**< current packet length */
		__be16 len;		/**< total packet length */
		__be16 flags;		/**< descriptor flags */
		__be64 src_addr;	/**< source address */
	};

Update this structure if any changes required in the descriptor format for bypass mode.
Accordingly, update the data flow functionality in ``descq_proc_st_h2c_request 	()`` functions in ``linux-kernel/libqdma/descq.c``.

   
====================
ST C2H(Card-to-Host)
====================

In ST C2H, data is moved from DMA Device to Host through C2H Stream Engine.

The C2H streaming engine is responsible for receiving data from the user logic and writing to the
Host memory address provided by the C2H descriptor for a given Queue.
The C2H Stream Engine, DMA writes the stream packets to the Host memory into the descriptors
provided by the Host QDMA driver through the C2H descriptor queue.

The C2H engine has two major blocks to accomplish C2H streaming DMA, 

- Descriptor Prefetch Cache (PFCH)
- C2H-ST DMA Write Engine

QDMA Driver needs to program the prefetch context along with the per queue context to achieve the ST C2H functionality.

The Prefetch Engine is responsible for calculating the number of descriptors needed for the DMA
that is writing the packet. The buffer size is fixed per queue basis. For internal and cached bypass
mode, the prefetch module can fetch up to 512 descriptors for a maximum of 64 different
queues at any given time.

The Completion Engine is used to write to the Completion queues.When used with a DMA engine, the
completion is used by the driver to determine how many bytes of data were transferred with
every packet. This allows the driver to reclaim the descriptors.

PFCH cache has three main modes, on a per queue basis, called 

- Simple Bypass Mode
- Internal Cache Mode
- Cached Bypass Mode 

While starting the queue in ST C2H mode using ``dmactl`` tool, user has the configuration options to configure
the queue in any of these 3 modes. 

The complete flow between the Host components and HW components is depicted in below sequence diagram.

.. image:: /images/ST_C2H_Flow.PNG
   :align: center
   
The current ST C2H functionality implemented in QDMA driver is tightly coupled with the Example Design.
Though the completion entry descriptor as per PG is fully configurable, this Example Design
mandates to have the the color, error and desc_used bits in the first nibble.
The completion entry format is defined in QDMA Driver code base **libqdma/qdma_st_c2h.c**

::

	struct cmpl_info {
	/* cmpl entry stat bits */
	union {
		u8 fbits;
		struct cmpl_flag {
			u8 format:1;
			u8 color:1;
			u8 err:1;
			u8 desc_used:1;
			u8 eot:1;
			u8 filler:3;
		} f;
	};
	u8 rsvd;
	u16 len;
	/* for tracking */
	unsigned int pidx;
	__be64 *entry;
	};
	
Completion entry is processed in ``parse_cmpl_entry()`` function which is part of **libqdma/qdma_st_c2h.c**.
If a different example design is opted, the QDMA driver code in **libqdma/qdma_st_c2h.h** and **libqdma/qdma_st_c2h.c** shall be updated to suit to the new example design.

The ST C2H descriptor format described above shall be changed as per example design requirements.

Update this structure if any changes required in the descriptor format for bypass mode.

Accordingly, update the data flow functionality in ``descq_process_completion_st_c2h ()`` functions in ``linux-kernel/libqdma/qdma_st_c2h.c``.

==================================
AXI4 Memory Mapped with Completion
==================================

This example design is generated when the DMA Interface Selection option is set to ``AXIMM with Completion`` in the Basic tab. Refer to QDMA Product Guide for more details on the example design and its registers.

The complete flow between the Host SW components and HW components for MM completion is depicted in below sequence diagram.

- Application add a queue in MM Mode and start the queue with MM Completions enabled
- Driver identifies the MM completion request from the Application provides flags and creates the completion ring along with Queue's H2C or C2H rings.
- Driver allocates the resources for the rings created and programs the contexts
- Application configures the user logic to generate the completions and  calls the command to read the completions
- Driver reads the completion ring based on the HW generated PIDX in completion status descriptors and updates CIDX
- Driver send the completion entries to the Application

.. image:: /images/MM_CMPT_Flow.PNG
   :align: center

