*************************************
DPDK Driver Interop with Linux Driver
*************************************

QDMA supports 4 PFs and 252 VFs. Each PF can be binded to a different driver or each VF can be mapped to a VM and user can load the required driver in VM based on the required use case. Xilinx provides two Linux based QDMA reference drivers (Linux Kernel driver and DPDK user space driver). These two drivers are inter-operable in terms of mailbox communication for VF configuration and management. Hence, PFs and VFs can be binded independently to any of these two drivers.

An example use case is depicted in below figure where

- 2 PFs are attached to Linux driver, each PF creating 1 VF
- 2 PFs are attached to DPDK driver, each PF creating 1 VF
- 2 VMs are created where VF created from Linux kernel driver binded PF is attached to DPDK driver and viceversa.

.. image:: /images/dpdk_linux_interop.PNG
   :align: center

To achieve this setup, 

- remove the [Vendor-ID:Device-ID] tuple from pci table for the required functions to be binded with DPDK in linux-kernel/drv/pci_ids.h 
- Undefine ``ENABLE_INIT_CTXT_MEMORY`` in ``qdma_access_common.h`` for the driver that is loaded last

**Note**
--------
If any mailbox timeouts are observed during DPDK and Linux Interop testing, tweak the mailbox timeout values

- DPDK Driver:  ``#define MBOX_OP_RSP_TIMEOUT (10000 * MBOX_POLL_FRQ)`` located in ``dpdk/drivers/net/qdma/qdma_mbox.h`` 
- LINUX Driver: and ``#define QDMA_MBOX_MSG_TIMEOUT_MS	10000 /* 10 sec*/``  located in ``linux_kernel/driver/libqdma/qdma_mbox.h``