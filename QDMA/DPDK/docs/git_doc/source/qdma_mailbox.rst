*************
QDMA Mailbox
*************

The QDMA Subsystem for PCIe provides an optional feature to support the Single Root I/O
Virtualization (SR-IOV). SR-IOV classifies the functions as:

- Physical Functions (PF) :
	Full featured PCIe functions which include SR-IOV capabilities among others.

- Virtual Functions (VF):
	PCIe functions featuring configuration space with Base Address
	Registers (BARs) but lacking the full configuration resources and are controlled by the PF
	configuration. The main role of the VF is data transfer.

VF can communicate to a PF through mailbox. Each function implements one 128B inbox and 128B outbox message buffer.
These mailboxes are accessible to the driver via PCIe BAR of its own function.
HW also provides ability to interrupt the driver for an incoming mailbox message to a PCIe function.
For further details on the mailbox internals and mailbox registers, refer to QDMA_Product_Guide_.

.. _QDMA_Product_Guide: https://www.xilinx.com/support/documentation/ip_documentation/qdma/v3_0/pg302-qdma.pdf

Physical function (PF) is privileged with full access to QDMA registers and resources, but VFs updates only data handling registers and interrupts.
VF drivers must communicate with the driver attached to the PF through the mailbox for configuration and resource allocation.
The PF and VF drivers define the message formatting to be exchanged between the driver.

The driver enables mailbox interrupt and registers a Rx handler for notification of incoming mailbox message.
It also creates a Tx thread to send a message and poll for its response from the peer driver.

Following are the list of messages supported between PF and VF driver

::

	enum mbox_msg_op {
		/** @MBOX_OP_BYE: vf offline */
		MBOX_OP_BYE,
		/** @MBOX_OP_HELLO: vf online */
		MBOX_OP_HELLO,
		/** @: FMAP programming request */
		MBOX_OP_FMAP,
		/** @MBOX_OP_CSR: global CSR registers request */
		MBOX_OP_CSR,
		/** @MBOX_OP_QREQ: request queues */
		MBOX_OP_QREQ,
		/** @MBOX_OP_QNOTIFY_ADD: notify of queue addition */
		MBOX_OP_QNOTIFY_ADD,
		/** @MBOX_OP_QNOTIFY_DEL: notify of queue deletion */
		MBOX_OP_QNOTIFY_DEL,
		/** @MBOX_OP_QCTXT_WRT: queue context write */
		MBOX_OP_QCTXT_WRT,
		/** @MBOX_OP_QCTXT_RD: queue context read */
		MBOX_OP_QCTXT_RD,
		/** @MBOX_OP_QCTXT_CLR: queue context clear */
		MBOX_OP_QCTXT_CLR,
		/** @MBOX_OP_QCTXT_INV: queue context invalidate */
		MBOX_OP_QCTXT_INV,
		/** @MBOX_OP_INTR_CTXT_WRT: interrupt context write */
		MBOX_OP_INTR_CTXT_WRT,
		/** @MBOX_OP_INTR_CTXT_RD: interrupt context read */
		MBOX_OP_INTR_CTXT_RD,
		/** @MBOX_OP_INTR_CTXT_CLR: interrupt context clear */
		MBOX_OP_INTR_CTXT_CLR,
		/** @MBOX_OP_INTR_CTXT_INV: interrupt context invalidate */
		MBOX_OP_INTR_CTXT_INV,

		/** @MBOX_OP_HELLO_RESP: response to @MBOX_OP_HELLO */
		MBOX_OP_HELLO_RESP = 0x81,
		/** @MBOX_OP_FMAP_RESP: response to @MBOX_OP_FMAP */
		MBOX_OP_FMAP_RESP,
		/** @MBOX_OP_CSR_RESP: response to @MBOX_OP_CSR */
		MBOX_OP_CSR_RESP,
		/** @MBOX_OP_QREQ_RESP: response to @MBOX_OP_QREQ */
		MBOX_OP_QREQ_RESP,
		/** @MBOX_OP_QADD: notify of queue addition */
		MBOX_OP_QNOTIFY_ADD_RESP,
		/** @MBOX_OP_QNOTIFY_DEL: notify of queue deletion */
		MBOX_OP_QNOTIFY_DEL_RESP,
		/** @MBOX_OP_QCTXT_WRT_RESP: response to @MBOX_OP_QCTXT_WRT */
		MBOX_OP_QCTXT_WRT_RESP,
		/** @MBOX_OP_QCTXT_RD_RESP: response to @MBOX_OP_QCTXT_RD */
		MBOX_OP_QCTXT_RD_RESP,
		/** @MBOX_OP_QCTXT_CLR_RESP: response to @MBOX_OP_QCTXT_CLR */
		MBOX_OP_QCTXT_CLR_RESP,
		/** @MBOX_OP_QCTXT_INV_RESP: response to @MBOX_OP_QCTXT_INV */
		MBOX_OP_QCTXT_INV_RESP,
		/** @MBOX_OP_INTR_CTXT_WRT_RESP: response to @MBOX_OP_INTR_CTXT_WRT */
		MBOX_OP_INTR_CTXT_WRT_RESP,
		/** @MBOX_OP_INTR_CTXT_RD_RESP: response to @MBOX_OP_INTR_CTXT_RD */
		MBOX_OP_INTR_CTXT_RD_RESP,
		/** @MBOX_OP_INTR_CTXT_CLR_RESP: response to @MBOX_OP_INTR_CTXT_CLR */
		MBOX_OP_INTR_CTXT_CLR_RESP,
		/** @MBOX_OP_INTR_CTXT_INV_RESP: response to @MBOX_OP_INTR_CTXT_INV */
		MBOX_OP_INTR_CTXT_INV_RESP,

		/** @MBOX_OP_MAX: total mbox opcodes*/
		MBOX_OP_MAX
	};


The following sequence diagrams shows the mailbox communication between PF and VF for VF configuration.

.. image:: /images/mailbox_communication.png
   :align: center