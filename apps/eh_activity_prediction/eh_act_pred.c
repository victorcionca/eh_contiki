#include "contiki.h"
#include "contiki-conf.h"
#include <eh_sim.h>
#include <battery_sim.h>
#include "eh_sched_interface.h"

PROCESS(eh_act_pred, "Activity prediction for energy harvesting");
AUTOSTART_PROCESSES(&eh_act_pred);

#ifndef EH_SET_POINT
#define EH_SET_POINT 1061683200UL
//#error "The energy harvesting set point needs to be pre-defined"
#endif

#define E_CONS_MAX  117964
#define E_CONS_MIN  155
uint32_t crt_max_allowed = -1;
static uint8_t crt_max_allowed_8bit;

unsigned int eh_act_pred_max_allowed()
{
  if (crt_max_allowed == -1){
    return 255;
  }else{
    // max is 2^20-1, so we scale down to 8bits
    return crt_max_allowed >> 12;
  }
}

uint32_t eh_sched_get_max_allowed()
{
  return crt_max_allowed;
}

uint8_t eh_sched_get_max_allowed_8bit()
{
  return (crt_max_allowed-E_CONS_MIN)*21/10000; // resolution is 461W-ticks
}

static uint32_t
get_max_allowed(uint32_t eharv)
{
  int32_t max_allowed;
  uint32_t battery_crt;

  battery_crt = battery_get();

  printf("set point=%lu\n", EH_SET_POINT);
  printf("battery_crt=%lu\n", battery_crt);
  printf("harvested=%lu\n", eharv);

  max_allowed = battery_crt + eharv - EH_SET_POINT;
  printf("Max allowed = %ld\n", max_allowed);

  if (max_allowed < 0){
    crt_max_allowed = 0;
  }else{
    crt_max_allowed = max_allowed;
    if (crt_max_allowed > E_CONS_MAX) crt_max_allowed = E_CONS_MAX;
  }

  if (crt_max_allowed < E_CONS_MIN) crt_max_allowed = E_CONS_MIN;

  printf("Allowed %lu\n", crt_max_allowed);
  return crt_max_allowed;
}

PROCESS_THREAD(eh_act_pred, ev, data)
{
  static uint32_t eharv;
  PROCESS_BEGIN();

  mallec_event = process_alloc_event();
  while (1){
    PROCESS_WAIT_EVENT();

    if (ev == eh_update_event){
      eharv = *(uint32_t*)data;
      
      // determine allowed econs
      get_max_allowed(eharv);

      crt_max_allowed_8bit = eh_sched_get_max_allowed_8bit();

      // notify everyone
      process_post(PROCESS_BROADCAST, mallec_event, &crt_max_allowed_8bit);
    }
  }

  PROCESS_END();
}
