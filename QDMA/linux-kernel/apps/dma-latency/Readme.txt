What is the dma-lat tool?
The intent of the tool is to gather metrics related how much time do packets
take to loopback to the host when a packet is transmitted in the ST H2C direction 
and the ST Traffic Generator loops back the same packet in the ST C2H direction.
The RDTSC timestamp is inserted into the packet that is transmitted in the H2C 
direction at the time of PIDX update. The timestamp at the time when the 
data interrupt is hit is taken and the difference in the timestamps is used to
measure the latency for the packet. The tool measures average, maximum and
minimum latency in CPU tick counts. To obtain the latency numbers in nanosecs,
the numbers reported by the tool need to be divided by the nominal CPU freq.

How to use the tool?
The tool takes in a configuration file as input which contains data such as the
number of queues, packet sizes etc. Please refer to the Sample_dma_latency_config.txt
for more information on what are available the configuraion parameters. 
The command syntax is -
dma-lat -c <config file>

Important Note-
To be able to get correct results for the latency numbers it is necessary that the 
number of CPUs be restricted to 1 and hyperthreading be turned OFF from BIOS. This is 
to ensure that the CPU core that sends out the H2C packet which contains the timestamp 
is the same CPU that receives that data interrupt for the loopback. If this is not done
then the packet might get transmitted from one CPU core with a different TSC timestamp, 
and the interrupt might get hit on another CPU core which would cause an error in the 
measurement. 
