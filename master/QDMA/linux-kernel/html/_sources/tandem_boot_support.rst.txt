********************
Tandem Boot Support
********************

Xilinx devices can meet 120 ms link training requirement by using Tandem Configuration, a solution that splits the programming image into two stages. The first stage quickly configures the
PCIe endpoint(s) so the endpoint is ready for link training within 120 ms. The second stage then configures the rest of the device. QDMA Linux Driver supports this feature for CPM5 designs

For more details on Tandem Configuration  refer to CMP5_QDMA_Product_Guide_.

.. _CMP5_QDMA_Product_Guide: https://docs.xilinx.com/v/u/2.1-English/pg347-cpm-dma-bridge


Note: Sample instructions are provided below to excercise the Tandem Boot support


=======
Step#1:
=======

Program the stage 1 bitstream to device (JTAG or PROM) and reboot host system

============================================
Step#2: Get the driver and application ready 
============================================

* Get the latest driver code base

* Remove old module from kernel (if already exist)
	$> rmmod qdma-pf

* Compile driver and applications
	$> make -DTANDEM_BOOT_SUPPORTED

* Install (copy) driver and apps to standard locations 
	$> make install

* Load the kernel module
	$> modprobe qdma-pf

* Enable streaming
	$> dma-ctl qdma41000 en_st

=============================
Step#3: Add and start queues
=============================

Note: Only as a sample to show that MM and ST queues can be added and started

* $>dma-ctl qdma41000 q add idx 0 mode mm dir h2c
* $>dma-ctl qdma41000 q add idx 1 mode mm dir c2h
* $>dma-ctl qdma41000 q add idx 2 mode st dir h2c
* $>dma-ctl qdma41000 q start idx 0 dir h2c aperture_sz 4096 //for SBI loads
* $>dma-ctl qdma41000 q start idx 1 dir c2h
* $>dma-ctl qdma41000 q start idx 2 dir h2c

===========================
Step#4: DMA Stage 2 to SBI
===========================

* Enable the slave-boot interface for TandemPROM or DFX designs. This is enabled automatically for TandemPCIe designs.
	– Write 0x29 to the SBI control register 0xF122_0000 (aliased to 0x1_0122_0000 for transactions from the NOC).
	- $> dma-to-device -d /dev/qdma41000-MM-0 -f <st2.pdi> -s <size> -a 0x102100000


===========================================
Step#5: MM DMA to and from PL BRAM to test
===========================================

* $> dma-to-device -d /dev/qdma41000-MM-0 -f /dev/urandom -s 32 -a 0x0
* $> dma-from-device -d /dev/qdma41000-MM-0 -f frombram.txt -s 32 -a 0x0