#include "optimal_scheduler_private.h"
#include <stdio.h>

// this will hold the estimated battery values
// TODO it's quite likely that the number of battery slots will be 
// half the number of harvesting slots (given that half of the cycle
// it's dark).

#define PRINTF(FORMAT, args...) while(0){}
//#define PRINTF printf

static BatterySlot battery_slots[40];
static uint8_t num_battery_slots;

/**
 * The first pass determines the battery slots as periods of time
 * where the battery is monotonous (increasing, decreasing or constant).
 *
 * It also checks for some basic failures.
 */
static int8_t optsched_first_pass(uint32_t batt_0, uint32_t e_min, uint32_t *harvested)
{
  uint16_t harv_i;
  uint8_t batt_i = 0; // this represents the current battery slot
  uint8_t crt_slot_type;
  uint32_t harv_slot_e_cons;
  uint32_t crt_batt_level;

  PRINTF("Running first pass\n");

  crt_batt_level = batt_0;

  // initialise the first battery slot
  battery_slots[0].min_level = batt_0;
  battery_slots[0].max_level = batt_0;
  battery_slots[0].start_slot = 0;
  battery_slots[0].length = 0;
  battery_slots[0].total_e_cons = 0;

  for (harv_i = 0; harv_i < SLOTS_PER_DAY; harv_i ++){
    // determine the e consumption, based on the amount of e harvested
    if (harvested[harv_i] >= E_CONS_MAX) {
      crt_slot_type = BATT_SLOT_CHARGING;
      harv_slot_e_cons = E_CONS_MAX;
    }else
    if (harvested[harv_i] <= e_min) {
      crt_slot_type = BATT_SLOT_DISCHARGING;
      harv_slot_e_cons = e_min;
    }else
    {
      crt_slot_type = BATT_SLOT_CONSTANT;
      harv_slot_e_cons = harvested[harv_i];
    }
    PRINTF("Slot %u, exp harvested %lu. Crt slot type: %u\n",
        harv_i, harvested[harv_i], crt_slot_type);

    // check if a new battery slot must be created
    if (harv_i > 0 && (crt_slot_type != battery_slots[batt_i].type)){
      // are we consuming/harvesting more than battery capacity?
      if (batt_slot_capacity(&battery_slots[batt_i]) > BATT_CAPACITY){
        // TODO handle failure
      }
      battery_slots[batt_i].length = harv_i - battery_slots[batt_i].start_slot;

      // initialise the next slot
      batt_i ++;
      battery_slots[batt_i].min_level = crt_batt_level;
      battery_slots[batt_i].max_level = crt_batt_level;
      battery_slots[batt_i].start_slot = harv_i;
      battery_slots[batt_i].length = 0;
      battery_slots[batt_i].total_e_cons = 0;
    }

    // update the battery slot
    battery_slots[batt_i].type = crt_slot_type;
    /*
     * TODO: there is a problem here because we use unsigned values,
     * if the battery level could reach negative values...
     */
    crt_batt_level += harvested[harv_i] - harv_slot_e_cons;
    if (crt_batt_level < battery_slots[batt_i].min_level){
      battery_slots[batt_i].min_level = crt_batt_level;
    }
    if (crt_batt_level > battery_slots[batt_i].max_level){
      battery_slots[batt_i].max_level = crt_batt_level;
    }
    battery_slots[batt_i].total_e_cons += harv_slot_e_cons;
  }

  // process the last battery slot
  if (battery_slots[batt_i].length == 0){
    battery_slots[batt_i].length = SLOTS_PER_DAY - battery_slots[batt_i].start_slot;
  }

  num_battery_slots = batt_i + 1;

  return 0;
}

/**
 * This function searches the list of battery slots for
 * the slot that has the minimum energy distance from BATT_MIN or _MAX.
 *
 * - in Python, this function was generating a list. However, it 
 *   would imply that we need an additional list of deltas and indices,
 *   which could increase the memory usage beyond limits.
 * - we rely on the fact that these lists should be short so there
 *   shouldn't be a big impact on computation time
 */
static int32_t next_min_delta(start_slot, end_slot, err_type)
{
  int32_t min_delta;
  if (err_type == BATT_ERROR_OVERSPENT)
    min_delta = BATT_MAX - battery_slots[start_slot].max_level;
  else if (err_type == BATT_ERROR_WASTE)
    min_delta = battery_slots[start_slot].min_level - BATT_MIN;

  for (start_slot++;start_slot < end_slot;start_slot++){
    int32_t delta;
    if (err_type == BATT_ERROR_OVERSPENT)
      delta = BATT_MAX - battery_slots[start_slot].max_level;
    else if (err_type == BATT_ERROR_WASTE)
      delta = battery_slots[start_slot].min_level - BATT_MIN;

    if (delta < min_delta) min_delta = delta;
  }

  return min_delta;
}

/**
 * With a given error in energy storage (waste or overspent)
 * this algorithm will adjust the consumption between start_ and
 * end_ battery slots to recover the error.
 *
 * The key here is next_min_list, which represents, for each slot,
 * the maximum delta that can be recovered. Therefore, when 
 * analysing a slot, the algorithm determines how much of the error
 * can be recovered, by looking at next_min_list, get_slot_max_e_* and
 * of course, error.
 *
 * Returns the accumulated changes to the battery level in @batt_delta
 */
static int adjust_energy(uint8_t start_slot,
                         uint8_t end_slot,
                         uint32_t e_min,
                         uint32_t error,
                         uint8_t err_type,
                         int32_t *batt_delta)
{
  PRINTF("Adjust energy [%u, %u]\n", start_slot, end_slot);

  while (start_slot < end_slot){
    uint32_t recoverable;
    int32_t e_cons_change;
    
    // we can only recover in CONSTANT slots or ones opposite to the error
    if ((battery_slots[start_slot].type == BATT_SLOT_CONSTANT) ||
      (err_type == BATT_ERROR_OVERSPENT && (battery_slots[start_slot].type == BATT_SLOT_CHARGING)) ||
      (err_type == BATT_ERROR_WASTE && (battery_slots[start_slot].type == BATT_SLOT_DISCHARGING)))
    {
      switch (err_type){
        case BATT_ERROR_OVERSPENT:
          recoverable = get_slot_max_e_decrease(&battery_slots[start_slot], e_min);
          e_cons_change = min(error,
                            min(next_min_delta(start_slot, end_slot, err_type)
                            - *batt_delta, recoverable));
          break;
        case BATT_ERROR_WASTE:
          recoverable = get_slot_max_e_increase(&battery_slots[start_slot]);
          e_cons_change = min(error,
                            min(next_min_delta(start_slot, end_slot, err_type)
                            + *batt_delta, recoverable));
          break;
      }
      error -= e_cons_change;
      if (err_type == BATT_ERROR_OVERSPENT) e_cons_change = -e_cons_change;
      // update the total e consumption in the battery slot
      battery_slots[start_slot].total_e_cons += e_cons_change;
      // update the battery level in the slot - however, only the 
      // level at the end of the battery slot changes, not at the start
      switch (battery_slots[start_slot].type){
        case BATT_SLOT_CHARGING:
          // last level is max, increase the start with the previous delta
          battery_slots[start_slot].min_level += *batt_delta;
          battery_slots[start_slot].max_level += *batt_delta;
          battery_slots[start_slot].max_level -= e_cons_change;
          break;
        case BATT_SLOT_DISCHARGING:
          // last level is min
          battery_slots[start_slot].max_level += *batt_delta;
          battery_slots[start_slot].min_level += *batt_delta;
          battery_slots[start_slot].min_level -= e_cons_change;
          break;
        case BATT_SLOT_CONSTANT:
          // constant slots can change type for non-zero changes
          // the sign of the change will determine the new type
          // of the slot
          /*
           * if the e_cons_change is large enough so that in all
           * the harvesting slots of this battery slot econs must
           * be maximum or minimum, then (and only then) we change
           * the slot type. otherwise the slot type is kept CONSTANT
           */
          {
          uint8_t type_change = 0;
          if (abs(e_cons_change) == recoverable){
            type_change = 1;
          }
          if (e_cons_change > 0){
            // slot will be decreasing, so start will be max and end, min
            if (type_change) battery_slots[start_slot].type = BATT_SLOT_DISCHARGING;
            battery_slots[start_slot].max_level += *batt_delta;
            battery_slots[start_slot].min_level += *batt_delta;
            battery_slots[start_slot].min_level -= e_cons_change;
          }else{
            // slot will be increasing, from min to max levels
            if (type_change) battery_slots[start_slot].type = BATT_SLOT_CHARGING;
            battery_slots[start_slot].min_level += *batt_delta;
            battery_slots[start_slot].max_level += *batt_delta;
            battery_slots[start_slot].max_level -= e_cons_change;
          }
          break;
          }
      }
      //PRINTF("Updated slot %u: delta_e=%ld, total_e_cons=%lu, min=%lu, max=%lu\n",
      //    start_slot, e_cons_change,
      //    battery_slots[start_slot].total_e_cons,
      //    battery_slots[start_slot].min_level,
      //    battery_slots[start_slot].max_level);

      *batt_delta  -= e_cons_change;
      // TODO this was here... WHY? battery_slots[start_slot].max_level += *batt_delta;
    }else{
      // if we are not making changes in this slot, we still
      // have to effect the ongoing battery changes
      battery_slots[start_slot].min_level += *batt_delta;
      battery_slots[start_slot].max_level += *batt_delta;

      //PRINTF("Updated slot %u, no e changes: min=%lu, max=%lu\n",
      //    start_slot,
      //    battery_slots[start_slot].min_level,
      //    battery_slots[start_slot].max_level);
    }

    if ((battery_slots[start_slot].type == err_type) && 
       (max(batt_slot_wasted_e(&battery_slots[start_slot]),
            batt_slot_missing_e(&battery_slots[start_slot]))
          > (*batt_delta>=0?*batt_delta:-*batt_delta)))
    {
      /* we have a slot where we can't recover, but the changes made to e_cons
       * have made the accumulated error too large
       * TODO I'm wondering if this is a viable case. After all we are only
       * recovering as allowed by (next_min_list - batt_delta).
       */

      // TODO this is where we signal an error
    }

    start_slot ++;
  }

  if (error > 0){
    // we failed, return an error
    PRINTF("Failed, error=%lu\n", error);
    return -1;
  }

  return 0;
}

/**
 * The second pass of the algorithm goes through all the slots
 * and fixes energy waste and overspending errors by increasing
 * or decreasing the energy consumption in the previous slots.
 *
 * It relies on the observation that recovery can only be done
 * as far ahead as the previous error of opposite type, otherwise
 * the changes would have conflicting results.
 *
 * Returns in the @batt_delta pointer the accumulated changes
 * to the battery value.
 * The changes to the energy consumption per slot are stored
 * in the global array.
 */
static int8_t optsched_second_pass(int32_t *batt_delta, uint32_t e_min)
{
  uint8_t list_start, index, max_err_index;
  uint8_t crt_err_type;
  uint32_t max_err;
  int32_t tent_err;

  PRINTF("Starting second pass. %u battery slots\n", num_battery_slots);

  list_start = index = 0;
  crt_err_type = BATT_ERROR_UNSET;
  *batt_delta = tent_err = max_err = 0;
  while (index < num_battery_slots){
    PRINTF("Slot %u type: %u, total_e_cons %lu, min_level %lu (>%lu), max_level %lu (<%lu), length %u, delta=%ld\n", 
        index, battery_slots[index].type,
        battery_slots[index].total_e_cons,
        battery_slots[index].min_level, BATT_MIN,
        battery_slots[index].max_level, BATT_MAX,
        battery_slots[index].length,
        *batt_delta);

    // search for battery slots with errors
    if (battery_slots[index].type == BATT_SLOT_CONSTANT){
      index ++;
      continue;
    }

    if (get_batt_error(&battery_slots[index], *batt_delta + tent_err) > 0){
      PRINTF("Battery error > 0 in slot %u: %lu\n", index,
          get_batt_error(&battery_slots[index], *batt_delta + tent_err));
      if (crt_err_type == BATT_ERROR_UNSET){
        crt_err_type = battery_slots[index].type;
      }else{
        if (crt_err_type !=
              get_batt_error_type(&battery_slots[index], *batt_delta + tent_err))
        {
          // stop at error of opposite type bc. changes cannot be effected after
          // adjust the energy usage in the slots up to the last max error
          if (adjust_energy(list_start, max_err_index+1, e_min,
                            max_err, crt_err_type, batt_delta))
          {
            // TODO report an error somehow
          }
        
          // reset the state
          crt_err_type = battery_slots[index].type;
          list_start = index = max_err_index + 1;
          max_err = tent_err = 0;
          continue;
        }
      }
    }

    // keep track of the maximum error and its index
    if (get_batt_error(&battery_slots[index], *batt_delta) > max_err){
      max_err = get_batt_error(&battery_slots[index], *batt_delta);
      max_err_index = index;
      tent_err = max_err;
      if (crt_err_type == BATT_ERROR_WASTE){
        tent_err = -tent_err;
      }
    }

    index ++;
  }

  // if there were unsolved errors by the end of the list...
  if ((list_start < num_battery_slots) && max_err != 0)
      //(get_batt_error(&battery_slots[num_battery_slots-1], *batt_delta + tent_err) > 0))
  {
    if (adjust_energy(list_start, num_battery_slots, e_min,
                      max_err, crt_err_type, batt_delta))
    {
      // TODO report an error somehow
    }
    /*
     * if we fix until the end of the list, it means that the 
     * batt_delta is applied to all the slots, so we can zero it.
     */
    *batt_delta = 0;
  }

  return 0;
}

/**
 * This is the third step of the algorithm where any offset
 * from the initial battery level is eliminated, so that the
 * node achieves energy-neutral operation.
 */
static int32_t optsched_offset_correction(uint32_t batt_0, uint32_t e_min, int32_t batt_delta)
{
  /*
   * Determine the final offset. 
   * Reduce the offset starting in the final slot and
   * increasing or decreasing e_cons as possible.
   * When moving to the predecessor slot, keep track of 
   * how much that slot battery can be shifted up or down
   * before it violates a battery limitation.
   */
  int32_t offset;
  uint32_t min_delta;
  int16_t index;

  index = num_battery_slots-1;

  PRINTF("Offset correction, index = %u, target = %lu, current = ", index, batt_0);
  // determine the offset from the final battery slot
  if (battery_slots[index].type == BATT_SLOT_DISCHARGING){
    offset = batt_0 - (battery_slots[index].min_level + batt_delta);
    PRINTF("%lu ", battery_slots[index].min_level + batt_delta);
  }else{
    offset = batt_0 - (battery_slots[index].max_level + batt_delta);
    PRINTF("%lu ", battery_slots[index].max_level + batt_delta);
  }
  PRINTF("offset = %ld\n", offset);

  min_delta = offset;

  // start from the end of the battery slot lists
  do{
    int32_t e_change = 0;
    uint32_t slot_start_value;

    // reduce the offset as much as possible in this slot
    if (offset > 0 && is_increasing(&battery_slots[index])){
      e_change = -min(offset, get_slot_max_e_decrease(&battery_slots[index], e_min));
    }else if (offset < 0 && is_decreasing(&battery_slots[index])){
      // negative offset because e_decrease > 0 and offset < 0
      e_change = min(-offset, get_slot_max_e_decrease(&battery_slots[index], e_min));
    }

    offset += e_change;

    /*
     * Apply change.
     * The problem that a change affects all the subsequent battery slots.
     * We can either save all the changes into an array, or
     * effect all the changes on the spot, by going through all the
     * subsequent slots each time.
     * Most likely there will only be a few slots to change,
     * so we choose the latter, to save memory
     *
     * There's a third option - don't change the battery value,
     * because it's not needed anymore! :)
     */
    // change the consumption in this slot
    battery_slots[index].total_e_cons += e_change;

    
    if (offset == 0){
      // done
      break;
    }

    /*
     * Can't recover all the offset here, we have to go back one slot.
     * However, we have to keep track of the distance from the 
     * battery thresholds, so we don't overflow
     * TODO We use the min or max battery level in these calculations,
     * however they may have changed in step 2.
     */
    if (is_increasing(&battery_slots[index])){
      slot_start_value = battery_slots[index].min_level;
    }else{
      slot_start_value = battery_slots[index].max_level;
    }

    if (offset > 0){
      min_delta = min(BATT_MAX - slot_start_value, min_delta);
    }else{
      min_delta = min(slot_start_value - BATT_MIN, min_delta);
    }

    // we can only recover up to min_delta, so don't try for more
    if (min_delta < offset) offset = min_delta;

    PRINTF("In slot %u, change=%ld, min_delta=%lu\n", index, e_change, min_delta);
    index --;
  }while (index >= 0);

  // we only return the final offset
  return offset;
}

/**
 * This step tries to increase the energy consumption
 * in min econs slots to @min_e, without changing
 * intermediate values unless necessary to avoid waste
 * or overspent. Therefore the difference will be in the
 * final offset
 */
static int8_t optsched_min_e_cons(uint32_t min_e)
{
  uint8_t i;
  uint32_t delta = 0; // overall additional E cons

  for (i=0;i<num_battery_slots;i++){
    int32_t change;
    
    /*
     * we are increasing the energy consumption by a positive delta
     * which will reduce the battery level in the remaining slots.
     * we know that it doesn't create overspent errors because
     * the change is no larger than the min delta
     */
    battery_slots[i].min_level -= delta;
    battery_slots[i].max_level -= delta; // if min is ok, max is ok too

    /*
     * how much should we change the econs?
     * can't change more than the minimum offset from BATT_MIN
     * in the remaining slots (including the current).
     * we're not checking here the type of the slot, in CHARGING
     * we ought to check against the max_level. however, CHARGING
     * slots don't need change in min_e_cons, so it's ok
     */
    change = min(min_e*battery_slots[i].length - battery_slots[i].total_e_cons,
                 next_min_delta(i, num_battery_slots, BATT_ERROR_WASTE)-delta);

    // does this slot require changes?
    if (change > 0){
      // yes it does, and we know we can accommodate
      battery_slots[i].total_e_cons += change;
      delta += change;
      // update slot-final value
      switch (battery_slots[i].type){
        case BATT_SLOT_CHARGING:
          // slot-final is max_level
          battery_slots[i].max_level -= change;
          break;
        case BATT_SLOT_DISCHARGING:
        case BATT_SLOT_CONSTANT:
          battery_slots[i].min_level -= change;
          break;
      }
    }
  }

  return 0;
}

int32_t optsched_run(uint32_t battery_start,
                  uint32_t battery_end,
                  uint32_t min_e_cons,
                  uint8_t correct_offset,
                  uint32_t *harvest_prediction)
{
  int32_t battery_delta;


  battery_delta = 0;

  // run first pass
  optsched_first_pass(battery_start, min_e_cons, harvest_prediction);

  // run second pass
  optsched_second_pass(&battery_delta, min_e_cons);

  // correct the offset
  battery_delta = optsched_offset_correction(battery_end, min_e_cons, battery_delta);

  return 0;
}


uint8_t get_number_of_battery_slots()
{
  return num_battery_slots;
}

uint8_t get_battery_slot_length(uint8_t slot_number)
{
  if (slot_number > num_battery_slots
      || slot_number == 0) return 0;
  else return battery_slots[slot_number-1].length;
}

uint32_t get_battery_slot_total_e_cons(uint8_t slot_number)
{
  if (slot_number > num_battery_slots
      || slot_number == 0) return 0;
  else return battery_slots[slot_number-1].total_e_cons;
}

uint8_t get_battery_slot_type(uint8_t slot_number)
{
  if (slot_number > num_battery_slots
      || slot_number == 0) return 255;
  else return battery_slots[slot_number-1].type;
}

uint32_t get_battery_slot_end(uint8_t slot_number)
{
  if (slot_number > num_battery_slots
      || slot_number == 0) return 0;
  switch (battery_slots[slot_number-1].type){
    case BATT_SLOT_CHARGING:
      return battery_slots[slot_number-1].max_level;
    case BATT_SLOT_DISCHARGING:
    case BATT_SLOT_CONSTANT:
      return battery_slots[slot_number-1].min_level;
  }
  return 0;
}

uint32_t get_battery_slot_start_level(uint8_t slot_number)
{
  if (slot_number > num_battery_slots
      || slot_number == 0) return 0;
  switch (battery_slots[slot_number-1].type){
    case BATT_SLOT_CHARGING:
      return battery_slots[slot_number-1].min_level;
    case BATT_SLOT_DISCHARGING:
    case BATT_SLOT_CONSTANT:
      return battery_slots[slot_number-1].max_level;
  }
  return 0;
}

