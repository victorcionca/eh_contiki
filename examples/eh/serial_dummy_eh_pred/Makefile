PROJECTDIRS=../../../apps/eh_optimal_scheduler
PROJECT_SOURCEFILES=optimal_scheduler.c

all: serial_dummy_eh_pred


TARGET=sky


CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CFLAGS += -I../../../apps/eh_optimal_scheduler
CFLAGS += -DSLOTS_PER_DAY=576UL
CFLAGS += -D__BATTERY_INIT_CAP=1061683200UL
CFLAGS += -D__NODE_OFF_THRESHOLD=530841600UL
include $(CONTIKI)/Makefile.include
