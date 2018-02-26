#include "contiki.h"
#include <stdio.h>
#include <eh_sim.h>
#include <battery_sim.h>
#include <eh_predictor.h>
#include "eh_opt_sched.h"
#include "optimal_scheduler.h"
#include "eh_sched_interface.h"

PROCESS(eh_optimal_sched, "Activity prediction for energy harvesting");
AUTOSTART_PROCESSES(&eh_optimal_sched);


static uint8_t current_battery_slot;   // battery slot
static uint8_t remaining_slots; // battery slot length
static int32_t remaining_energy; // in the battery slot
static uint32_t crt_max_allowed = -1;
static uint8_t crt_max_allowed_8bit = -1;

uint32_t eh_sched_get_max_allowed()
{
  return crt_max_allowed;
}

uint8_t eh_sched_get_max_allowed_8bit()
{
  return (crt_max_allowed-E_CONS_MIN)*21/10000; // resolution is 461W-ticks
}

uint8_t eh_act_pred_max_allowed()
{
  if (crt_max_allowed == -1){
    return 255;
  }else{
    // max is 2^20-1, so we scale down to 8bits
    return crt_max_allowed >> 12;
  }
}

PROCESS_THREAD(eh_optimal_sched, ev, data)
{
  static uint32_t min_e_cons;
  static uint32_t sum_eh_pred_in_slot;
  PROCESS_BEGIN();
  remaining_slots = 0;

  // init mallec event
  mallec_event = process_alloc_event();

  /*
   * min e cons is sending 1 packet/min
   */
  min_e_cons = E_CONS_MIN; 
  printf("Min e cons %lu\n", min_e_cons);

  while (1){
    PROCESS_WAIT_EVENT();

    /*
     * We need to listen for eh_update events
     * so that we can determine when we start
     * a cycle
     */
    if (ev == eh_update_event){
      uint32_t current_battery;
      uint8_t slot_id;
      uint32_t e_cons_est_per_slot;
      uint32_t b_i_est;

      slot_id = eh_pred_get_slot_number();
      current_battery = battery_get();

      if (slot_id == 0){
        // generate optimal schedule
        optsched_run(current_battery,
                     BATT_MAX,
                     min_e_cons,
                     0,   // run without offset correction
                     eh_pred_get_cycle_prediction());
        current_battery_slot = 0;
        remaining_slots = 0;

        // print the number of battery slots
        printf("Number of battery slots in this cycle: %u\n", 
            get_number_of_battery_slots());
      }

      if (remaining_slots == 0){
        // we are starting a new battery slot
        current_battery_slot ++;

        // OLD remaining_energy = get_battery_slot_total_e_cons(current_battery_slot);
        sum_eh_pred_in_slot = 0;

        // print the max e cons in this slot
        printf("Total energy available in this slot: %lu\n", remaining_energy);

        printf("Slot type is %u\n", get_battery_slot_type(current_battery_slot));

        // keep track of the next battery slot
        remaining_slots = get_battery_slot_length(current_battery_slot);
        printf("Next battery slot starts at %u\n", slot_id+remaining_slots);
      }

      // 1. determine batt(i) estimate
      e_cons_est_per_slot = get_battery_slot_total_e_cons(current_battery_slot)/
                            get_battery_slot_length(current_battery_slot);
      b_i_est = get_battery_slot_start_level(current_battery_slot) + 
                sum_eh_pred_in_slot -
                e_cons_est_per_slot *
                  (get_battery_slot_length(current_battery_slot) - remaining_slots);

      // 2. adjust remaining energy to account for eh_pred and eh_cons errs
      remaining_energy = e_cons_est_per_slot * remaining_slots +
                         current_battery - b_i_est;

      printf("Estimated %lu actual %lu.", b_i_est, current_battery);
      printf("Slot start %lu, sum_eh_pred %lu, e_cons_p_s %lu\n",
          get_battery_slot_start_level(current_battery_slot),
          sum_eh_pred_in_slot,
          e_cons_est_per_slot);


      // determine allowed econs
      if (remaining_energy < (int32_t)remaining_slots * E_CONS_MIN){
        // not enough energy until the end of the battery slot
        crt_max_allowed = E_CONS_MIN;
      }else{
        crt_max_allowed = remaining_energy/remaining_slots;
      }
      crt_max_allowed_8bit = eh_sched_get_max_allowed_8bit();
      printf("Allowed %lu =%ld/%u\n", crt_max_allowed, remaining_energy, remaining_slots);
      process_post(PROCESS_BROADCAST, mallec_event, &crt_max_allowed_8bit);

      /* --------------------- OLD ECONS ADAPTATION -------------- */
#if 0
      remaining_slots --;
      remaining_energy -= crt_max_allowed;
      // change the remaining e_cons accounting for prediction errors
      remaining_energy += *(uint32_t*)data - eh_pred_get_slot(slot_id);
#endif
      /* -------------------------------------------------------- */

      // 3. add the eh_pred to the tracking sum
      sum_eh_pred_in_slot += eh_pred_get_cycle_prediction(); 
      remaining_slots --;
    }
  }

  PROCESS_END();
}
