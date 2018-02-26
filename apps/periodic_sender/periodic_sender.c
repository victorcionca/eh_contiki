#include "contiki.h"
#include "net/rime/rime.h"
#include "eh_sim.h"
#include "eh_sched_interface.h"

#include <stdio.h>

#define PERIOD  10*CLOCK_SECOND

PROCESS(periodic_sender, "Periodically broadcasts packets");

static struct etimer et;
static struct broadcast_callbacks cbacks = {};
static struct broadcast_conn broadcast;
PROCESS_THREAD(periodic_sender, ev, data)
{
  static uint32_t period;
  period = PERIOD;

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &cbacks);
  etimer_set(&et, PERIOD);

  while (1){
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER){
      packetbuf_copyfrom("Hello", 6);
      broadcast_send(&broadcast);
      printf(">\n");
      etimer_reset(&et);
    }else if (ev == eh_update_event){
      uint32_t max_allowed_econs;
      
      // first shut down timer
      etimer_stop(&et);
      etimer_set(&et, CLOCK_SECOND);

      // determine max allowed econs
      max_allowed_econs = eh_sched_get_max_allowed();

      // convert to period
      if (max_allowed_econs <= 2887){
        // sleep and wait for next eh interval
        printf("Period 0\n");
        etimer_stop(&et);
        continue;
      }

      if (max_allowed_econs >= 111900){
        period = 30;
      }else{
        period = (8522UL*128)/(max_allowed_econs - 2887UL);
      }

      if (period < 30) period = 30;

      printf("Period %lu\n", period);
      etimer_set(&et, period);
    }
  }

  PROCESS_END();
}
