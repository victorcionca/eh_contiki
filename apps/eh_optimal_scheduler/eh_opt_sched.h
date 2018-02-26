#ifndef __EH_OPTIMAL_SCHED_H
#define __EH_OPTIMAL_SCHED_H

PROCESS_NAME(eh_optimal_sched);

/**
 * Returns the maximum allowed energy consumption
 * at this moment in time (in this current slot).
 */
uint8_t eh_optimal_sched_max_allowed();

#endif
