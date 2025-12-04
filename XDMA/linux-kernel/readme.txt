The files in this directory provide Xilinx PCIe DMA drivers, example software,
and example test scripts that can be used to exercise the Xilinx PCIe DMA IP.

This software can be used directly or referenced to create drivers and software
for your Xilinx FPGA hardware design.

Operating System Support:
=========================
Operating System Architecture: x86_64
Linux kernel:	3.10, 4.18.0, 5.14.0, 5.15, 6.8
RHEL/CentOS:	7.6, 7.7, 7.8, 7.9,
		8.1, 8.2
ALMA OS:	8.10, 9.4
Ubuntu:		16.04.5 LTS, 16.04.6 LTS
		18.04.1 LTS, 18.04.2 LTS, 18.04.4 LTS3, 18.04.5 LTS
		20.04 LTS, 20.04.1 LTS, 22.04 LTS, 24.04 LTS

HW Requirement:
===============
The PCIe DMA supports UltraScale+, UltraScale, Virtex-7 XT and 7 Series Gen2
devices.

Please refer to the Xilinx documentation "PG195 DMA/Bridge Subsystem for PCI
Express" for details of the IP.


Directory and file description:
===============================
 - xdma/: This directory contains the Xilinx PCIe DMA kernel module
       driver files.

 - include/: This directory contains all include files that are needed for
	compiling driver.

 - tests/: This directory contains example application software to exercise the
	provided kernel module driver and Xilinx PCIe DMA IP. This directory
	also contains the following scripts and directories.

	- load_driver.sh:
		This script loads the kernel module and creates the necissary
		kernel nodes used by the provided software.
		The The kernel device nodes will be created under /dev/xdma*.
		Additional device nodes are created under /dev/xdma/card* to
		more easily differentiate between multiple PCIe DMA enabled
		cards. Root permissions will be required to run this script.

	- run_test.sh: 
	- dma_memory_mapped_test.sh, dma_streaming_test.sh:
	- data/:
		run_test.sh runs sample tests on a Xilinx PCIe DMA target and
		returns a pass (0) or fail (1) result.
		This script calls 2 other scripts in the same directory:
		- dma_memory_mapped_test.sh for memory-mapped dma mode
		- dma_streaming_test.sh for streaming dma mode
		The data/ directory contains binary data files that are used
		for DMA data transfers to the Xilinx FPGA PCIe endpoint device
		with the run_test.sh.
		!NOTE!:
		=======
		These scripts are intended for use with the PCIe DMA EXAMPLE
		DESIGN ONLY.

	 - perform_hwcount.sh:
		This script runs hardware performance for XDMA for both Host to
		Card (H2C) and Card to Host (C2H). The result are copied to
		'hw_log_h2c.txt' and hw_log_c2h.txt' text files. 
		For each direction the performance script loops from 64 bytes
		to 4MBytes and generate performance numbers (byte size doubles
		for each loop count).
		You can grep for 'data rate' on those two files to see data
		rate values.
		Data rate values are in percentage of maximum throughput.
		Maximum data rate for x8 Gen3 is 8Gbytes/s, so for a x8Gen3
		design value of 0.81 data rate is 0.81*8 = 6.48Gbytes/s.
		Maximum data rate for x16 Gen3 is 16Gbytes/s, so for a x16Gen3
		design value of 0.78 data rate is 0.78*16 = 12.48Gbytes/s.
		This program can be run on AXI-MM example design.
		AXI-ST example design is a loopback design, both H2C and C2H
		are connected. Running on AXI-ST example design will not
		generate proper numbers.
		If a AXI-ST design is independent of H2C and C2H, performance
		number can be generated. 

	- scripts_mm/
		This directory contains a set of scripts to check basic driver
		loading/unloading and perform dma operations in memory-mapped
		mode.
		Compare with dma_memory_mapped_test.sh, the test is more
		extensive with more dma size and it also utilizes fio tool in
		addition to dma_from/to_device tools.

		- xdma_mm.sh
			top level script.

		- io_sweep.sh, io.sh, unaligned
			dma test via dma_from/to_device

		- fio_test.sh fio_parse_result.sh
			dma test via fio tool

	- scripts_mm/ dependency
		Some test in script_mm/ requires fio tool and python extension

		Install fio:
		- Centos/RHEL: yum install fio
		- Ubuntu: apt install fio

		Install python extension openpyxl, xlrd(version 1.2.0)
			python --version
			pip2 install openpyxl
			pip2 install xlrd=1.2.0

Usage:
  - Change directory to the driver directory.
        cd xdma
  - Compile and install the kernel module driver.
        make install
  - Change directory to the tools directory.
        cd tools
  - Compile the provided example test tools.
        make
  - Load the kernel module driver:
	a. modprobe xdma
	b. using the provided script.
		cd tests
        	./load_driver.sh
  - Run the provided test script to generate basic DMA traffic.
        ./run_test.sh
 	
	For more extensive memory mapped test:
	assume the XDMA FGPA is at pci slot 0000:01:00.0
		cd scripts_mm
		./xdma_mm.sh 0000:01:00.0 | tee /tmp/xdma_mm.log

  - Check driver Version number
        modinfo xdma (or)
        modinfo ../xdma/xdma.ko    

Updates and Backward Compaitiblity:
  - The following features were added to the PCIe DMA IP and driver in Vivado
    2016.1. These features cannot be used with PCIe DMA IP if the IP was
    generated using a Vivado build earlier than 2016.1.
      - Poll Mode: Earlier versions of Vivado only support interrupt mode which
	is the default behavior of the driver.
      - Source/Destination Address: Earlier versions of Vivado PCIe DMA IP
	required the low-order bits of the Source and Destination address to be
	the same.
	As of 2016.1 this restriction has been removed and the Source and
	Destination addresses can be any arbitrary address that is valid for
        your system.

Frequently asked questions:
  Q: How do I uninstall the kernel module driver?
  A: Use the following commands to uninstall the driver.
       - Uninstall the kernel module.
             rmmod -s xdma

  Q: How do I modify the PCIe Device IDs recognized by the kernel module driver?
  A: The xdma/xdma_mod.c file constains the pci_device_id struct that identifies
     the PCIe Device IDs that are recognized by the driver in the following
     format:
         { PCI_DEVICE(0x10ee, 0x8038), },
     Add, remove, or modify the PCIe Device IDs in this struct as desired. Then
     uninstall the existing xdma kernel module, compile the driver again, and
     re-install the driver using the load_driver.sh script.

  Q: By default the driver uses interupts to signal when DMA transfers are
     completed. How do I modify the driver to use polling rather than
     interrupts to determine when DMA transactions are completed?
  A: The driver can be changed from being interrupt driven (default) to being
     polling driven (poll mode) when the kernel module is inserted. To do this
     modify the load_driver.sh file as follows:
        Change: insmod xdma/xdma.ko
        To:     insmod xdma/xdma.ko poll_mode=1
     Note: Interrupt vs Poll mode will apply to all DMA channels. If desired the
     driver can be modified such that some channels are interrupt driven while
     others are polling driven. Refer to the poll mode section of PG195 for
     additional information on using the PCIe DMA IP in poll mode. 

  Q: How to improve the driver performance and/or latency in poll mode?
  A: First of all, get a baseline by measuring performance and/or latency in 
     interrupt mode and standard poll mode. Then see if your design can actually
     provide more than this (i.e. AXI peripherals can supply/consume data immediately,
     either directly or through a buffer). If you have several "slow" peripherals,
     you can try increasing number of C2H or H2C channels. While one channel is
     busy waiting its data, the other channel can do its job. Note that having
     multiple channels if your peripherals can supply/consume data immediately 
     will not increase performance and thus only wastes resources.

     Now, after you tweaked your design and become familiar with its capabilities,
     you can tweak the driver' poll settings which are defined as following:
        /* Advanced Poll Mode flags */
        #define POLL_ENABLE                        (1 << 0)
        #define POLL_ENABLE_THREAD_SCHED_FIFO      (1 << 1)
        #define POLL_DISABLE_THREAD_TO_CPU_BINGING (1 << 2)
        #define POLL_SINGLE_THREAD                 (1 << 3)

    Set POLL_ENABLE_THREAD_SCHED_FIFO for RT kernel to increase poll threads
    priorities. This should improve driver latencies by not allowing lower priority
    tasks to preempt XDMA polling. The possible downside is that lower priority tasks
    may get less CPU time while polling is active.

    Set POLL_DISABLE_THREAD_TO_CPU_BINGING to disable thread to CPU binding in the
    driver. This allows you to manually bind poll threads to certain CPU cores
    like this:
        # Get Poll Thread PID by looking for cmpl_status_th threads
        ps -eaf | grep cmpl_status_th
        # Bind thread to certain CPUs 
        taskset -p <CPU Mask> <Poll Thread PID>
    Alternatively, you can use a tool that dynamically binds poll threads to certain
    CPU cores. The idea behind this tweak is that migrating a task from one CPU core
    to another is an expensive task. Having no task migration can give you a huge
    performance and/or latency benefits on certain systems. Bind userspace threads
    that work with corresponding DMA channels to the corresponding CPUs to get 
    the best result. Use a profiling tool (like lttng) to visualize the scheduling
    details.
    Note that if you set this flag, you really should bind poll threads to certain CPU 
    cores, otherwise you allow the system to do that for you and it probably will not 
    do a very good job.

    Set POLL_SINGLE_THREAD to limit number of poll threads to 1. Having a single 
    thread is the most effective way to interact with XDMA because it eliminates or 
    significantly reduces the amount of context switches and task migrations,
    reducing overhead and giving your workload the maximum CPU time it can get on
    your system. Note that if you set this flag, you probably want your userspace 
    app to interact with XDMA within a single userspace thread too, and you also
    would want to use POLL_DISABLE_THREAD_TO_CPU_BINGING flag and bind both poll
    thread and userspace thread to the same CPU core. Also note that the driver
    effectively becomes single channel, so if you are accessing a "slow" AXI peripheral,
    it will decrease overall performance, and having multiple H2C or C2H channels 
    will not improve the performance in this case.

    Overall, to get the maximum performance you should understand your design
    requirements and implementation details and know what bottlenecks are you
    trying to fix with certain flags. To use all flags:

        Change: insmod xdma/xdma.ko poll_mode=1
        To:     insmod xdma/xdma.ko poll_mode=15
                <add your CPU pinning code>