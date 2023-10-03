.. _build_dpdk:

Building QDMA DPDK Software
===========================

DPDK requires certain packages to be installed on host system.
For a full list, refer to the official DPDK documentation:
https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html.

**Note**: If the NUMA library is missing, it should be installed.
For example:

	For Ubuntu: ``sudo apt-get install libnuma-dev``

	For Red Hat: ``sudo yum install numactl-devel``

Modifying the driver for PCIe device ID
---------------------------------------

During the PCIe DMA IP customization in Vivado, user can specify a PCIe Device ID.
This Device ID must be added to the driver to identify the PCIe QDMA device.
The current driver is designed to recognize the PCIe Device IDs
that get generated with the PCIe example design when this value has not been modified.
If the PCIe Device ID is modified during IP customization,
one needs to modify QDMA PMD to recognize this new ID.

User can also remove PCIe Device IDs that will not be used by the end solution.
To modify the PCIe Device ID in the driver,

	For PF devices,
		update ``struct rte_pci_id qdma_pci_id_tbl[]`` inside ``drivers/net/qdma/qdma_ethdev.c``
	For VF devices,
		update ``struct rte_pci_id qdma_vf_pci_id_tbl[]`` inside ``drivers/net/qdma/qdma_vf_ethdev.c``

	Also add the device IDs in ``usertools/dpdk-devbind.py`` in ``xilinx_qdma_pf`` for PF device
	and ``xilinx_qdma_vf`` for VF device as specified in later section.

Once modified, the driver and application must be recompiled.

Xilinx QDMA DPDK Software database structure
--------------------------------------------

Below Table describes the DPDK software database structure and its contents
on the Xilinx GitHub https://github.com/Xilinx/dma_ip_drivers, subdirectory QDMA/DPDK.

+-----------------------------+------------------------------------------------------------------+
| Directory                   | Description                                                      |
+=============================+==================================================================+
| drivers/net/qdma            | Xilinx QDMA DPDK poll mode driver                                |
+-----------------------------+------------------------------------------------------------------+
| examples/qdma_testapp       | Xilinx CLI based test application for QDMA                       |
+-----------------------------+------------------------------------------------------------------+
| tools/0001-PKTGEN-20.12.0-  | This is dpdk-pktgen patch based on DPDK v20.11                   |
| Patch-to-add-Jumbo-packet   | This patch extends dpdk-pktgen application to handle packets     |
| -support.patch              | with packet sizes more than 1518 bytes and it disables the       |
|                             | packet size classification logic to remove application           |
|                             | overhead in performance measurement. This patch is used for      |
|                             | performance testing with dpdk-pktgen application.                |
+-----------------------------+------------------------------------------------------------------+
| tools/0001-PKTGEN-22.04.1-  | This is dpdk-pktgen patch based on DPDK v22.11.                  |
| Patch-to-add-Jumbo-packet   | This patch extends dpdk-pktgen application to handle packets     |
| -support.patch              | with packet sizes more than 1518 bytes and it disables the       |
|                             | packet size classification logic to remove application           |
|                             | overhead in performance measurement. This patch is used for      |
|                             | performance testing with dpdk-pktgen application.                |
+-----------------------------+------------------------------------------------------------------+
| RELEASE.txt                 | Release Notes                                                    |
+-----------------------------+------------------------------------------------------------------+

Setup: Download and modifications
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The reference driver code requires DPDK v22.11/v21.11/v20.11.
Follow the steps below to download the proper version of DPDK and apply
driver code and test application supplied in the GitHub.

Extract the DPDK driver software database from the Xilinx GitHub to the server where VCU1525
is installed. Henceforth, this area is referred as <dpdk_sw_database>.

Create a directory for the DPDK download on the server where the VCU1525
is installed and move to this directory.

::

	mkdir <server_dir>/<dpdk_test_area>
	cd <server_dir>/<dpdk_test_area>
	git clone http://dpdk.org/git/dpdk-stable
	cd dpdk-stable
	git checkout v22.11/v21.11/v20.11
	git clone git://dpdk.org/dpdk-kmods
	cp -r <dpdk_sw_database>/drivers/net/qdma ./drivers/net/
	cp -r <dpdk_sw_database>/examples/qdma_testapp ./examples/

Additionally, make below changes to the DPDK v22.11/v21.11/v20.11 tree to build QDMA driver,
support 2K queues and populate Xilinx devices for binding.

	1. To build QDMA driver

	a. To support 2K queues and 256 PCIe functions for QDMA5.0, update below configurations	in ``./config/rte_config.h``

	::

		#define RTE_MAX_MEMZONE 40960
		#define RTE_MAX_VFIO_CONTAINERS 256
		#define RTE_MAX_QUEUES_PER_PORT 2048

	b. To support 4K queues for CPM5 designs, update below configurations in ``./config/rte_config.h``

	::

		#define RTE_MAX_QUEUES_PER_PORT 4096

	c. Add below lines to ``./config/meson.build`` in DPDK v22.11/v21.11/v20.11 tree

	::

		# Set maximum Ethernet ports to 256
		dpdk_conf.set('RTE_MAX_ETHPORTS', 256)

		# Set maximum VFIO Groups to 256
		dpdk_conf.set('RTE_MAX_VFIO_GROUPS', 256)

	d. Add below lines to ``./config/rte_config.h`` to enable driver debug logs

	::

		#define RTE_LIBRTE_QDMA_DEBUG_DRIVER 1

	e. Add below line to ``./drivers/net/meson.build``, where PMDs are added to drivers list

	::

		'qdma',

	f. Add below line to ``./drivers/net/qdma/meson.build``, to support required DPDK framework version

	::

		# Use QDMA_DPDK_22_11 compiler flag for DPDK v22.11
		# Use QDMA_DPDK_21_11 compiler flag for DPDK v21.11
		# Use QDMA_DPDK_20_11 compiler flag for DPDK v20.11
		cflags += ['-DQDMA_DPDK_22_11']


	2. To add Xilinx devices for device binding, add below lines to	``./usertools/dpdk-devbind.py`` after cavium_pkx class, where PCI base class for devices are listed.

	::

		xilinx_qdma_pf = {'Class': '05', 'Vendor': '10ee', 'Device': '9011,9111,9211,9311,9014,9114,9214,9314,9018,9118,9218,9318,901f,911f,921f,931f,9021,9121,9221,9321,9024,9124,9224,9324,9028,9128,9228,9328,902f,912f,922f,932f,9031,9131,9231,9331,9034,9134,9234,9334,9038,9138,9238,9338,903f,913f,923f,933f,9041,9141,9241,9341,9044,9144,9244,9344,9048,9148,9248,9348,b011,b111,b211,b311,b014,b114,b214,b314,b018,b118,b218,b318,b01f,b11f,b21f,b31f,b021,b121,b221,b321,b024,b124,b224,b324,b028,b128,b228,b328,b02f,b12f,b22f,b32f,b031,b131,b231,b331,b034,b134,b234,b334,b038,b138,b238,b338,b03f,b13f,b23f,b33f,b041,b141,b241,b341,b044,b144,b244,b344,b048,b148,b248,b348,b058,b158,b258,b358',
		'SVendor': None, 'SDevice': None}
		xilinx_qdma_vf = {'Class': '05', 'Vendor': '10ee', 'Device': 'a011,a111,a211,a311,a014,a114,a214,a314,a018,a118,a218,a318,a01f,a11f,a21f,a31f,a021,a121,a221,a321,a024,a124,a224,a324,a028,a128,a228,a328,a02f,a12f,a22f,a32f,a031,a131,a231,a331,a034,a134,a234,a334,a038,a138,a238,a338,a03f,a13f,a23f,a33f,a041,a141,a241,a341,a044,a144,a244,a344,a048,a148,a248,a348,c011,c111,c211,c311,c014,c114,c214,c314,c018,c118,c218,c318,c01f,c11f,c21f,c31f,c021,c121,c221,c321,c024,c124,c224,c324,c028,c128,c228,c328,c02f,c12f,c22f,c32f,c031,c131,c231,c331,c034,c134,c234,c334,c038,c138,c238,c338,c03f,c13f,c23f,c33f,c041,c141,c241,c341,c044,c144,c244,c344,c048,c148,c248,c348,c058,c158,c258,c358',
		'SVendor': None, 'SDevice': None}

	Update entries in network devices class in ``./usertools/dpdk-devbind.py`` to add Xilinx devices

	::

		network_devices = [network_class, cavium_pkx, xilinx_qdma_pf, xilinx_qdma_vf]

	3. Additional qdma dpdk driver compile time flags

	a. To enable VF 4K queue driver support for CPM5 design, add below line to ./drivers/net/qdma/meson.build

	::

		cflags += ['-DEQDMA_CPM5_VF_GT_256Q_SUPPORTED']

	b. Writeback coalesce timer count value in QDMA is used to flush CMPT data packet at a constant interval. By default Writeback coalesce timer count is configured for low latency performance measurements, but to configure this value for high throughput measurements, add below line to ./drivers/net/qdma/meson.build

	::

		cflags += ['-DTHROUGHPUT_MEASUREMENT']

	b. To enhance the DPDK driver's debugging capabilities, added support for latency measurement statistics in both the Tx path (TxQ SW PIDX to HW CIDX) and the Rx path (RxQ SW PIDX to CMPT PIDX). To enable this driver support, please add the following line to ./drivers/net/qdma/meson.build

	::

		cflags += ['-DLATENCY_MEASUREMENT']

Setup: Host system
^^^^^^^^^^^^^^^^^^

DPDK requires that hugepages are setup on the server.
The following modifications must be made to the ``/boot/grub/grub.cfg`` on the host system

- Add hugepages for DPDK

	Add following parameter to ``/etc/default/grub file``

	::

		GRUB_CMDLINE_LINUX="default_hugepagesz=1GB hugepagesz=1G hugepages=20"

	| This example adds 20 1GB hugepages, which are required to support 2048 queues, with descriptor ring of 1024 entries and each descriptor buffer length of 4KB.
	| The number of hugepages required should be changed if the above configuration (queues, ring size, buffer size) changes.

- Enable IOMMU for VM testing

	Update ``/etc/default/grub`` file as below.

	::

		GRUB_CMDLINE_LINUX="default_hugepagesz=1GB hugepagesz=1G hugepages=20 iommu=pt intel_iommu=on"

Execute the following command to modify the ``/boot/grub/grub.cfg`` with the configuration set in the above steps and permanently add them to the kernel command line.

	::

		update-grub

Reboot host system after making the above modifications.

Setup: Make Commands
^^^^^^^^^^^^^^^^^^^^

* Compile DPDK & QDMA driver

	- Make sure to delete all the rte and cmdline header files from /usr/local/include before switching to different dpdk framework versions.


	- Execute the following to compile the driver.

	::

		cd <server_dir>/<dpdk_test_area>/dpdk-stable
		meson build
		cd build
		ninja
		ninja install
		ldconfig

	- The following should appear when ninja completes

	  ::

		Linking target app/test/dpdk-test.

	- Verify that ``librte_net_qdma.a`` is installed in ``./build/drivers`` directory.


	- Execute the following to compile the igb_uio kernel driver.

	::

		cd <server_dir>/<dpdk_test_area>/dpdk-stable/dpdk-kmods/linux/igb_uio
		make

* Compile Test application

	Change to root user and compile the application

	::

		sudo su
		cd examples/qdma_testapp
		make RTE_SDK=`pwd`/../.. RTE_TARGET=build

	The following should appear when make completes

	::

		ln -sf qdma_testapp-shared build/qdma_testapp

	Additionally, for memory mapped mode, BRAM size can be configured with make command.
	Default BRAM size is set to 512KB in the driver makefile.

	::

		make BRAM_SIZE=<BRAM size in bytes in decimal> RTE_SDK=`pwd`/../.. RTE_TARGET=build


If any of above steps are missed or require code modifications,
perform ``make clean`` before required modifications and re-building.
For driver related modifications, perform ``make clean``
from inside ``build`` directory.

