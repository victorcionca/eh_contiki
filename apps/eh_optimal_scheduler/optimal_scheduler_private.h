#ifndef __OPT_SCHED_PRIVATE_H
#define __OPT_SCHED_PRIVATE_H

#include "contiki-conf.h"
#include "optimal_scheduler.h"
#include "../battery_sim/battery_sim.h"

enum{
  BATT_SLOT_CHARGING = 0,
  BATT_SLOT_CONSTANT,
  BATT_SLOT_DISCHARGING,
};

enum{
  BATT_ERROR_WASTE = 0,
  BATT_ERROR_OVERSPENT = 2,
  BATT_ERROR_UNSET=255,
};

typedef struct battery_slot{
  uint8_t type;       // a BATT_SLOT_ type
  uint8_t start_slot; // index of harvesting slot when this battery slot begins
  uint8_t length;     // length (in harvesting slots)
  uint32_t min_level;
  uint32_t max_level;
  uint32_t total_e_cons;  // amount of energy consumed in this slot
} BatterySlot;

#define max(A, B) ((A)>=(B)?(A):(B))
#define min(A, B) ((A)<(B)?(A):(B))
#define abs(A)    ((A)>=0?(A):-(A))

/**
 * This determines the maximum error (overcharge or depletion)
 * for a battery slot, given a certain battery delta @delta_b
 */
inline uint32_t get_batt_error(BatterySlot *slot, int32_t delta_b){
  int32_t wasted, missed;
  wasted = slot->max_level - BATT_MAX;
  missed = BATT_MIN - slot->min_level;
  return max(0, max(wasted+delta_b, missed - delta_b));
}

inline uint8_t get_batt_error_type(BatterySlot *slot, int32_t delta_b){
  int32_t wasted, missed;
  wasted = slot->max_level - BATT_MAX;
  missed = BATT_MIN - slot->min_level;

  if (wasted + delta_b > 0) return BATT_SLOT_CHARGING;
  else if (missed - delta_b > 0) return BATT_SLOT_DISCHARGING;

  return BATT_SLOT_CONSTANT;
}

/**
 * Determines how much extra energy can be consumed in this slot
 */
inline uint32_t get_slot_max_e_increase(BatterySlot *slot){
  return slot->length*E_CONS_MAX - slot->total_e_cons;
}

/**
 * Determines how much extra energy can be saved in this slot
 */
inline uint32_t get_slot_max_e_decrease(BatterySlot *slot, uint32_t e_min){
  return slot->total_e_cons - slot->length*e_min;
}

#define batt_slot_capacity(S) ((S)->max_level - (S)->min_level)
#define batt_slot_wasted_e(S) ((S)->max_level - BATT_MAX)
#define batt_slot_missing_e(S) (BATT_MIN - (S)->min_level)

#define is_increasing(SLOT) ((SLOT)->type == BATT_SLOT_CONSTANT || (SLOT)->type == BATT_SLOT_CHARGING)
#define is_decreasing(SLOT) ((SLOT)->type == BATT_SLOT_CONSTANT || (SLOT)->type == BATT_SLOT_DISCHARGING)

#endif
