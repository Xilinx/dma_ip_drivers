***************
PF/VF Shutdown
***************

QDMA design contains PFs and VFs and they can be shutdown independently as QDMA Linux driver contains seperate modules for VFs (qdma_vf.ko) and PFs (qdma.ko)

- When shutdown is issues for a VF, only the resources associated with the VFs is released and VF driver  is unloaded
- When shutdown is issued for PF, all resources of the PF, including that of its associated VFs, will be released


The following sequence diagrams shows the shutdown process for PF and VF.


.. image:: /images/pf_vf_shutdown.png
   :align: center