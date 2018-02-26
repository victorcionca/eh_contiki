# Example: dynamic data rate to maximise EH utilisation

Run with the periodic sender app that modulates its data rate (packets per second)
  so as to maximise the utilisation of the allowed energy consumption computed by 
  MAllEC.

## How to run

  First define the CONTIKI macro in the Makefile/

  Start with cooja in command line:
  ~~~
  > java -jar ${CONTIKI}/tools/cooja/dist/cooja.jar -nogui=test.csc -contiki=${CONTIKI}
  ~~~
  replacing test.csc with the CSC file, for example the optimal\_predictor\_test.csc

  Needs an energy harvesting source, use the sim\_eh\_source from the tools folder.
