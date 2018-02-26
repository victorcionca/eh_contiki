all: optsched_test

CONTIKI=

APPDIRS += ../../../apps
APPS+=energy_harvester
APPS+=eh_predictor
APPS+=battery_sim
APPS+=eh_optimal_scheduler
APPS+=periodic_sender

CFLAGS += -DSLOTS_PER_DAY=144
CFLAGS += -DEH_UPDATE_PERIOD=60*CLOCK_SECOND
include $(CONTIKI)/Makefile.include
