**************************
QDMA Linux Driver UseCases
**************************

QDMA IP is released with five example designs in the Vivado® Design Suite. They are

#. AXI4 Memory Mapped And AXI-Stream with Completion
#. AXI Memory Mapped
#. AXI Memory Mapped with Completion
#. AXI Stream with Completion
#. Descriptor Bypass In/Out Loopback

Refer to QDMA Program Guide for more details on these example designs.

All the use cases described below are with respect to synchronous application requests and described in QDMA internal mode.

The driver functionality remains same for ``AXI4 Memory Mapped And AXI-Stream with Completion``, ``AXI Memory Mapped`` and ``AXI Stream with Completion`` example designs.
For ``Descriptor Bypass In/Out Loopback`` example design, application has to enable the bypass mode in driver.
Currently driver does not support ``AXI Memory Mapped with Completion`` example design.


====================================================
AXI4 Memory Mapped And AXI-Stream with Completion
====================================================

This is the default example design used to test the MM and ST functionality using QDMA driver.
This example design provides blocks to interface with the AXI4 Memory Mapped and AXI4-Stream interfaces.

- The AXI-MM interface is connected to 512 KBytes of block RAM(BRAM).
- The AXI4-Stream interface is connected to custom data generator and data checker module.
- The CMPT interface is connected to the Completion block generator.
- The data generator and checker works only with predefined pattern, which is a 16-bit incremental pattern starting with 0. This data file is included in driver package.

.. image:: /images/Example_Design_1.PNG
   :align: center
   
The pattern generator and checker can be controlled using the registers found in the Example Design Registers. Refer to QDMA Program Guide for more details on these registers.

====================
MM H2C(Host-to-Card)
====================

This Example Design provides BRAM with AXI-MM interface to achieve the MM H2C functionality.
The current driver with dmactl tool and ``dma_to_device`` application helps achieve the MM H2C functionality and QDMA driver takes care of HW updations.

The complete flow between the Host components and HW components is depicted in below sequence diagram.

- User needs to start the queue in MM mode and H2C direction 
- Pass the  buffer to be transferred as an input to ``dma_to_device`` application which inturn passes it to the QDMA Driver
- QDMA driver devides buffer as 4KB chunks for each descriptor and programs the descriptors with buffer base address and updates the H2C ring PIDX
- Upon H2C ring PIDX update, DMA engine fetches the descriptors and passes them to H2C MM Engine for processing
- H2C MM Engine reads the buffer contents from the Host and writes to the BRAM
- Upon transfer completion, DMA Engine updates the CIDX in H2C ring completion status and generates interrupt if required. In poll mode, QDMA driver polls on CIDX update.
- QDMA driver processes the completion status and sends the response back to the application

.. image:: /images/MM_H2C_Flow.PNG
   :align: center

====================
MM C2H(Card-to-Host)
====================

This Example Design provides BRAM with AXI-MM interface to achieve the MM C2H functionality.
The current driver with dmactl tool and dma_from_device application helps achieve the MM C2H functionality and QDMA driver takes care of HW updations.

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
- Pass the  buffer to be transferred as an input to ``dma_to_device`` application which inturn passes it to the QDMA Driver
- QDMA driver devides buffer as 4KB chunks for each descriptor and programs the descriptors with buffer base address and updates the H2C ring PIDX
- Upon H2C ring PIDX update, DMA engine fetches the descriptors and passes them to H2C Stream Engine for processing
- H2C Stream Engine reads the buffer contents from the Host buffers the data
- Upon transfer completion, DMA Engine updates the CIDX in H2C ring completion status and generates interrupt if required. In poll mode, QDMA driver polls on CIDX update.
- QDMA driver processes the completion status and sends the response back to the application

.. image:: /images/ST_H2C_Flow.PNG
   :align: center
   
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

QDMA Driver needs to programm the prefetch context along with the per queue context to achieve the ST C2H functionality.

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
madates to have the the color, error and desc_used bits in the first nibble.
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


