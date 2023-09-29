###############################################################################

                 Xilinx QDMA Software README

###############################################################################

_____________________________________________________________________________
Contents

1.   Setup: Download and modifications
2    Setup: Host system
3.   Setup: Build Commands
4.   Compile Test application
5.   Running the DPDK software test application
_____________________________________________________________________________


Note: This DPDK driver and applciation were tested on Ubuntu 18.04 machine.


1.) Setup: Download and modifications

The reference driver code requires DPDK version 22.11.
Follow the steps below to download the proper version of DPDK and apply
driver code and test application supplied in the GitHub.

Extract the DPDK driver software database from the Xilinx GitHub to the server where VCU1525
is installed. Henceforth, this area is referred as <dpdk_sw_database>.

Create a directory for the DPDK download on the server where the VCU1525
is installed and move to this directory.

 $ mkdir <server_dir>/<dpdk_test_area>
 $ cd <server_dir>/<dpdk_test_area>
 $ git clone http://dpdk.org/git/dpdk-stable
 $ cd dpdk-stable
 $ git checkout v22.11
 $ git clone git://dpdk.org/dpdk-kmods
 $ cp -r <dpdk_sw_database>/drivers/net/qdma ./drivers/net/
 $ cp -r <dpdk_sw_database>/examples/qdma_testapp ./examples/

	Additionally, make below changes to the DPDK 22.11 tree to build QDMA driver,
	support 4K queues and populate Xilinx devices for binding.

		i. Add QDMA driver
			a. To support 4K queues and 256 PCIe functions, update below configurations	in ./config/rte_config.h
				CONFIG_RTE_MAX_MEMZONE=40960
				CONFIG_RTE_MAX_ETHPORTS=256
				CONFIG_RTE_MAX_QUEUES_PER_PORT=4096

			b. Add below lines to ./config/meson.build in DPDK 22.11 tree
				# Set maximum Ethernet ports to 256
				dpdk_conf.set('RTE_MAX_ETHPORTS', 256)

				# Set maximum VFIO Groups to 256
				dpdk_conf.set('RTE_MAX_VFIO_GROUPS', 256)

			c. Add below lines to ./config/rte_config.h to enable driver debug logs
				#define RTE_LIBRTE_QDMA_DEBUG_DRIVER 1

			d. Add below line to ./drivers/net/meson.build, where PMDs are added to drivers list
				'qdma',

			e. To add Xilinx devices for device binding, add below lines to	./usertools/dpdk-devbind.py after cavium_pkx class, where PCI base class for devices are listed.
				xilinx_qdma_pf = {'Class':  '05', 'Vendor': '10ee', 'Device': '9011,9111,9211,9311,9014,9114,9214,9314,9018,9118,9218,9318,901f,911f,921f,931f,9021,9121,9221,9321,9024,9124,9224,9324,9028,9128,9228,9328,902f,912f,922f,932f,9031,9131,9231,9331,9034,9134,9234,9334,9038,9138,9238,9338,903f,913f,923f,933f,9041,9141,9241,9341,9044,9144,9244,9344,9048,9148,9248,9348,b011,b111,b211,b311,b014,b114,b214,b314,b018,b118,b218,b318,b01f,b11f,b21f,b31f,b021,b121,b221,b321,b024,b124,b224,b324,b028,b128,b228,b328,b02f,b12f,b22f,b32f,b031,b131,b231,b331,b034,b134,b234,b334,b038,b138,b238,b338,b03f,b13f,b23f,b33f,b041,b141,b241,b341,b044,b144,b244,b344,b048,b148,b248,b348',
				'SVendor': None, 'SDevice': None}
				xilinx_qdma_vf = {'Class':  '05', 'Vendor': '10ee', 'Device': 'a011,a111,a211,a311,a014,a114,a214,a314,a018,a118,a218,a318,a01f,a11f,a21f,a31f,a021,a121,a221,a321,a024,a124,a224,a324,a028,a128,a228,a328,a02f,a12f,a22f,a32f,a031,a131,a231,a331,a034,a134,a234,a334,a038,a138,a238,a338,a03f,a13f,a23f,a33f,a041,a141,a241,a341,a044,a144,a244,a344,a048,a148,a248,a348,c011,c111,c211,c311,c014,c114,c214,c314,c018,c118,c218,c318,c01f,c11f,c21f,c31f,c021,c121,c221,c321,c024,c124,c224,c324,c028,c128,c228,c328,c02f,c12f,c22f,c32f,c031,c131,c231,c331,c034,c134,c234,c334,c038,c138,c238,c338,c03f,c13f,c23f,c33f,c041,c141,c241,c341,c044,c144,c244,c344,c048,c148,c248,c348',
				'SVendor': None, 'SDevice': None}

			d. Update entries in network devices class in ./usertools/dpdk-devbind.py to add Xilinx devices
				network_devices = [network_class, cavium_pkx, xilinx_qdma_pf, xilinx_qdma_vf],

2.) Setup: Host system

DPDK requires that hugepages are setup on the server.
The following modifications must be made to the /boot/grub/grub.cfg on the host system

	i. Add hugepages for DPDK
		- Add following parameter to /etc/default/grub file
			GRUB_CMDLINE_LINUX="default_hugepagesz=1GB hugepagesz=1G hugepages=20"

		This example adds 20 1GB hugepages, which are required to support 2048 queues, with descriptor ring of 1024 entries and each descriptor buffer length of 4KB. The number of hugepages required should be changed if the above configuration (queues, ring size, buffer size) changes.

	ii. Enable IOMMU for VM testing
		- Update /etc/default/grub file as below.
			GRUB_CMDLINE_LINUX="default_hugepagesz=1GB hugepagesz=1G hugepages=20 iommu=pt intel_iommu=on"

	iii. Execute the following command to modify the /boot/grub/grub.cfg with the configuration set in the above steps and permanently add them to the kernel command line.
		update-grub

	iv. Reboot host system after making the above modifications.

3.) Setup: Build Commands

	i. Compile DPDK & QDMA driver
		a. Execute the following to compile and install the driver.
			cd <server_dir>/<dpdk_test_area>/dpdk-stable
			meson build
			cd build
			ninja
			ninja install
			ldconfig
			
		- The following should appear when ninja completes
			Linking target app/test/dpdk-test.

		- Verify that librte_net_qdma.a is installed in ./build/drivers directory.

	ii. Execute the following to compile the igb_uio kernel driver.
			cd <server_dir>/<dpdk_test_area>/dpdk-stable/dpdk-kmods/linux/igb_uio
			make

	Important Note for VF 4K queue support for CPM5 design only
    -----------------------------------------------------------

    To enable VF 4K queue driver support for CPM5 design, QDMA DPDK driver need
    to compile by enabling the EQDMA_CPM5_VF_GT_256Q_SUPPORTED macro

				cd <server_dir>/<dpdk_test_area>/dpdk-stable
				meson build -DEQDMA_CPM5_VF_GT_256Q_SUPPORTED=1
				cd build
				ninja
				ninja install
				ldconfig

4.) Compile Test application

	i. Change to root user and compile the application
		sudo su
		cd examples/qdma_testapp
		make RTE_SDK=`pwd`/../.. RTE_TARGET=build

	- The following should appear when make completes
			ln -sf qdma_testapp-shared build/qdma_testapp

	ii. Additionally, for memory mapped mode, BRAM size can be configured with make command.
	Default BRAM size is set to 512KB in the driver makefile.


5.) Running the DPDK software test application

The below steps describe the step by step procedure to run the DPDK QDMA test
application and to interact with the QDMA PCIe device.

	i. Navigate to examples/qdma_testapp directory.
		cd <server_dir>/<dpdk_test_area>/dpdk-stable/examples/qdma_testapp

	ii. Run the 'lspci' command on the console and verify that the PFs are detected as shown below. Here, '81' is the PCIe bus number on which Xilinx QDMA device is installed.
		# lspci | grep Xilinx
		81:00.0 Memory controller: Xilinx Corporation Device 903f
		81:00.1 Memory controller: Xilinx Corporation Device 913f
		81:00.2 Memory controller: Xilinx Corporation Device 923f
		81:00.3 Memory controller: Xilinx Corporation Device 933f

	iii. Execute the following commands required for running the DPDK application
		# mkdir /mnt/huge
		# mount -t hugetlbfs nodev /mnt/huge
		# modprobe uio
		# insmod <server_dir>/<dpdk_test_area>/dpdk-stable/dpdk-kmods/linux/igb_uio/igb_uio.ko

	iv. Bind PF ports to the igb_uio module as shown below
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.0
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.1
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.2
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.3

	v. The execution of steps iii and iv creates a max_vfs file entry in /sys/bus/pci/devices/0000:<bus>:<device>.<function>.

	vi. Enable VFs for each PF by writing the number of VFs to enable to this file as shown below.
		This example creates 1 VF for each PF.
			# echo 1 > /sys/bus/pci/devices/0000\:81\:00.0/max_vfs
			# echo 1 > /sys/bus/pci/devices/0000\:81\:00.1/max_vfs
			# echo 1 > /sys/bus/pci/devices/0000\:81\:00.2/max_vfs
			# echo 1 > /sys/bus/pci/devices/0000\:81\:00.3/max_vfs

	vii. Run the lspci command on the console and verify that the VFs are listed in the output as shown below
		# lspci | grep Xilinx
		81:00.0 Memory controller: Xilinx Corporation Device 903f
		81:00.1 Memory controller: Xilinx Corporation Device 913f
		81:00.2 Memory controller: Xilinx Corporation Device 923f
		81:00.3 Memory controller: Xilinx Corporation Device 933f
		81:00.4 Memory controller: Xilinx Corporation Device a03f
		81:08.4 Memory controller: Xilinx Corporation Device a13f
		81:10.0 Memory controller: Xilinx Corporation Device a23f
		81:17.4 Memory controller: Xilinx Corporation Device a33f

	viii. Execute the following commands to bind the VF ports to igb_uio module
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.4
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:08.4
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:10.0
		# ../../usertools/dpdk-devbind.py -b igb_uio 81:17.4

	ix. Run the qdma_testapp using the following command
		#./build/qdma_testapp -c 0xf -n 4
			- '-c' represents processor mask
			- '-n' represents number of memory channels

CLI support in qdma_testapp
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sample log of the qdma_testapp execution is given below.
After running the qdma_testapp, command line prompt appears on the console as shown below.

::

	#./build/qdma_testapp -c 0xf -n 4
	QDMA testapp rte eal init...
	EAL: Detected 8 lcore(s)
	EAL: Probing VFIO support...
	EAL: PCI device 0000:81:00.0 on NUMA socket -1
	EAL: probe driver: 10ee:903f net_qdma
	EAL: PCI device 0000:81:00.1 on NUMA socket -1
	EAL: probe driver: 10ee:913f net_qdma
	EAL: PCI device 0000:81:00.2 on NUMA socket -1
	EAL: probe driver: 10ee:923f net_qdma
	EAL: PCI device 0000:81:00.3 on NUMA socket -1
	EAL: probe driver: 10ee:933f net_qdma
	EAL: PCI device 0000:81:00.4 on NUMA socket -1
	EAL: probe driver: 10ee:a03f net_qdma
	EAL: PCI device 0000:81:08.4 on NUMA socket -1
	EAL: probe driver: 10ee:a13f net_qdma
	EAL: PCI device 0000:81:10.0 on NUMA socket -1
	EAL: probe driver: 10ee:a23f net_qdma
	EAL: PCI device 0000:81:17.4 on NUMA socket -1
	EAL: probe driver: 10ee:a33f net_qdma
	Ethernet Device Count: 8
	Logical Core Count: 4
	xilinx-app>

Commands supported by the qdma_testapp CLI
++++++++++++++++++++++++++++++++++++++++++

1. port_init

	This command assigns queues to the port, sets up required resources for the queues, and prepares queues for data processing.
	Format for this commad is:

		port_init <port-id> <num-queues> <num-st-queues> <ring-depth> <pkt-buff-size>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	num-queues represents the total number of queues to be assigned to the port

	num-st-queues represents the total number of queues to be configured in streaming mode.
	This implies that the (num-queues - num-st-queues) number of queues has to be configured in memory mapped mode.

	ring-depth represents the number of entries in C2H and H2C descriptor rings of each queue of the port

	pkt-buff-size represents the size of the data that a single C2H or H2C descriptor can support

	Example usage:

		port_init 1 32 16 1024 4096

	This example initializes Port 1 with first 16 queues in streaming mode and remaining 16 queues in memory mapped mode.
	Number of C2H and H2C descriptor ring depth is set to 1024 and data buffer of 4KB supported by each descriptor.

2. port_close

	This command frees up all the allocated resources and removes the queues associated with the port.
	Format for this commad is:

		port_close <port-id>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	Example usage:

	::

		port_close 0

	This example closes the port 0. Port 0 can again be re-initialized with `port_init` command after `port_close` command.

3. port_reset

	This command resets the DPDK port. This command is supported for VF port only.
	This command closes the port and re-initializes it with the values provided in this command.
	Format for this commad is:

		port_reset <port-id> <num-queues> <num-st-queues> <ring-depth> <pkt-buff-size>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	num-queues represents the total number of queues to be assigned to the port

	num-st-queues represents the total number of queues to be configured in streaming mode.
	This implies that the (num-queues - num-st-queues) number of queues has to be configured in memory mapped mode.

	ring-depth represents the number of entries in C2H and H2C descriptor rings of each queue of the port

	pkt-buff-size represents the size of the data that a single C2H or H2C descriptor can support

	Example usage:

		port_reset 4 32 16 1024 4096

	This example command resets the port 4 and re-initializes it with first 16 queues in streaming mode and
	remaining 16 queues in memory mapped mode. Number of C2H and H2C descriptor ring depth is set to 1024
	and data buffer of 4KB supported by each descriptor.

4. port_remove

	This command frees up all the resources allocated for the port and removes the port from application use.
	User will need to restart the application to use the port again.
	Format for this commad is:

		port_remove <port-id>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	Example usage:

		port_remove 4

	This example removes the port 4. Restart the application to use port 4 again.

5. dma_to_device

	This command is used to DMA the data from host to card.
	Format for this commad is:

		dma_to_device <port-id> <num-queues> <input-filename> <dst-addr> <size> <iterations>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	num-queues represents the total number of queues to use for transmitting the data

	input-filename represents the path to a valid binary data file, contents of which needs to be DMA'ed

	dst-addr represents the destination address (offset) in the card to where DMA should be done in memory mapped mode.
	This field is ignored for streaming mode queues.

	size represents the amount of data in bytes that needs to be transmitted to the card from the given input file.
	Data will be segmented across queues such that the total data transferred to the card is `size` amount

	iterations represents the number of loops to repeat the same DMA transfer

	Example usage:

		dma_to_device 0 2048 mm_datafile_1MB.bin 0 524288 0

	This example segments the (524288) bytes from the mm_datafile_1MB.bin file equally to 2048 queues
	and transmits the segmented data on each queue starting at destination BRAM offset 0 for 1st queue,
	offset (1*524288)/2048 for 2nd queue, and so on.

6. dma_from_device

	This command is used to DMA the data from card to host.
	Format for this commad is:

		dma_from_device <port-id> <num-queues> <output-filename> <src-addr> <size> <iterations>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	num-queues represents the total number of queues to use to receive the data

	output-filename represents the path to output file to dump the received data

	src-addr represents the source address (offset) in the card from where DMA should be done in memory mapped mode.
	This field is ignored for streaming mode queues.

	size represents the amount of data in bytes that needs to be received from the card.
	Data will be segmented across queues such that the total data transferred from the card is `size` amount

	iterations represents the number of loops to repeat the same DMA transfer

	Example usage:

		dma_from_device 0 2048 port0_qcount2048_size524288.bin 0 524288 0

	This example receives 524288 bytes from 2048 queues and writes to port0_qcount2048_size524288.bin file.
	1st queue receives (524288/2048) bytes of data from BRAM offset 0,
	2nd queue receives (524288/2048) bytes of data from BRAM offset (1*524288)/2048, and so on.

7. reg_read

	This command is used to read the specified register.
	Format for this commad is:

		reg_read <port-id> <bar-num> <address>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	bar-num represents the PCIe BAR where the register is located

	address represents offset of the register in the PCIe BAR `bar-num`

8. reg_write

	This command is used to write a 32-bit value to the specified register.
	Format for this commad is:

		reg_write <port-id> <bar-num> <address> <value>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	bar-num represents the PCIe BAR where the register is located

	address represents offset of the register in the PCIe BAR `bar-num`

	value represents the value to be written at the register offset `address`

9. reg_dump

	This command dumps important QDMA registers values of the given port on console.
	Format for this commad is:

		reg_dump <port-id>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

10. reg_info_read

	This command reads the field info for the specified number of registers of the given port on console.
	Format for this commad is:

		reg_info_read <port-id> <reg-addr> <num-regs>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	reg-addr represents offset of the register in the PCIe BAR bar-num

	rum-regs represents number of registers to read

11. queue_dump

	This command dumps the queue context of the specified queue number of the given port.
	Format for this commad is:

		queue_dump <port-id> <queue-id>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	queue-id represents the queue number relative to the port, whose context information needs to be logged

12. desc_dump

	This command dumps the descriptors of the C2H and H2C rings of the specified queue number of the given port.
	Format for this commad is:

		desc_dump <port-id> <queue-id>

	port-id represents a logical numbering for PCIe functions in the order they are bind to igb_uio driver.
	The first PCIe function that is bound has port id as 0.

	queue-id represents the queue number relative to the port, whose C2H and H2C ring descriptors needs to be dumped

13. load_cmds

	This command executes the list of CLI commands from the given file.
	Format for this commad is:

		load_cmds <file_name>

	file_name represents path to a valid file containing list of above described CLI commands to be executed in sequence.

14. help

	This command dumps the help menu with supported commands and their format.
	Format for this commad is:

		help

15. ctrl+d

	The keyboard keys Ctrl and D when pressed together quits the application.

Instructions on how to use proc-info test for driver debugging:
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

1. Apply the below patch on top of the code changes mentioned in the dpdk driver documentation page and build the dpdk source code.
	patch -p1 < 0001-Add-QDMA-xdebug-to-proc-info-of-dpdk-22.11.patch

2. Run the testpmd application as primary application on one linux terminal
	./build/app/dpdk-testpmd -l 1-17 -n 4 -a 65:00.0,desc_prefetch=1,cmpt_desc_len=16 --log-level=3 -- --burst=256 -i --nb-cores=1 --rxq=1 --txq=1 --forward-mode=io --rxd=2048 --txd=2048 --mbcache=512 --mbuf-size=4096

3. Run the proc info as secondary application on another linux terminal as mentioned below with diferent port combinations.
One port:
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 -g
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --qdevice
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --qinfo
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --qstats_clr
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --qstats
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --desc-dump tx
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --desc-dump rx
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --desc-dump cmpt
	./build/app/dpdk-proc-info -a 81:00.0 --log-level=7 -- -p 1 -q 0 --stats
Two ports:
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 -g
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --qdevice
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --qinfo
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --qstats_clr
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --qstats
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --desc-dump tx
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --desc-dump rx
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --desc-dump cmpt
	./build/app/dpdk-proc-info -a 81:00.0, -a 81:00.1, --log-level=7 -- -p 3 -q 0 --stats
4. Available commands for proc info are mentioned below.
		-m to display DPDK memory zones, segments and TAILQ information
		-g to display DPDK QDMA PMD global CSR info
		-p PORTMASK: hexadecimal bitmask of ports to retrieve stats for
		--stats: to display port statistics, enabled by default
		--qdevice: to display QDMA device structure
		--qinfo: to display QDMA queue context and queue structures
		--qstats: to display QDMA Tx and Rx queue stats
		--qstats_clr: to clear QDMA Tx and Rx queue stats
		--desc-dump {rx | tx | cmpt}: to dump QDMA queue descriptors
		--xstats: to display extended port statistics, disabled by default
		--metrics: to display derived metrics of the ports, disabled by



/*-
 *   BSD LICENSE
 *
 *   Copyright (c) 2017-2022 Xilinx, Inc. All rights reserved.
 *   Copyright (c) 2022-2023, Advanced Micro Devices, Inc. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

