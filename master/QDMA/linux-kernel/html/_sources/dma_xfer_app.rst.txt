*******************************
DMA Xfer Application (dma-xfer)
*******************************

Xilinx-developed custom application ``dma-xfer`` is an example application provided along with QDMA Linux Driver to demonstrate
QDMA configuration and transfer functionality.

=====
Note:
=====
The name of the application in previous releases before 2020.1 was ``dmaxfer``. If user installed ``dmaxfer`` application aleady in ``/usr/local/sbin`` area, make sure to uninstall the old application(s). Using ``dmaxfer`` aginst latest driver would lead to undefined behaviour and errors may be observed.

``dma-xfer`` application internally takes care of Queues creation/destruction and performs DMA transfers on each queue one by one. If any failure occurs during the initial creation of queues or performing dma transfers on queues, application reports corresponding error and cleans up all the allocated resources then exits.


The following diagram illustrates the design flow of application.

.. image:: /images/qdma_xfer_app_design_flow.png
   :align: center
   
   
**Prerequisites**:

- QDMA Linux kernel driver has to be loaded.
- Proper QMAX value need to set for corresponding Function.

  Example:
  
  echo <queue_count>  > /sys/bus/pci/devices/0000\:BB\:DD.FF/qdma/qmax
  
  <queue_count> is a valid positive number 1 – 2048

**Note**:

Testing/Creation of cmpt queues is outside the scope of this application as transfers are not involved in cmpt queue.

::

	usage: dma-xfer [OPTIONS]

	  -c (--config) config file that has configuration for IO
	  -v (--version), to print version name


dma-xfer application takes a configuration file as input. The configuration file format is as below.

::

	name=mm_sample
	pci_bus=08 #pci bus id
	pci_device=00 #pci device id
	function=0 #fun_id
	is_vf=0 #IS VF ?
	mode=mm #mode
	dir=bi #dir
	q_range=0:0 #no spaces
	tmr_idx=6
	cntr_idx=6
	cmptsz=0
	trig_mode=cntr_tmr
	pfetch_en=0
	rngidx=0
	pkt_sz=1024
	io_type=io_async #type of DMA transfer
	inputfile=INPUT
	outputfile=OUTPUT


**Parameters**:

- name : name of the configuration.
- pci_bus : value as it appears in lspci.
- pci_device: value as it appears in lspci.
- function_id: value as it appears in lspci.
- is_vf: To indicate the target function is PF or VF <0, 1>.
- mode : mode of the queue, streaming or memory mapped <st, mm>.
- dir : Direction of the queue, host-to-card, card-to-host or both <h2c, c2h, bi>.
- q_range : Range of the Queues from 0-2047.
- tmr_idx : Range of index from 0 – 15.
- cntr_idx : Range of index from 0 – 15.
- cmptsz : size of completions, Set 0 to use default value.
- trig_mode : trigger mode <every, usr_cnt, usr, usr_tmr, cntr_tmr, dis>.
- pfetch_en: prefetch mode enable/disable <1 - 0>.
- rngidx : Ring index of CSR register from 0 - 15.
- pkt_sz : packet size, MM mode: <1 – 512MB> ST mode: <1 – 28KB>.
- io_type:  synchronous or asynchronous DMA transfers <io_sync, io_async>.
- inputfile: input file name from where Host to Card dma data taken from <file name of length upto 127>.
- Outputfile: output file name to where Card to Host dma data to be written <file name of length upto 127>.