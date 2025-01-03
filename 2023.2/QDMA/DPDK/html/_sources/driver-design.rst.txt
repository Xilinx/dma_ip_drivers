Driver Design
*************

QDMA DPDK driver is implemented as an Ethernet poll mode driver in DPDK v20.11.
It supports both streaming (network) and memory (compute) interface to QDMA.
Below diagram gives a high level system overview of the QDMA DPDK driver.

.. image:: /images/dpdk_overview.png
   :align: center

Below sections describes design of different blocks and use cases of DPDK driver.

Driver interfaces and callbacks
===============================

The Driver registers two driver interfaces (``net_qdma`` for PFs and ``net_qdma_vf`` for VFs) with DPDK RTE.
Below ``eth_dev_ops`` callbacks are implemented by both driver interfaces of the DPDK driver.

::

	static struct eth_dev_ops qdma_eth_dev_ops = {
		.dev_configure            = qdma_dev_configure,
		.dev_infos_get            = qdma_dev_infos_get,
		.dev_start                = qdma_dev_start,
		.dev_stop                 = qdma_dev_stop,
		.dev_close                = qdma_dev_close,
		.dev_reset                = qdma_dev_reset,
		.link_update              = qdma_dev_link_update,
		.rx_queue_setup           = qdma_dev_rx_queue_setup,
		.tx_queue_setup           = qdma_dev_tx_queue_setup,
		.rx_queue_release         = qdma_dev_rx_queue_release,
		.tx_queue_release         = qdma_dev_tx_queue_release,
		.rx_queue_start           = qdma_dev_rx_queue_start,
		.rx_queue_stop            = qdma_dev_rx_queue_stop,
		.tx_queue_start           = qdma_dev_tx_queue_start,
		.tx_queue_stop            = qdma_dev_tx_queue_stop,
		.rx_queue_count           = qdma_dev_rx_queue_count,
		.rx_descriptor_status     = qdma_dev_rx_descriptor_status,
		.tx_descriptor_status     = qdma_dev_tx_descriptor_status,
		.tx_done_cleanup          = qdma_dev_tx_done_cleanup,
		.queue_stats_mapping_set  = qdma_dev_queue_stats_mapping,
		.get_reg                  = qdma_dev_get_regs,
		.stats_get                = qdma_dev_stats_get,
		.stats_reset              = qdma_dev_stats_reset,
		.rxq_info_get             = qdma_dev_rxq_info_get,
		.txq_info_get             = qdma_dev_txq_info_get,
	};


Below sequence diagram depicts the application call flow to these driver callbacks and their high level operations.

.. image:: images/dpdk_call_flow.png

Implementation details of the callback APIs is described further in

.. toctree::
   :maxdepth: 1

   dpdk-callbacks.rst

Additional interfaces are exported by the driver for QDMA configuration. These new interfaces are described in

.. toctree::
   :maxdepth: 1

   qdma_pmd_export.rst

Resource Management
===================

.. toctree::
   :maxdepth: 1

   qdma_resmgmt.rst

Mailbox Communication for VF configuration
==========================================

.. toctree::
   :maxdepth: 1

   qdma_mailbox.rst

PF/VF Shutdown
==============

.. toctree::
   :maxdepth: 1

   pf_vf_shutdown.rst

DPDK Driver Interop with Linux Driver
=====================================

.. toctree::
   :maxdepth: 1

   dpdk_linux_interop.rst

QDMA Hardware Error Monitoring
==============================

There are various causes for errors in QDMA IP e.g. descriptor errors, DMA engine errors, etc.
If the errors are enabled by SW, QDMA logs the status of error in the respective global status registers.

DPDK driver executes a poll thread ``qdma_check_errors()`` every 1 second to check the error status and log the type of HW error occured on the console.

DPDK Driver debug
=================
DPDK driver provides below APIs to dump debug information.

``rte_pmd_qdma_dbg_regdump()``
	Dump QDMA registers

``rte_pmd_qdma_dbg_qdevice()``
	Dump QDMA device structure maintained by SW

``rte_pmd_qdma_dbg_qinfo()``
	Dump QDMA queue context maintained by HW and queue structures maintained by SW

``rte_pmd_qdma_dbg_qdesc()``
	Dump the specified descriptor ring of the given queue id

User application can call these APIs to dump the required debug information.
