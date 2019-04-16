The files in this directory provide Xilinx PCIe DMA drivers, example software,
and example test scripts that can be used to exercise the Xilinx PCIe DMA IP.

This software can be used directly or referenced to create drivers and software
for your Xilinx FPGA hardware design.

Directory and file description:
===============================
 - xdma/: This directory contains the Xilinx PCIe DMA kernel module
       driver files.

 - libxdma/: This directory contains support files for the kernel driver module,
	which interfaces directly with the XDMA IP.

 - include/: This directory contains all include files that are needed for
	compiling driver.

 - etc/: This directory contains rules for the Xilinx PCIe DMA kernel module
	and software. The files in this directory should be copied to the /etc/
	directory on your linux system.

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
		This script runs sample tests on a Xilinx PCIe DMA target and
		returns a pass (0) or fail (1) result.
		This script is intended for use with the PCIe DMA example
		design.

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
	- data/:
		This directory contains binary data files that are used for DMA
		data transfers to the Xilinx FPGA PCIe endpoint device.

Usage:
  - Change directory to the driver directory.
        cd xdma
  - Compile and install the kernel module driver.
        make install
  - Change directory to the tools directory.
        cd tools
  - Compile the provided example test tools.
        make
  - Copy the provided driver rules from the etc directory to the /etc/ directory
    on your system.
        cp ../etc/udev/rules.d/* /etc/udev/rules.d/
  - Load the kernel module driver:
	a. modprobe xdma
	b. using the provided script.
		cd tests
        	./load_driver.sh
  - Run the provided test script to generate basic DMA traffic.
        ./run_test.sh
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
       - Delete the dma rules that were added.
             rm -f /etc/udev/rules.d/60-xdma.rules
             rm -f /etc/udev/rules.d/xdma-udev-command.sh

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
