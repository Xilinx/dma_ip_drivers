***************
PF/VF Shutdown
***************

The PF and VF ports can be independently removed/shutdown by calling ``rte_dev_remove()`` API from the application.

VF shutdown
===========

When a VF port is removed, only the resources associated with the that port is released.
The following sequence diagram shows the shutdown process for a VF.

.. image:: /images/vf_shutdown.png
   :align: center

To freeup all the VF resources, it is required that application explicitly calls ``rte_dev_remove()`` API for the VF port before exiting the application.


PF shutdown
===========

When a PF port is removed, all resources of the PF including that of its associated VFs are released.
PF waits for all the VFs resources to be freed before removing itself.
The following sequence diagram shows the shutdown process for a PF.

.. image:: /images/pf_shutdown.png
   :align: center
