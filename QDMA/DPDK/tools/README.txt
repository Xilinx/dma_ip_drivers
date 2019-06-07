0001-PKTGEN-3.6.1-Patch-to-add-Jumbo-packet-support.patch is the patch file
over dpdk-pktgen v3.6.1 that extends dpdk-pktgen application to handle packets
with packet sizes more than 1518 bytes and it disables the packet size
classification logic in dpdk-pktgen to remove application overhead in
performance measurement.

This patch is used for performance testing with dpdk-pktgen application.
