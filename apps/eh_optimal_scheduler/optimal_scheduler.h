#ifndef __OPT_SCHED_H
#define __OPT_SCHED_H

#include "contiki-conf.h"

/*
 * energy limits per EH slot in Watt-ticks
 * E_CONS_MIN is for when the node only sends its own packets
 * E_CONS_MAX is for when the radio is constanly on
 * TODO E_CONS_MIN should be dynamically defined
 */
#define E_CONS_MIN  155  // defined for EH interval=1min, data=1pkt/min
#define E_CONS_MAX  117964 // same as above

#define BATT_MIN  __NODE_OFF_THRESHOLD
#define BATT_MAX  __BATTERY_INIT_CAP
#define BATT_CAPACITY (BATT_MAX - BATT_MIN)

// this determines the length of arrays such as harvested or battery; < 255
#ifndef SLOTS_PER_DAY
#error "Must define number of slots per day"
#endif


/**
 * Runs the optimal algorithm, given the starting battery value,
 * the desired end battery value and the expected amount of
 * harvested energy.
 *
 * It keeps the minimum energy at min_e_cons.
 * If @correct_offset is true the algorithm will attempt to 
 * reduce the final offset.
 *
 * Returns the difference between desired end battery and 
 * achieved end battery, battery_end - final_battery.
 */
int32_t optsched_run(uint32_t battery_start, 
                     uint32_t battery_end,
                     uint32_t min_e_cons,
                     uint8_t correct_offset,
                     uint32_t *harvest_prediction);

uint8_t get_number_of_battery_slots();
uint8_t get_battery_slot_length(uint8_t slot_number);
uint32_t get_battery_slot_total_e_cons(uint8_t slot_number);
uint8_t get_battery_slot_type(uint8_t slot_number);
uint32_t get_battery_slot_start_level(uint8_t slot_number);

#endif
