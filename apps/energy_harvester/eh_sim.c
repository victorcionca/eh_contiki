/**
 * author: victorcionca, 22/06/2015
 */


#include "eh_sim.h"
#include "dev/serial-line.h"
#include "sys/etimer.h"

#include <stdio.h>

// How often we poll the outside world for energy values
#ifndef EH_UPDATE_PERIOD
#define EH_UPDATE_PERIOD 60*CLOCK_SECOND
#endif

// We have to limit the maximum harvested energy
#ifndef EH_MAX_LIMIT
#define EH_MAX_LIMIT 10048575UL
#endif

PROCESS(eh_sim_process, "Serial test");
//AUTOSTART_PROCESSES(&eh_sim_process);

uint32_t latest_eh_val;

/**
 * Function to convert strings to ints
 * Returns 0 for success, not zero for error.
 * The converted value is stored in the pointer
 */
static unsigned char
isdigit(char c){
  return (c >= 48) && (c <= 57);
}

static unsigned char
atoi(const char *data, int len, unsigned long *result) {
  unsigned long value = 0;
  int i;
  if (!result) return -1;
  for(i = 0; i < len; i++) {
    if (!data[i] || !isdigit(data[i])) return -1;
    value = value * 10 + data[i] - '0';
  }
  *result = value;

  return 0;
}

void process_data(char *data, int len){
  uint32_t eh_val;

  if (len == 0){
    return;
  }
  if (atoi(data, len, &eh_val)){
    return;
  }else{
    if (eh_val >= EH_MAX_LIMIT) // apply a cap on the maximum harvestable energy
      eh_val = EH_MAX_LIMIT;

    latest_eh_val = eh_val;
    
    printf("Harvested %lu\n", latest_eh_val);
    // notify listeners that there is a new EH value
    process_post(PROCESS_BROADCAST, eh_update_event, &latest_eh_val);

  }
}


PROCESS_THREAD(eh_sim_process, ev, data)
{
  static struct etimer eh_timer;
  unsigned char eh_available = 0;

  PROCESS_BEGIN();
  
  // allocate an ID for the new event
  eh_update_event = process_alloc_event();

  etimer_set(&eh_timer, EH_UPDATE_PERIOD);

  while (1){
    PROCESS_WAIT_EVENT();
    if (ev == serial_line_event_message && data != NULL){
      process_data(data, strlen(data));
      continue;
    }
    if (ev == PROCESS_EVENT_TIMER){
      printf("E#H#\n");
      etimer_reset(&eh_timer);
    }
  }

  PROCESS_END();
}
