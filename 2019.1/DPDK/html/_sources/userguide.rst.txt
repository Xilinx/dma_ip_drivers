User Guide
==========

This section describes the details on controlling and configuring the QDMA IP

.. _run_dpdk:

Running the DPDK software test application
------------------------------------------

The below steps describe the step by step procedure to run the DPDK QDMA test
application and to interact with the QDMA PCIe device.

1. Navigate to examples/qdma_testapp directory.

::

	# cd <server_dir>/<dpdk_test_area>/dpdk-stable/examples/qdma_testapp

2. Run the 'lspci' command on the console and verify that the PFs are detected as shown below. Here, '81' is the PCIe bus number on which Xilinx QDMA device is installed.

::

	# lspci | grep Xilinx
	81:00.0 Memory controller: Xilinx Corporation Device 903f
	81:00.1 Memory controller: Xilinx Corporation Device 913f
	81:00.2 Memory controller: Xilinx Corporation Device 923f
	81:00.3 Memory controller: Xilinx Corporation Device 933f

3. Execute the following commands required for running the DPDK application

::

	# mkdir /mnt/huge
	# mount -t hugetlbfs nodev /mnt/huge
	# modprobe uio
	# insmod ../../build/kmod/igb_uio.ko

4. Bind PF ports to the igb_uio module as shown below

::

	# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.0
	# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.1
	# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.2
	# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.3

5. The execution of steps 3 and 4 creates a max_vfs file entry in /sys/bus/pci/devices/0000:<bus>:<device>.<function>.
Enable VFs for each PF by writing the number of VFs to enable to this file as shown below.
This example creates 1 VF for each PF.

::

	# echo 1 > /sys/bus/pci/devices/0000\:81\:00.0/max_vfs
	# echo 1 > /sys/bus/pci/devices/0000\:81\:00.1/max_vfs
	# echo 1 > /sys/bus/pci/devices/0000\:81\:00.2/max_vfs
	# echo 1 > /sys/bus/pci/devices/0000\:81\:00.3/max_vfs

6. Run the lspci command on the console and verify that the VFs are listed in the output as shown below

::

	# lspci | grep Xilinx
	81:00.0 Memory controller: Xilinx Corporation Device 903f
	81:00.1 Memory controller: Xilinx Corporation Device 913f
	81:00.2 Memory controller: Xilinx Corporation Device 923f
	81:00.3 Memory controller: Xilinx Corporation Device 933f
	81:00.4 Memory controller: Xilinx Corporation Device a03f
	81:08.4 Memory controller: Xilinx Corporation Device a13f
	81:10.0 Memory controller: Xilinx Corporation Device a23f
	81:17.4 Memory controller: Xilinx Corporation Device a33f

In total, 8 ports are serially arranged as shown above,
where 81:00.0 represents port 0, 81:00.1 represents port 1 and so on.
81:17.4 being the last one, represents port 7.

7. Execute the following commands to bind the VF ports to igb_uio module

::

	# ../../usertools/dpdk-devbind.py -b igb_uio 81:00.4
	# ../../usertools/dpdk-devbind.py -b igb_uio 81:08.4
	# ../../usertools/dpdk-devbind.py -b igb_uio 81:10.0
	# ../../usertools/dpdk-devbind.py -b igb_uio 81:17.4

8. Run the qdma_testapp using the following command

::

	#./build/app/qdma_testapp -c 0xf -n 4

- ``-c`` represents processor mask
- ``-n`` represents number of memory channels

Binding PCIe functions to VFIO
------------------------------

Above section binds PFs and VFs with igb_uio driver. However, one can also bind to vfio-pci driver, which provides more secure and IOMMU protected access to device.

Boot argument changes for VFIO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To bind with VFIO, enable IOMMU in the grub file as below

	Update ``/etc/default/grub`` file as below.

	::

		GRUB_CMDLINE_LINUX="default_hugepagesz=1GB hugepagesz=1G hugepages=20 iommu=on intel_iommu=on"

	Execute the following command to modify the ``/boot/grub/grub.cfg`` with the configuration set in the above steps and permanently add them to the kernel command line.

	::

		update-grub

	Reboot host system after making the above modifications.


Driver binding with vfio-pci
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

vfio-pci doesn't provide sysfs interface to enable VFs. Hence, we first bind PFs with igb_uio and enable VFs and then unbind from igb_uio to bind with vfio-pci.

Execute steps 1 to 6 of :ref:`run_dpdk` to bind PFs with igb_uio and enable VFs.

Now, unbind PFs from igb_uio driver and bind PFs and VFs to vfio-pci driver.

::

	# ../../usertools/dpdk-devbind.py -u 81:00.0
	# ../../usertools/dpdk-devbind.py -u 81:00.1
	# ../../usertools/dpdk-devbind.py -u 81:00.2
	# ../../usertools/dpdk-devbind.py -u 81:00.3

	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:00.0
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:00.1
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:00.2
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:00.3
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:00.4
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:08.4
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:10.0
	# ../../usertools/dpdk-devbind.py -b vfio-pci 81:17.4



Controlling and Configuring the QDMA IP
---------------------------------------

Supported Device arguments (module parameters)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Device specific parameters can be passed to a device by using the ‘-w’ EAL option.
Xilinx supports following device arguments to configure PCIe device.

1. **config_bar**

	This parameter specifies the PCIe BAR number where QDMA configuration register space is mapped.
	Valid values are 0 to 5. Default is set to 0 i.e. BAR 0 in the driver.

	Example usage:

	::

	./build/app/qdma_testapp -c 0x1f -n 4 -w 81:00.0,config_bar=2 -w 81:00.1,config_bar=4

	This example configures BAR 2 as QDMA configuration BAR for device "81:00.0"
	and BAR 4 as QDMA configuration BAR for device "81:00.1".

2. **desc_prefetch**

	This parameter enables or disables descriptor prefetch on C2H streaming queues.
	Default is prefetch disabled.

	Example usage:

	::

	./build/app/qdma_testapp -c 0x1f -n 4 -w 81:00.0,desc_prefetch=1

	This example enables descriptor prefetch on all the streaming C2H queues of
	the device "81:00.0".

3. **cmpt_desc_len**

	This parameter sets the completion entry length of the completion queue.
	Valid lengths are 8, 16 and 32 bytes. Default length is 8 bytes.

	Example usage:

	::

	./build/app/qdma_testapp -c 0x1f -n 4 -w 81:00.0,cmpt_desc_len=32

	This example sets completion entry length to 32 bytes on all the completion queues
	of the device "81:00.0".

4. **trigger_mode**

	This parameter sets the trigger mode for completion. Possible values for trigger_mode are:

	0 - DISABLE

	1 – Trigger on EVERY event

	2 – Trigger when USER_COUNT threshold is reached

	3 – Trigger when USER defined event is reached

	4 - Trigger when USER_TIMER threshold is reached

	5 - Trigger when either of USER_TIMER or COUNT is reached.

	Default value configured in the driver is 5.

	Example usage:

	::

	./build/app/qdma_testapp -c 0x1f -n 4 -w 81:00.0,trigger_mode=1

	This example sets the trigger mode to every event for all the
	completion queues of the device "81:00.0".

5. **c2h_byp_mode**

	This parameter sets the C2H stream mode. Valid values are

	0 - Bypass disabled

	1 - Cache bypass mode

	2 - Simple bypass mode

	Default is internal mode i.e. bypass disabled.

	Example usage:

	::

	./build/app/qdma_testapp -c 0x1f -n 4 -w 81:00.0,c2h_byp_mode=2

	This example sets simple bypass mode on all the C2H queues belonging to the
	PCIe device "81:00.0".

6. **h2c_byp_mode**

	This parameter sets the H2C bypass mode. Valid values are

	0 - Bypass disabled

	1 - Bypass enabled

	Default is Bypass disabled.

	Example usage:

	::

	./build/app/qdma_testapp -c 0x1f -n 4 -w 81:00.0,h2c_byp_mode=1

	This example sets bypass mode on all the H2C queues belonging to the
	PCIe device "81:00.0".


CLI support in qdma_testapp
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sample log of the qdma_testapp execution is given below.
After running the qdma_testapp, command line prompt appears on the console as shown below.

::

	#./build/app/qdma_testapp -c 0xf -n 4
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

1. **port_init**

	This command assigns queues to the port, sets up required resources for the queues, and prepares queues for data processing.
	Format for this commad is:

	::

		port_init <port-id> <num-queues> <num-st-queues> <ring-depth> <pkt-buff-size>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**num-queues** represents the total number of queues to be assigned to the port

	**num-st-queues** represents the total number of queues to be configured in streaming mode.
	This implies that the (num-queues - num-st-queues) number of queues has to be configured in memory mapped mode.

	**ring-depth** represents the number of entries in C2H and H2C descriptor rings of each queue of the port

	**pkt-buff-size** represents the size of the data that a single C2H or H2C descriptor can support

	Example usage:

	::

		port_init 1 32 16 1024 4096

	This example initializes Port 1 with first 16 queues in streaming mode and remaining 16 queues in memory mapped mode.
	Number of C2H and H2C descriptor ring depth is set to 1024 and data buffer of 4KB supported by each descriptor.

2. **port_close**

	This command frees up all the allocated resources and removes the queues associated with the port.
	Format for this commad is:

	::

		port_close <port-id>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	Example usage:

	::

		port_close 0

	This example closes the port 0. Port 0 can again be re-initialized with `port_init` command after `port_close` command.

3. **dma_to_device**

	This command is used to DMA the data from host to card.
	Format for this commad is:

	::

		dma_to_device <port-id> <num-queues> <input-filename> <dst-addr> <size> <iterations>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**num-queues** represents the total number of queues to use for transmitting the data

	**input-filename** represents the path to a valid binary data file, contents of which needs to be DMA'ed

	**dst-addr** represents the destination address (offset) in the card to where DMA should be done in memory mapped mode.
	This field is ignored for streaming mode queues.

	**size** represents the amount of data in bytes that needs to be transmitted to the card from the given input file.
	Data will be segmented across queues such that the total data transferred to the card is `size` amount

	**iterations** represents the number of loops to repeat the same DMA transfer

	Example usage:

	::

		dma_to_device 0 2048 mm_datafile_1MB.bin 0 524288 0

	This example segments the (524288) bytes from the mm_datafile_1MB.bin file equally to 2048 queues
	and transmits the segmented data on each queue starting at destination BRAM offset 0 for 1st queue,
	offset (1*524288)/2048 for 2nd queue, and so on.

4. **dma_from_device**

	This command is used to DMA the data from card to host.
	Format for this commad is:

	::

		dma_from_device <port-id> <num-queues> <output-filename> <src-addr> <size> <iterations>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**num-queues** represents the total number of queues to use to receive the data

	**output-filename** represents the path to output file to dump the received data

	**src-addr** represents the source address (offset) in the card from where DMA should be done in memory mapped mode.
	This field is ignored for streaming mode queues.

	**size** represents the amount of data in bytes that needs to be received from the card.
	Data will be segmented across queues such that the total data transferred from the card is `size` amount

	**iterations** represents the number of loops to repeat the same DMA transfer

	Example usage:

	::

		dma_from_device 0 2048 port0_qcount2048_size524288.bin 0 524288 0

	This example receives 524288 bytes from 2048 queues and writes to port0_qcount2048_size524288.bin file.
	1st queue receives (524288/2048) bytes of data from BRAM offset 0,
	2nd queue receives (524288/2048) bytes of data from BRAM offset (1*524288)/2048, and so on.

5. **reg_read**

	This command is used to read the specified register.
	Format for this commad is:

	::

		reg_read <port-id> <bar-num> <address>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**bar-num** represents the PCIe BAR where the register is located

	**address** represents offset of the register in the PCIe BAR `bar-num`

6. **reg_write**

	This command is used to write a 32-bit value to the specified register.
	Format for this commad is:

	::

		reg_write <port-id> <bar-num> <address> <value>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**bar-num** represents the PCIe BAR where the register is located

	**address** represents offset of the register in the PCIe BAR `bar-num`

	**value** represents the value to be written at the register offset `address`

7. **reg_dump**

	This command dumps important QDMA registers values of the given port on console.
	Format for this commad is:

	::

		reg_dump <port-id>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

8. **queue_dump**

	This command dumps the queue context of the specified queue number of the given port.
	Format for this commad is:

	::

		queue_dump <port-id> <queue-id>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**queue-id** represents the queue number relative to the port, whose context information needs to be logged

9. **desc_dump**

	This command dumps the descriptors of the C2H and H2C rings of the specified queue number of the given port.
	Format for this commad is:

	::

		desc_dump <port-id> <queue-id>

	**port-id** represents a logical numbering for PCIe functions in the order they are bind to ``igb_uio`` driver.
	The first PCIe function that is bound has port id as 0.

	**queue-id** represents the queue number relative to the port, whose C2H and H2C ring descriptors needs to be dumped

10. **load_cmds**

	This command executes the list of CLI commands from the given file.
	Format for this commad is:

	::

		load_cmds <file_name>

	**file_name** represents path to a valid file containing list of above described CLI commands to be executed in sequence.

11. **help**

	This command dumps the help menu with supported commands and their format.
	Format for this commad is:

	::

		help

12. **ctrl+d**

	The keyboard keys ``Ctrl`` and ``D`` when pressed together quits the application.


Virtual Machine (VM) execution and test
---------------------------------------
Assuming that the VM image has been created with the settings outlined in Table :ref:`guest_system_cfg`, follow below steps to execute qdma_testapp on VM.

1. Enable VF(s) on host system by writing the number of VF(s) to enable in ``max_vfs`` file under ``/sys/bus/pci/devices/0000:<bus>:<device>.<function>``.

	::

		# echo 1 > /sys/bus/pci/devices/0000\:81\:00.0/max_vfs

	``lspci`` should now show new entry for VF

	::

		81:00.4 Memory controller: Xilinx Corporation Device a03f

2. Start the VM using below command by attaching the VF (81:00.4 in this example)

	::

		qemu-system-x86_64 -cpu host -enable-kvm -m 4096 -object memory-backend-file,id=mem,size=4096M,mem-path=/mnt/huge,
		share=on -numa node,memdev=mem -mem-prealloc -smp sockets=2,cores=4 -hda <vm_image.qcow2> -device pci-assign,host=81:00.4

3. On the host system, bind the parent PFs of the VFs being tested on VM with the igb_uio driver and start qdma_testapp application

4. Once the VM is launched, execute steps as outlined in section :ref:`build_dpdk` to build DPDK on VM

5. Bind the VF device in VM to igb_uio driver and execute qdma_testapp in VM as outlined in section :ref:`run_dpdk`.

Executing with vfio-pci driver in VM using QEMU
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Follow below steps on Host side before attaching VFs to QEMU.

1. Find the Vendor Id (e.g. 10ee) and Device Id (e.g. a03f, a13f) of the VFs being attached to VM using ``lspci`` command.
Add the Vendor Id and Device Id to vfio-pci and bind the VFs to vfio-pci as below

::

	echo "10ee a03f" > /sys/bus/pci/drivers/vfio-pci/new_id
	echo 0000:81:00.4 > /sys/bus/pci/devices/0000:81:00.4/driver/unbind
	echo 0000:81:00.4 > /sys/bus/pci/drivers/vfio-pci/bind

	echo "10ee a13f" > /sys/bus/pci/drivers/vfio-pci/new_id
	echo 0000:81:08.4 > /sys/bus/pci/devices/0000:81:08.4/driver/unbind
	echo 0000:81:08.4 > /sys/bus/pci/drivers/vfio-pci/bind

2. Attach the VFs (here, 81:00.4 and 81:08.4) to VM via vfio-pci using QEMU with below command.

::

	qemu-system-x86_64 -cpu host -enable-kvm -m 4096 -mem-prealloc -smp sockets=2,cores=4 -hda vm_image.qcow2 -device vfio-pci,host=81:00.4 -device vfio-pci,host=81:08.4


Follow below steps inside VM to bind the functions with vfio-pci

1. Once the VM is launched, execute steps as outlined in section :ref:`build_dpdk` to build DPDK on VM

2. Enable ``noiommu`` mode to support vfio-pci driver in VM as there won't be physical IOMMU device present inside VM

::

	modprobe vfio-pci
	echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode

3. Bind the VF device in VM to vfio-pci driver and execute qdma_testapp in VM as outlined in section :ref:`run_dpdk`.
