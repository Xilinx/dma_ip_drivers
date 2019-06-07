*************
dma_to_device
*************

dma_to_device is a user application tool provided along with QDMA Linux driver to perform the Host to Card data transfers.

usage: dma_to_device [OPTIONS]

Write via SGDMA, optionally read input from a file.

**Parameters**

::

  -d (--device) device (defaults to /dev/qdma01000-MM-0)
  -a (--address) the start address on the AXI bus
  -s (--size) size of a single transfer in bytes, default 32,
  -o (--offset) page offset of transfer
  -c (--count) number of transfers, default 1
  -f (--data infile) filename to read the data from.
  -w (--data outfile) filename to write the data of the transfers
  -h (--help) print usage help and exit
  -v (--verbose) verbose output

::

   [xilinx@]# dma_to_device -d /dev/qdma01000-ST-1 -s 64
   ** Average BW = 64, 0.880221


