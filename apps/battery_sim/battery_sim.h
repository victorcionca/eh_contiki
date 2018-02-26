#ifndef __BATTERY_SIM
#define __BATTERY_SIM

#include "contiki.h"

PROCESS_NAME(battery_process);
process_event_t battery_update_event;

/**
 * Battery capacity is measured in Watt-Ticks (instead of Watt-seconds = Joules).
 * Tension V = 3V
 */

#ifndef __BATTERY_UPDATE_PERIOD
#define __BATTERY_UPDATE_PERIOD 60*CLOCK_SECOND
#endif

#ifndef __BATTERY_INIT_CAP
//#define __BATTERY_INIT_CAP  32400UL*RTIMER_SECOND    // equivalent to energy stored in 2AA batteries = 3V*2*1.5A*3600s*RTIMER_SECOND(ticks/second)
#define __BATTERY_INIT_CAP  8100UL*RTIMER_SECOND    // equivalent to 880mAh energy = 3V*0.88A*3600s*RTIMER_SECOND(ticks/second)
#endif

#ifndef __NODE_OFF_THRESHOLD
//#define __NODE_OFF_THRESHOLD  16200UL*RTIMER_SECOND
#define __NODE_OFF_THRESHOLD  4050UL*RTIMER_SECOND
#endif

// this can be used to increase energy consumption
#ifndef __BATTERY_CONSUMPTION_FACTOR
#define __BATTERY_CONSUMPTION_FACTOR  1
#warning "Will use battery consumption factor 1"
#endif


/**
 * Get the remaining energy level
 */
unsigned long battery_get();

/**
 * Get the remaining energy level scaled down 
 * to 8bits.
 *
 * To scale down we define the battery consumption resolution
 * as 1 second. There are 2^^15 ticks per second, therefore
 * we divide the number of ticks in the remaining energy by 2^^15.
 */
uint8_t battery_get_8bit();

/**
 * Returns the battery consumption normalised to 256
 */
unsigned int battery_get_cons_norm();

/**
 * Schedule an immediate (1 tick) update of the battery state
 */
void battery_update();
#endif
