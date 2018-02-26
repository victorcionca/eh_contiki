#include "contiki.h"
#include "dev/serial-line.h"
#include "optimal_scheduler.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>

#define PRINTF(FORMAT, args...) while(0){}
//#define PRINTF printf

/*
 * This is a dummy predictor that receives harvested energy values
 * over the serial line, in one block for the whole cycle.
 * Then it generates the eh_update_event.
 *
 * This is only for testing the performnace of the scheduler.
 *
 * The serial protocol is a state machine:
 * <IDLE> state: wait for "SOF" on serial.
 * <READ> state: read integer values as they come.
 *               - stop when receive "EOF"
 *               - if read SLOTS_PER_DAY values, go to next
 *               - if read < SLOTS_PER_DAY values, go to IDLE
 * <PUBLISH> state: send eh_update_event, go to IDLE.
 */

// We have to limit the maximum harvested energy
#ifndef EH_MAX_LIMIT
#define EH_MAX_LIMIT 10048575UL
#endif

PROCESS(eh_pred, "Dummy serial predictor reading full data over serial");
AUTOSTART_PROCESSES(&eh_pred);

static uint32_t cycle_prediction[SLOTS_PER_DAY];
static uint16_t slot_id = 0;

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

static uint32_t process_data(char *data, int len){
  uint32_t eh_val;

  if (len == 0){
    return 0;
  }
  if (atoi(data, len, &eh_val)){
    return 0;
  }else{
    if (eh_val >= EH_MAX_LIMIT) // apply a cap on the maximum harvestable energy
      eh_val = EH_MAX_LIMIT;
  }

  return eh_val;
}

uint32_t eh_pred_get_next_slot()
{
  return cycle_prediction[1];
}

uint8_t eh_pred_get_slot_number()
{
  return 0;
}

uint32_t eh_pred_get_slot(uint8_t slot)
{
  return cycle_prediction[0];
}

uint32_t *eh_pred_get_cycle_prediction()
{
  return cycle_prediction;
}

uint8_t is_sof(char *data)
{
  return data[0] == 'S' && data[1] == 'O' && data[2] == 'F';
}

uint8_t is_eof(char *data)
{
  return data[0] == 'E' && data[1] == 'O' && data[2] == 'F';
}

enum{
  IDLE,
  READ,
  PUBLISH,
};

PROCESS_THREAD(eh_pred, ev, data)
{
  static state;
  PROCESS_BEGIN();
  state = IDLE;
  slot_id = 0;
  memset(cycle_prediction, 0, 4*SLOTS_PER_DAY);

  // configure pin
  P6DIR |= BV(2);
  P6SEL &= ~BV(2);

  P6OUT &= ~BV(2);  // clear the pin
  //P6OUT |= BV(2);   // set the pin

  while (1){
    PROCESS_WAIT_EVENT();

    if (ev == serial_line_event_message && data != NULL){
      switch (state)
      {
        case IDLE:
          slot_id = 0;
          if (is_sof(data)){
            PRINTF("SOF, start reading\n");
            leds_on(LEDS_GREEN);
            state = READ;
          }
          break;
        case READ:
          if (is_eof(data)){
            if (slot_id == SLOTS_PER_DAY){
              PRINTF("EOF\n");
              state = PUBLISH;
              /* No break, flow into PUBLISH */
            }else{
              PRINTF("Missed EOF: %d\n", slot_id);
              leds_toggle(LEDS_RED);
              state = IDLE;
              break;
            }
          }else{
            PRINTF(">%03d\n", slot_id);
            cycle_prediction[slot_id++] = process_data(data, strlen(data));
            break;
          }
        case PUBLISH:
          {
          long int start, end;
          PRINTF("Running alg\n");
          P6OUT |= BV(2);
          start = RTIMER_NOW();
          optsched_run(__BATTERY_INIT_CAP,
                       __BATTERY_INIT_CAP,
                       E_CONS_MIN,
                       0,
                       cycle_prediction);
          end = RTIMER_NOW();
          printf("%ld,%ld,%ld\n", end,start, end-start);
          P6OUT &= ~BV(2);
          leds_off(LEDS_GREEN);
          PRINTF("Done\n");
          state = IDLE;
          }
      }
    }
  }

  PROCESS_END();
}
