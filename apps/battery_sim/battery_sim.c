#include "contiki.h"
#include "contiki-net.h" // for NETSTACK_MAC
#include <stdio.h>
#include "../energy_harvester/eh_sim.h"
#include "battery_sim.h"

/**
 * Battery capacity is measured in Watt-Ticks (instead of Watt-seconds = Joules).
 * Tension V = 3V
 */

#define DEBUG 1

#ifdef DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(battery_process, "Battery process");
//AUTOSTART_PROCESSES(&battery_process);


struct energy_time {
  long cpu;
  long lpm;
  long transmit;
  long listen;
};

static struct etimer et;
static struct energy_time last;
static struct energy_time diff;
static unsigned long battery_capacity = __BATTERY_INIT_CAP;     // expressed in watt*ticks to avoid floating point

unsigned long battery_get()
{
  return battery_capacity;
}

uint8_t battery_get_8bit()
{
  unsigned long temp, batt_max;
  temp = battery_capacity>>15;
  batt_max = __BATTERY_INIT_CAP>>15;
  temp = (temp*255)/batt_max;
  return (uint8_t) temp;
}

unsigned int battery_get_cons_norm()
{
  //return (__BATTERY_INIT_CAP - battery_capacity)/__BATTERY_INIT_CAP*256;
  // There are 2^15 ticks per second so
  // we consider this value as battery consumption resolution.
  // Capped at 256
  return (((__BATTERY_INIT_CAP - battery_capacity) >> 15) & 0x000000FF);
}

/**
 * Hurry up and update the battery value
 */
void battery_update()
{
  PROCESS_CONTEXT_BEGIN(&battery_process);
  etimer_stop(&et);
  etimer_set(&et, CLOCK_SECOND);
  PROCESS_CONTEXT_END(&battery_process);
}

PROCESS_THREAD(battery_process, ev, data)
{
  
  PROCESS_BEGIN();
  PRINTF("[BATT]: started\n");

  energest_flush();
  /* Energy time init */
  last.cpu = energest_type_time(ENERGEST_TYPE_CPU);
  last.lpm = energest_type_time(ENERGEST_TYPE_LPM);
  last.transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  last.listen = energest_type_time(ENERGEST_TYPE_LISTEN);

  PRINTF("[BATT] Energy remaining: %lu\n", battery_capacity);
  etimer_set(&et, __BATTERY_UPDATE_PERIOD);
  
  while(1) {
    unsigned long consumed;

    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER){
      /* Energy time diff */
      diff.cpu = energest_type_time(ENERGEST_TYPE_CPU) - last.cpu;
      diff.lpm = energest_type_time(ENERGEST_TYPE_LPM) - last.lpm;
      diff.transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT) - last.transmit;
      diff.listen = energest_type_time(ENERGEST_TYPE_LISTEN) - last.listen;
      last.cpu = energest_type_time(ENERGEST_TYPE_CPU);
      last.lpm = energest_type_time(ENERGEST_TYPE_LPM);
      last.transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
      last.listen = energest_type_time(ENERGEST_TYPE_LISTEN);

      PRINTF("[BATT] Ticks diff: CPU=%ld  LPM=%ld  TX=%ld  RX=%ld\n",
          diff.cpu, diff.lpm, diff.transmit, diff.listen);
      consumed = diff.cpu/1000*4 + diff.lpm/1000000UL*20 + diff.transmit/1000*20 + diff.listen/1000*20; // ampere-ticks
      consumed *=3; // watt-ticks
      consumed *= __BATTERY_CONSUMPTION_FACTOR;
      battery_capacity -= consumed;

      if (battery_capacity < __NODE_OFF_THRESHOLD){
        // the node stops functioning, there's not enough energy
        PRINTF("[BATT] DEAD\n");
        NETSTACK_MAC.off(0);
      }

      if (battery_capacity <= 0){
        battery_capacity = 0;
      }

      PRINTF("[BATT] Energy remaining: %lu. Until threshold: %ld\n", battery_capacity, battery_capacity - __NODE_OFF_THRESHOLD);

      etimer_set(&et, __BATTERY_UPDATE_PERIOD);

      // broadcast a battery update event
      process_post(PROCESS_BROADCAST, battery_update_event, NULL);
    }/*else if (ev == eh_update_event){
      unsigned long eharv = *(unsigned long*)data;
      // energy harvested to be added to the battery
      if (battery_capacity < __NODE_OFF_THRESHOLD - eharv){
        // node is back
        NETSTACK_MAC.on();
      }
      battery_capacity += eharv;
      if (battery_capacity > __BATTERY_INIT_CAP){
        eharv -= battery_capacity - __BATTERY_INIT_CAP;
        battery_capacity = __BATTERY_INIT_CAP;
        PRINTF("[BATT] eharv would exceed capacity\n");
      }
      PRINTF("[BATT] Adding %lu\n", eharv);
    }*/
  }

  PROCESS_END();
}
