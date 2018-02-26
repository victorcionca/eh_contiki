#ifndef __EH_PRED_H
#define __EH_PRED_H

PROCESS_NAME(eh_pred);
/**
 * Returns the prediction for the next 
 * time slot.
 */
uint32_t eh_pred_get_next_slot();

/**
 * Returns the prediction for slot number
 */
uint32_t eh_pred_get_slot(uint8_t slot);

/**
 * Retrieves the number of the slot in the
 * day (0 -> SLOTS_PER_DAY)
 */
uint8_t eh_pred_get_slot_number();

/**
 * Returns a pointer to the prediction for
 * the entire cycle
 */
uint32_t *eh_pred_get_cycle_prediction();
#endif
