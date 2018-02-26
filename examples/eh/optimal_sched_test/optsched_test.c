#include "contiki.h"
#include "eh_sim.h"
#include "eh_predictor.h"
#include "eh_opt_sched.h"
#include "battery_sim.h"
#include "periodic_sender.h"


PROCESS(optsched_test, "Test for the EH optimal scheduler");
AUTOSTART_PROCESSES(&optsched_test, &eh_sim_process, &eh_pred, &battery_process, &eh_optimal_sched, &periodic_sender);

PROCESS_THREAD(optsched_test, event, data)
{
  PROCESS_BEGIN();

  while (1){
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
