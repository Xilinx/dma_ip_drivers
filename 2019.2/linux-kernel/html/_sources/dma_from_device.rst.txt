*********************************
DMA from Device (dma_from_device)
*********************************

dma_from_device is a user application tool provided along with QDMA Linux driver to perform the Card to Host data transfers.

usage: 

::

	[xilinx@]# dma_from_device [OPTIONS]


**OPTIONS**

::

  -d (--device) device path from /dev. Device name is formed as qdmabbddf-<mode>-<queue_number>. Ex: /dev/qdma01000-MM-0
  -a (--address) the start address on the AXI bus
  -s (--size) size of a single transfer in bytes, default 32 bytes.
  -o (--offset) page offset of transfer
  -c (--count) number of transfers, default is 1.
  -f (--file) file to write the data of the transfers
  -h (--help) print usage help and exit
  -v (--verbose) verbose output


Example:

::

   [xilinx@]# dma_from_device -d /dev/qdma01000-ST-1 -s 64
   ** Average BW = 64, 0.880221
   
   
**NOTE**:
For ST C2H, the dma_from_device application only tries to perform the C2H transfers and it does not take care of generating the packets. The Packet generation is user logic specific functionality and users of the QDMA IP and driver shall make sure the user logic functionality is implemented and integrated in the bit stream used and appropriate configuration parameters are being set before executing dma_from_device command.

Ex: Xilinx example design user logic functionality requires the number of packets, packet length and queue id to be configured in user logic specific registers and packet generation needs to be triggered by configuring a specic register. This is subject to change as per the user logic design.