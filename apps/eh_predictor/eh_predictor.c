#include "contiki.h"
#include "eh_sim.h"
#include "eh_predictor.h"

PROCESS(eh_pred, "Prediction for energy harvesting");
AUTOSTART_PROCESSES(&eh_pred);


static uint32_t cycle_prediction[SLOTS_PER_DAY];
static uint8_t slot_id = 0;
static uint8_t exp_weight = 100; // EWMA alpha * 100

uint32_t eh_pred_get_next_slot()
{
  return cycle_prediction[(slot_id+1)%SLOTS_PER_DAY];
}

uint8_t eh_pred_get_slot_number()
{
  return slot_id;
}

uint32_t eh_pred_get_slot(uint8_t slot)
{
  return cycle_prediction[slot_id % SLOTS_PER_DAY];
}

uint32_t *eh_pred_get_cycle_prediction()
{
  return cycle_prediction;
}

PROCESS_THREAD(eh_pred, ev, data)
{
  PROCESS_BEGIN();
  slot_id = 0;
  memset(cycle_prediction, 0, 4*SLOTS_PER_DAY);

  while (1){
    PROCESS_WAIT_EVENT();

    if (ev == eh_update_event){
      uint32_t eharv;
      eharv = *(uint32_t*)data;
      // insert the value in the predictor
      cycle_prediction[slot_id] = ((100-exp_weight) * cycle_prediction[slot_id] + exp_weight*eharv)/100;
      slot_id = (slot_id+1)%SLOTS_PER_DAY;
    }
  }

  PROCESS_END();
}
