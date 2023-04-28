*******************************
QDMA Function Level Reset (FLR)
*******************************

QDMA supports Function Level Reset (FLR) to enable the function-level reset from the OS.

- When reset is issues for a VF, only the resources associated with the VF is reseted.
-  When a PF is reset, all resources of the PF, including that of its associated VFs, will be reseted


User can request FLR as follows

::

	[xilinx@]# echo 1 > /sys/bus/pci/devices/$BDF/reset
	
Ex: 

::

	[xilinx@]# echo 1 > /sys/bus/pci/devices/0000\:01\:00.0/reset
	


The following sequence diagrams shows the reset process for PF and VF.


.. image:: /images/flr.png
   :align: center
   
   
**NOTE:** FLR is not supported in CentOS because linux kernels provided in CentOS versions does not support the FLR driver call back registration.