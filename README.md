# Contiki framework for EH experimentation

## Apps
* energy\_harvester:
  * simulates an energy harvester (eg solar panel)
  * receives harvested energy values over the serial line
  * generates an event each time energy is harvested.
* battery\_sim:
  * simulates an energy store with current, maximum and minimum capacity
  * energy consumed is deducted from the current level, based on Energest
  * energy harvested (by listening to the event above) is added to the current level.
* eh\_predictor:
  * uses an EWMA filter to predict the EH for a certain horizon
  * the prediction is accessible for other apps.
* eh\_optimal\_scheduler:
  * implementation of the MAllEC energy consumption scheduler.
* eh\_activity\_prediction:
  * a simple energy consumption scheduler with one slot prediction
  * schedules energy consumption in the current slot to try and maintain the energy
  level at a constant value.
* periodic\_sender:
  * application that periodically sends packets
  * inter packet interval is set to match the maximum allowed energy consumption value
  computed by an algorithm (eg MAllEC).


## Examples
* optimal\_sched\_test: runs the periodic sender app with MAllEC.
* serial\_dummy\_eh\_pred: runs MAllEC on its own based on a full cycle; used for processing time measurements.


## Tools -> sim\_eh\_source

This folder contains a Python application that takes an EH trace and feeds it to Contiki devices implementing the above framework over the serial line.

It relies on devices being simulated in Cooja and uses Cooja's socket server framework for serial line communication.

It works with any number of devices. It takes in the network configuration directly from the Cooja CSC file.

It is able to modulate the EH trace for each device by applying time-dependent shaders.
