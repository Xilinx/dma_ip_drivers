***************
dma_from_device
***************

dma_from_device is a user application tool provided along with QDMA Linux driver to perform the Card to Host data transfers.

usage: dma_from_device [OPTIONS]

Read via SGDMA, optionally save output to a file

::

  -d (--device) device (defaults to /dev/qdma01000-MM-0)
  -a (--address) the start address on the AXI bus
  -s (--size) size of a single transfer in bytes, default 32.
  -o (--offset) page offset of transfer
  -c (--count) number of transfers, default is 1.
  -f (--file) file to write the data of the transfers
  -h (--help) print usage help and exit
  -v (--verbose) verbose output


::

   [xilinx@]# dma_from_device -d /dev/qdma01000-ST-1 -s 64
   ** Average BW = 64, 0.880221