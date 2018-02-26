This is an experiment to measure the time it takes to run the algorithm on the
Sky mote.

# Design

* test for a range of time slots to determine how the time complexity evolves


# Application

* uses the optimal scheduler directly from the apps folder
* receives a full cycle of eh values over serial
* once it has all the values it runs the algorithm

## Serial protocol

* start a cycle with 'SOF'
* then submit the values one at a time
* end with 'EOF'
* all comms newline terminated

## Parameters used by the algorithm

* B start is full battery capacity
* e cons min is 155