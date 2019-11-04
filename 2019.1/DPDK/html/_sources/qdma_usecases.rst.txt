**************************
QDMA DPDK Driver UseCases
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

This is the default example design used to test the MM and ST functionality using QDMA driver.
This example design provides blocks to interface with the AXI4 Memory Mapped and AXI4-Stream interfaces.
This example design covers most of the functionality provided by QDMA.

Refer to QDMA Product Guide for more details on the example design and its registers.

Below sections describes C2H and H2C data flow for ST and MM mode required in all the example designs.

MM H2C(Host-to-Card)
--------------------

The Example Design provides BRAM with AXI-MM interface to achieve the MM H2C functionality.
The CLI command ``dma_to_device`` supported by the DPDK software test application helps verify the MM H2C functionality.
QDMA driver takes care of HW configuration and data processing.

The complete flow between the Host SW components and HW components is depicted in below sequence diagram.

- Application needs to configure the queue in MM mode and H2C (Tx) direction
- ``dma_to_device`` command takes a file containing the contents to be transmitted to FPGA memory as input. Application divides the file contents between the number of queues provided and calls ``rte_eth_tx_burst()`` API on each configured queue for data transmission.
- Application sets the memory offset to write to in FPGA using ``rte_pmd_qdma_set_mm_endpoint_addr`` API
- QDMA driver frees previous transmitted mbufs pending in the queue and checks for H2C ring capacity by finding the difference between PIDX and CIDX
- QDMA driver programs the descriptors with buffer base address and length to be transmitted
- QDMA driver updates the H2C ring PIDX and polls the status descriptor for CIDX to be same as PIDX
- Upon H2C ring PIDX update, DMA engine fetches the descriptors and passes them to H2C MM Engine for processing
- H2C MM Engine reads the buffer contents from the Host and writes to the BRAM at the given memory offset
- Upon transfer completion, DMA Engine updates the CIDX in H2C ring completion status descriptor, indicating to SW that the transmission of corresponding descriptors is completed.
- QDMA driver processes the completion status and sends the response back to the application

.. image:: /images/MM_H2C_Flow.PNG
   :align: center

For MM (H2C and C2H) bypass mode, application needs to enable the bypass mode on the required queues.
Application can enable C2H/H2C bypass using device arguments (``c2h_byp_mode`` and ``h2c_byp_mode``) during application launch or
by invoking ``rte_pmd_qdma_configure_rx_bypass() / rte_pmd_qdma_configure_tx_bypass()`` APIs from application on the required queues.

The MM descriptor format used by the example design is defined in QDMA Driver code base at ``drivers/net/qdma/qdma.h``

::

	struct __attribute__ ((packed)) qdma_mm_desc
	{
		volatile uint64_t	src_addr;
		volatile uint64_t	len:28;
		volatile uint64_t	dv:1;
		volatile uint64_t	sop:1;
		volatile uint64_t	eop:1;
		volatile uint64_t	rsvd:33;
		volatile uint64_t	dst_addr;
		volatile uint64_t	rsvd2;

	};

Update this structure if any changes required in the descriptor format for bypass mode.
Accordingly, update the data flow functionality in ``qdma_xmit_pkts_mm() / qdma_recv_pkts_mm()`` functions in ``drivers/net/qdma/qdma_rxtx.c``.

MM C2H(Card-to-Host)
--------------------

The Example Design provides BRAM with AXI-MM interface to achieve the MM C2H functionality.
The CLI command ``dma_from_device`` supported by the DPDK software test application helps verify the MM C2H functionality.
QDMA driver takes care of HW configuration and data processing.

The complete flow between the Host SW components and HW components is depicted in below sequence diagram.

- Application needs to configure the queue in MM mode and C2H (Rx) direction
- Application sets the memory offset to read from in FPGA using ``rte_pmd_qdma_set_mm_endpoint_addr`` API
- Application distributes total requested data across the given queues and calls the ``rte_eth_rx_burst()`` API on each queue
- QDMA driver programs the required descriptors with buffer base address and length based on the number of packets requested and the length of packet buffer (mbuf size)
- QDMA driver updates the C2H ring PIDX and polls the status descriptor for CIDX to be same as PIDX
- Upon C2H ring PIDX update, DMA engine fetches the descriptors and passes them to C2H MM Engine for processing
- C2H MM Engine reads the BRAM contents and writes to the Host buffers
- Upon transfer completion, DMA Engine updates the CIDX in C2H ring completion status descriptor, indicating to SW that the corresponding descriptors are available for consumption.
- QDMA driver processes the completion status descriptor and sends the response back to the application with the data received

.. image:: /images/MM_C2H_Flow.PNG
   :align: center

ST H2C(Host-to-Card)
--------------------

In ST H2C, data is moved from Host to Device through H2C stream engine.The H2C stream engine moves data from the Host to the H2C Stream interface. The engine is
responsible for breaking up DMA reads to MRRS size, guaranteeing the space for completions,
and also makes sure completions are reordered to ensure H2C stream data is delivered to user
logic in-order.The engine has sufficient buffering for up to 256 DMA reads and up to 32 KB of data. DMA
fetches the data and aligns to the first byte to transfer on the AXI4 interface side. This allows
every descriptor to have random offset and random length. The total length of all descriptors put
to gather must be less than 64 KB.

There is no dependency on user logic for this use case.
The CLI command ``dma_to_device`` supported by the DPDK software test application helps verify the ST H2C functionality.

The complete flow between the Host SW components and HW components is depicted in below sequence diagram.

- User needs to configure the queue in ST mode (default) and H2C (Tx) direction
- ``dma_to_device`` command takes a file containing the contents to be transmitted to FPGA as input. Application divides the file contents between the number of queues provided and calls ``rte_eth_tx_burst()`` API on each configured queue for data transmission.
- QDMA driver frees previous transmitted mbufs pending in the queue and checks for H2C ring capacity by finding the difference between PIDX and CIDX.
- QDMA driver programs the descriptors with buffer base address and length to be transmitted and updates the H2C ring PIDX
- QDMA driver returns to the application the number packets it was able to write in the descriptor ring without waiting for acknowledgement from HW.
- Upon H2C ring PIDX update, DMA engine fetches the descriptors and passes them to H2C MM Engine for processing
- H2C MM Engine reads the buffer contents from the Host and writes to the BRAM at the given memory offset
- Upon transfer completion, DMA Engine updates the CIDX in H2C ring completion status, indicating to SW that the transmission of corresponding descriptors is completed.

.. image:: /images/ST_H2C_Flow.PNG
   :align: center

For ST H2C bypass mode, application needs to enable the bypass mode on the required queues.
Application can enable H2C bypass using device argument (``h2c_byp_mode``) during application launch or
by invoking ``rte_pmd_qdma_configure_tx_bypass()`` API from application on the required queues.

The ST H2C descriptor format used by the example design is defined in QDMA Driver code base at ``drivers/net/qdma/qdma.h``

::

	struct __attribute__ ((packed)) qdma_h2c_desc
	{
		volatile uint16_t	cdh_flags;
		volatile uint16_t	pld_len;
		volatile uint16_t	len;
		volatile uint16_t	flags;
		volatile uint64_t	src_addr;
	};

Update this structure if any changes required in the descriptor format for bypass mode.
Accordingly, update the data flow functionality in ``qdma_xmit_pkts_st()`` function in ``drivers/net/qdma/qdma_rxtx.c``.

ST C2H(Card-to-Host)
--------------------

In ST C2H, data is moved from DMA Device to Host through C2H Stream Engine.

The C2H streaming engine is responsible for receiving data from the user logic and writing to the
Host memory address provided by the C2H descriptor for a given queue.
The C2H Stream Engine DMA writes the stream packets to the host memory into the descriptors
provided by the host QDMA driver through the C2H descriptor queue.

The C2H engine has two major blocks to accomplish C2H streaming DMA,

- Descriptor Prefetch Engine (PFCH)
- C2H-ST DMA Write Engine

QDMA Driver needs to program the prefetch context along with the queue software context to achieve the ST C2H functionality.

The Prefetch Engine is responsible for calculating the number of descriptors needed for the DMA
that is writing the packet. The buffer size is fixed per queue basis. For internal and cached bypass
mode, the prefetch module can fetch up to 512 descriptors for a maximum of 64 different
queues at any given time.

The Completion Engine is used to write to the Completion queues.
Though the completion queue is independent of C2H queue, the example design binds it with C2H Stream engine so that when used with a DMA engine, the
completion is used by the driver to determine how many bytes of data were transferred with
every packet. This allows the driver to reclaim the descriptors.

The complete flow between the Host SW components and HW components is depicted in below sequence diagram.

- Application needs to configure the queue in ST mode (default) and C2H (Rx) direction
- Application calls the ``rte_eth_rx_burst()`` API on each queue with number of packets to receive as input
- Application programs user logic registers to generate the required packets on a given queue, before calling ``rte_eth_rx_burst()`` API on that queue
- QDMA driver processes the completion queue to determine the packet length of each received packet and updates the completion queue CIDX
- QDMA driver retrieves the packets from the C2H ring based on the number of descriptors consumed per packet
- QDMA driver populates the C2H ring descriptors with new packet buffer addresses
- QDMA driver updates the C2H ring PIDX for HW to start using the new descriptors
- QDMA driver returns the packets retrieved to the application

.. image:: /images/ST_C2H_Flow.PNG
   :align: center

The Streaming C2H functionality implemented in QDMA driver is tightly coupled with the Example Design
because the completion entry descriptor is defined by the user logic in the example design.

The completion entry format used by the example design is defined in QDMA Driver code base at ``drivers/net/qdma/qdma_user.h``

::

	struct __attribute__ ((packed)) c2h_cmpt_ring
	{
		/* For 2018.2 IP, this field determines the
		 * Standard or User format of completion entry
		 */
		volatile uint32_t	data_frmt:1;

		/* This field inverts every time
		 * PIDX wraps the completion ring
		 */
		volatile uint32_t	color:1;

		/* Indicates that C2H engine
		 * encountered a descriptor error
		 */
		volatile uint32_t	err:1;

		/* Indicates that the completion
		 * packet consumes descriptor in C2H ring
		 */
		volatile uint32_t	desc_used:1;

		/* Indicates length of the data packet */
		volatile uint32_t	length:16;

		/* Reserved field */
		volatile uint32_t	user_rsv:4;

		/* User logic defined data of
		 * length based on CMPT entry length
		 */
		volatile uint8_t	user_def[];
	};

Completion entry is processed in ``qdma_recv_pkts_st()`` function which is part of ``drivers/net/qdma/qdma_rxtx.c``.
If a different example design is used, the QDMA driver code in ``drivers/net/qdma/qdma_user.h`` and
``drivers/net/qdma/qdma_rxtx.c`` must be updated to suit to the descriptor format defined by the new example design.


For ST C2H bypass mode, application needs to enable the bypass mode on the required queues.
Application can enable C2H bypass using device argument (``c2h_byp_mode``) during application launch or
by invoking ``rte_pmd_qdma_configure_rx_bypass()`` API from application on the required queues.

The ST C2H descriptor format used by the example design is defined in QDMA Driver code base at ``drivers/net/qdma/qdma.h``

::

	struct __attribute__ ((packed)) qdma_c2h_desc
	{
		volatile uint64_t	dst_addr;
	};


Update this structure if any changes required in the descriptor format for bypass mode.
Accordingly, update the data flow functionality in ``qdma_recv_pkts_st()`` function in ``drivers/net/qdma/qdma_rxtx.c``.


==================================
AXI4 Memory Mapped with Completion
==================================

This example design is generated when the DMA Interface Selection option is set to ``AXIMM
with Completion`` in the Basic tab.
Refer to QDMA Product Guide for more details on the example design and its registers.

The complete flow between the Host SW components and HW components for MM completion is depicted in below sequence diagram.

- Application needs to configure the queue in MM mode
- Application calls ``rte_pmd_qdma_dev_cmptq_setup()`` API to allocate resources for Completion ring
- Application calls ``rte_pmd_qdma_dev_cmptq_start()`` API to program the completion ring context
- Application programs the Example design registers to generate completions in the completion ring
- Application calls ``rte_pmd_qdma_mm_cmpt_process()`` API to process completion ring and receive the completion data in the application provided buffers
- Application can call MM H2C and C2H flow for data transfers along with the completions processing
- When done, application can call ``rte_pmd_qdma_dev_cmptq_stop()`` API to disable the completion context

.. image:: /images/MM_CMPT_Flow.png
   :align: center
