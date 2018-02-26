# Energy harvesting source
This application takes an energy harvesting trace and supplies values to a network of
nodes over the serial line.

Values are supplied individually to each node - each node has its own client.

In order to synchronise the network time with the host time, the nodes will poll 
the source for values when they need them.

The protocol is as follows:
- periodically a node will poll the source by sending "EH\n"
- the source should reply with an EH value (unsigned int) representing energy
  harvested in the previous period (Joules).


This particular implementation is designed for Cooja simulations, where each node
has a TCP socket allocated, proxying the node's serial line. The source application
will open a server socket to which all the serial clients will connect. Each client will then receive its own handler thread that keeps track of time, etc.

## Usage
~~~
python eh_source_server.py <base-port> <trace-file> <trace-offset> <csc-file> <shader.pickle>
~~~
where:
* __base-port__: TCP port allocated to the first device in the simulation; it is assumed that the ports are allocated incrementally, so device _k_ has _base-port_ + _k_
* __trace-file__: path to the EH trace file
* __trace-offset__: offset in the EH trace file, allows skipping ahead; in number of samples
* __csc-file__: path to the CSC file that defines the network setup
* __shader.pickle__: file defining the shaders that are applied over the network and modulate the EH for every device.
