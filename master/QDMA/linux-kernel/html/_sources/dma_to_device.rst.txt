*****************************
DMA to Device (dma-to-device)
*****************************

dma-to-device is a user application tool provided along with QDMA Linux driver to perform the Host to Card data transfers.

=====
Note:
=====
The name of the application in previous releases before 2020.1 was ``dma_to_device``. If user installed ``dma_to_device`` application aleady in ``/usr/local/sbin`` area, make sure to uninstall the old application(s). Using ``dma_to_device`` aginst latest driver would lead to undefined behaviour and errors may be observed.


usage: 

::

	[xilinx@]# dma-to-device [OPTIONS]


**OPTIONS**

::

  -d (--device) device path from /dev. Device name is formed as qdmabbddf-<mode>-<queue_number>. Ex: /dev/qdma01000-MM-0
  -a (--address) the start address on the AXI bus
  -s (--size) size of a single transfer in bytes, default 32 bytes
  -o (--offset) page offset of transfer
  -c (--count) number of transfers, default 1
  -f (--data input file) filename to read the data from.
  -w (--data output file) filename to write the data of the transfers
  -h (--help) print usage help and exit
  -v (--verbose) verbose output

Example:

::

   [xilinx@]# dma-to-device -d /dev/qdma06000-MM-0 -s 64
   size=64 Average BW = 375.194937 KB/sec	
