###########################################################
#  Instructions for building DPDK, testapp, and running:
###########################################################

The driver requires a large chunk of continuous memory.
Add the following to the linux boot command in the /boot/grub2/grub.cfg file:

For this configuration, the following is required:
default_hugepagesz=1G hugepagesz=1GB hugepages=4

Note: Use  "sh qdma_testapp/setup-hugepages.sh <hugepages-count> to setup hugepages as *root/sudo* user then reboot system.
      example : ./qdma_testapp/setup-hugepages.sh 10 ; reboot -f /* To setup 10 huge pages*/

An example grub.cfg modification snippet lines shown below:
menuentry 'CentOS Linux (3.10.0-514.26.2.el7.x86_64) 7 (Core)' --class centos --class gnu-linux --class gnu --class os --unrestricted $menuentry_id_option 'gnulinux-3.10.0-327.18.2.el7.x86_64-advanced-31fc0493-e909-4426-905c-d1b7673cb6a5' {
		load_video
		set gfxpayload=keep
		insmod gzio
		insmod part_msdos
		insmod xfs
		set root='hd0,msdos1'
		if [ x$feature_platform_search_hint = xy ]; then
		  search --no-floppy --fs-uuid --set=root --hint-bios=hd0,msdos1 --hint-efi=hd0,msdos1 --hint-baremetal=ahci0,msdos1 --hint='hd0,msdos1'  6d9a0e33-283b-4714-8590-87420ee6881b
		else
		  search --no-floppy --fs-uuid --set=root 6d9a0e33-283b-4714-8590-87420ee6881b
		fi
		linux16 /vmlinuz-3.10.0-514.26.2.el7.x86_64 root=/dev/mapper/centos-root ro crashkernel=auto rd.lvm.lv=centos/root rd.lvm.lv=centos/swap rhgb quiet default_hugepagesz=1G hugepagesz=1G hugepages=4 LANG=en_US.UTF-8 pci=realloc=on
		initrd16 /initramfs-3.10.0-514.26.2.el7.x86_64.img
}

Reboot the server after making the above change.
Assume you are currently in the directory containing this file.

	Note: This DPDK driver and applciation were tested on Centos-7.2 x86_64 bit machine.


1.) Build the DPDK PMD & libraries:
 $ cd <dpdk_test_area>
 $ git clone http://dpdk.org/git/dpdk-stable
 $ cd dpdk-stable
 $ git checkout v17.11.1
 $ cp -r <dpdk_sw_database>/drivers/net/qdma ./drivers/net/
 $ cp -r <dpdk_sw_database>/examples/qdma_testapp ./examples/

	Make below changes in DPDK tree for QDMA driver
		i. Add QDMA driver
			a. Add below lines to ./config/common_base in DPDK 17.11.1 tree
				#
				#Complie Xilinx QDMA PMD driver
				#
				CONFIG_RTE_LIBRTE_QDMA_PMD=y
				CONFIG_RTE_LIBRTE_QDMA_DEBUG_DRIVER=n
						To enable driver debug logs, set
				CONFIG_RTE_LIBRTE_QDMA_DEBUG_DRIVER=y
			b. Add below lines to drivers/net/Makefile, where PMDs are added
				DIRS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += qdma
			c. Add below lines to mk/rte.app.mk, where PMDs are added
				_LDLIBS-$(CONFIG_RTE_LIBRTE_QDMA_PMD) += -lrte_pmd_qdma

		ii.	To add Xilinx devices for device binding, add below lines to ./usertools/dpdk-devbind.py after cavium_pkx class, where PCI base class for devices are listed.
				xilinx_qdma_pf = {'Class':  '05', 'Vendor': '10ee', 'Device': '9011,9111,9211,9311,9014,9114,9214,9314,9018,9118,9218,9318,901f,911f,921f,931f,9021,9121,9221,9321,9024,9124,9224,9324,9028,9128,9228,9328,902f,912f,922f,932f,9031,9131,9231,9331,9034,9134,9234,9334,9038,9138,9238,9338,903f,913f,923f,933f,9041,9141,9241,9341,9044,9144,9244,9344,9048,9148,9248,9348',
							  'SVendor': None, 'SDevice': None}
				xilinx_qdma_vf = {'Class':  '05', 'Vendor': '10ee', 'Device': 'a011,a111,a211,a311,a014,a114,a214,a314,a018,a118,a218,a318,a01f,a11f,a21f,a31f,a021,a121,a221,a321,a024,a124,a224,a324,a028,a128,a228,a328,a02f,a12f,a22f,a32f,a031,a131,a231,a331,a034,a134,a234,a334,a038,a138,a238,a338,a03f,a13f,a23f,a33f,a041,a141,a241,a341,a044,a144,a244,a344,a048,a148,a248,a348',
							  'SVendor': None, 'SDevice': None}

			Update entries in network devices class in ./usertools/dpdk-devbind.py to add Xilinx devices
				network_devices = [network_class, cavium_pkx, xilinx_qdma_pf, xilinx_qdma_vf]

		iii. To support 2K queues and 256 PCIe functions, update below configurations in ./config/common_base
				CONFIG_RTE_MAX_MEMZONE=7680
				CONFIG_RTE_MAX_ETHPORTS=256
				CONFIG_RTE_MAX_QUEUES_PER_PORT=2048

 $ make config T=x86_64-native-linuxapp-gcc
	Note: Above uses default output: ./build
 $ make
 $ make BRAM_SIZE=262144 // optional command, required when BRAM-size modified, currently it was set to 256KB
	Note: QDMA BRAM-size is defined to 512K by default. To change this, you can specify the BRAM-size to the
			command line arguments of make as shown in above example command(for 64K BRAM size need to mention as 65536 decimal value). If this BRAM_SIZE argument was not specified in make command, then it considers default BRAM-size 512K.

2.) Build the test application:
 $ cd examples/qdma_testapp
 $ . setup.sh
 $ make
 $ make BRAM_SIZE=262144 // optional command, required when BRAM-size modified, currently it was set to 256KB
	Note: QDMA BRAM-size is defined to 256K by default. To change this,  you can specify the BRAM-size to the command line arguments of make as shown in above example command(for 64K BRAM size need to mention as 65536 decimal value). If this BRAM_SIZE argument was not specified in make command, then it considers default BRAM-size 256K.


3.) Prepare the system for DPDK runtime ( sudo if not root)
 $ mkdir /mnt/huge
 $ mount -t hugetlbfs nodev /mnt/huge
 $ modprobe uio
 $ insmod ../../build/kmod/igb_uio.ko

4.) Bind the driver
 $ ../../usertools/dpdk-devbind.py --status

	Should see:
		Other network devices
		=====================
		0000:01:00.0 'Device 903f' unused=igb_uio
		0000:01:00.1 'Device 913f' unused=igb_uio

For the above (the device number may vary):

 $ ../../usertools/dpdk-devbind.py -b igb_uio 01:00.0
 $ ../../usertools/dpdk-devbind.py -b igb_uio 01:00.1


5.)  To execute "testapp" with custom parameters, Run the test application  with either 'a' or 'b' or 'c' options as follows:
	(where, -c processor mask -n memory channels, --f input-commands-file)

	a) $ ./build/app/qdma_testapp -c 0xf -n 4 --

	b) $ ./build/app/qdma_testapp -c 0xf -n 4 --  --filename="<input-filename>"  /** This input-filename indicates the commands filename to run the list of commands mentioned in the file automatically **/
	Example:  ./build/app/qdma_testapp -c 0xf -n 4 -- --filename="all_host_tests.txt"

		Examples:
				./build/app/qdma_testapp -c 0xf -n 4                       /* configure max queues as 64, ie default */
				./build/app/qdma_testapp -c 0xf -n 4 --  --config="(8)"    /* configure max queues as 8 */
				./build/app/qdma_testapp -c 0xf -n 4 --  --config="(2048)" /* configure max queues as 2048 */
		Note: To support 2048 queue pairs, server must need a minimum of 17GB free huge pages on local NUMA node.
				Update the kernel paramets with 17 huges pages as shown below.
				"default_hugepagesz=1G hugepagesz=1GB hugepages=17"

================Sample output log of the testapp execution on command-line interface============
QDMA testapp rte eal init...
EAL: Detected 8 lcore(s)
EAL: Probing VFIO support...
EAL: PCI device 0000:01:00.0 on NUMA socket -1
EAL:   probe driver: 10ee:903f net_qdma
EAL: PCI device 0000:01:00.1 on NUMA socket -1
EAL:   probe driver: 10ee:913f net_qdma
Ethernet Device Count: 1
Logical Core Count: 4
Setting up port :0.
xilinx-app>
xilinx-app>
xilinx-app>
xilinx-app>
xilinx-app> help

Supported Commands:
	reg_read			<port-id>	<bar-num> <address>			:Reads Specified Register
	reg_write			<port-id>	<bar-num> <address> <value>	:Writes Specified Register
	port_init			<port-id>	<queue-base-id> <num-queues> <num ST-queues> <ring-depth> <buffer-size>

	Note:
	- ring-depth supported values  = [256, 512, 768, 1024, 1280, 1536, 2048, 2560, 3072, 4096, 5120, 6144, 8192].
	- buffer-size supported values = [256, 512, 1024, 2048, 3968, 4096, 8192, 9018, 16384].

	port_close			<port-id>
	dma_to_device		<port-id>	<num-queues> <input-filename> <dst-addr> <size> <iterations> :to-xmit
	dma_from_device		<port-id>	<num-queues> <output-filename> <src-addr> <size> <iterations> :to-recv
	reg_dump			<port-id>							: to dump all the valid registers
	queue_dump			<port-id>  <queue-id>				: to dump the queue-context of a queue-number
	desc_dump			<port-id>  <queue-id>				: to dump the descriptor-fields of a queue-number
	load_cmds			<file-name>							: to execute the list of commands from the file
	help
	Ctrl-d	: To quit from this command-line type Ctrl+d

xilinx-app>
==========================================================================================
	Example usage of the above supported commands:

		1) port_init  0  0  32  32 1024 4096
		 This example initializes PF-0 with 32 queues in ST-mode, queue depth or number descriptor is 1024, buffer size 4k bytes.
		 Other example options:
			 port_init  0  0  32  0	  0	1024 4096
				This initializes PF-0 with all 32 queues in MM-mode
			 port_init  0  0  32  16  16 1024 4096
				This initializes PF-0 with First 16-queues in ST-mode and remaining 16-queues configured in MM-mode.

		2) dma_to_device, is use to transmit the data to DMA with following parameters:
			a) port-id:  port-number ( 0 or 1 supported values)
			b) num-queues:  number of queues (this should be less-than equal to the 'testapp --q <number>' driver configured queue-number.
			c) input-file:  valid existing input file, with proper size.
			d) dst-addr: Valid for MM mode only, the destination start address to DMA the data
			e) size:        size of data to be transferred from the above input-file.
			f) iterations:  number of loops to repeat the same transfer.

		3) dma_from_device, is use to transmit the data to DMA with following parameters:
			a) port-id:  port-number ( 0 or 1 supported values)
			b) num-queues:  number of queues (this should be less-than equal to the 'testapp --q <number>' driver configured queue-number.
			c) output-file:  new filename.
			d) src-addr: Valid for MM mode only, the source address to read the data from.
			e) size:        size of data to be transferred from the above input-file.
			f) iterations:  number of loops to repeat the same transfer.

			Note:
				1) In MM-mode, based on the number of queues input to the dma_to_device/dma_from_device,
				the BRAM-size is divided by number of queues, such that every queue get BRAM share region to transfer/receive the data.
				For example, if BRAM-size is 256KB, and 4 queues are input to the dma_to_device/dma_from_device command,
				then every queue gets 64KB region of memory, where Queue-0 gets first region with start address as 0,
				Queue-1 will have the start address as 64KB, then after Queue-2 will have the start address as 128KB and so on.

		4) load_cmds  test_cmds/qdma_mm_cmds.txt
			This will load the list of commands avilable in the test_cmds/qdma_mm_cmds.txt file

		5) Compare the input-file to the output-file as follows, you can open other shell to compare(as the current is in command-line interface):
			$ cmp <input-file> <output-file>

		6) Exit the application by pressing Ctrl+D keys.
==========================================================================================

ENABLING VFs:
-------------
1) To enable VFs for a particular PF device, first bind the PF device to igb_uio module. This creates a max_vfs file entry in /sys/bus/pci/devices/BDF/.
	Then write the number of VFs to enable to this file.
	Eg: for PF device with BDF 01:00.0
		# echo 1 > /sys/bus/pci/devices/0000\:01\:00.0/max_vfs

2) lspci output lists the enabled VF, in the below example device 01:00.4 is the VF associated to 01:00.0

eg: # lspci | grep -i xilinx
01:00.0 Memory controller: Xilinx Corporation Device 903f
01:00.1 Memory controller: Xilinx Corporation Device 913f
01:00.2 Memory controller: Xilinx Corporation Device 923f
01:00.3 Memory controller: Xilinx Corporation Device 933f
01:00.4 Memory controller: Xilinx Corporation Device a03f

3) Now bind the VF devices to igb_uio driver to use it

4) to disable VF device, unbind the VF device and then write 0 to max_vfs file
	Eg:
		# ./usertools/dpdk-devbind.py -u 01:00.4
		# echo 0 > /sys/bus/pci/devices/0000\:01\:00.0/max_vfs

How to pass devargs (per device)
--------------------------------
- Use '-w pci-bdf, key1=value, key2=value' to pass the qdma specific params to any DPDK's test application.
- supported config params are desc_prefetch, cmpt_desc_len, trigger_mode, c2h_byp_mode, h2c_byp_mode

- desc_prefetch: Enable or disable descriptor prefetch on C2H streaming (ST-mode) queues. Default is prefetch disabled.
	Example: "./build/app/qdma_testapp -c 0x1ff -n 4 -w 02:00.0,desc_prefetch=1 -w 02:00.4,desc_prefetch=0"
		In the above example, for device, 02:00.0, descriptor prefetch is enabled on all the C2H queues belonging to this PCIe function.
		For device, 02:00.4, the descriptor prefetch is disabled on all the C2H queues belonging to this PCIe function.

- cmpt_desc_len: Sets the descriptor length of the completion queue. Valid lengths are 8, 16 and 32 bytes. Default length is 8bytes.
	Example: "./build/app/qdma_testapp -c 0x1ff -n 4 -w 02:00.0,cmpt_desc_len=8 -w 02:00.4,cmpt_desc_len=32"
		In the above example, for device, 02:00.0, completion queue descriptor length is set to 8 bytes for all the completion queues belonging to this PCIe function.
		For device, 02:00.4, completion queue descriptor length is set to 32 bytes for all the completion queues belonging to this PCIe function.

- trigger_mode: Possible values for trigger_mode are 0,1,2,3,4,5. Default is TRIG_MODE_USER_TIMER_COUNT.
		0-RTE_PMD_QDMA_TRIG_MODE_DISABLE
		1-RTE_PMD_QDMA_TRIG_MODE_EVERY
		2-RTE_PMD_QDMA_TRIG_MODE_USER_COUNT
		3-RTE_PMD_QDMA_TRIG_MODE_USER
		4-RTE_PMD_QDMA_TRIG_MODE_USER_TIMER
		5-RTE_PMD_QDMA_TRIG_MODE_USER_TIMER_COUNT
	Example: "./build/app/qdma_testapp -c 0x1ff -n 4 -w 02:00.0,nb_pfetch_queues=8,trigger_mode=3

- c2h_byp_mode: Sets the C2H stream mode. Valid values are 0 (Bypass disabled), 1 (Cache bypass mode) and 2 (Simple bypass mode).
				Default is internal mode i.e. bypass disabled.
	Example: ./build/app/qdma_testapp -c 0x1ff -n 4 -w 02:00.0,c2h_byp_mode=1
		In the above example, for device, 02:00.0, Cache bypass mode is set on all the C2H queues belonging to this PCIe function.

- h2c_byp_mode: Sets the H2C bypass mode. Valid values are 0 (Bypass disabled) and 1 (Bypass enabled). Default is Bypass disabled.
	Example: ./build/app/qdma_testapp -c 0x1ff -n 4 -w 02:00.0,c2h_byp_mode=2,h2c_byp_mode=1
		In the above example, for device, 02:00.0, simple bypass mode is set on all the C2H queues belonging to this PCIe function
		and bypass is enabled on all the H2C queues belonging to this PCIe function.
		For the example design, make sure to set User BAR register 0x90 to '3' for Cache bypass and '5' for Simple bypass test
