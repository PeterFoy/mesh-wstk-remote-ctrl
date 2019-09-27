#ifndef APP_WORKER_H_
#define APP_WORKER_H_

#include <stdint.h>

#define NUM_WORKERS 8

typedef void (*worker_f)(void *arg);
typedef void (*woreker_timer_f)(uint8_t id, uint32_t ms);

void workers_init(woreker_timer_f timer);
void workers_check(uint8_t id);
void execute_later(worker_f exec, void* arg, uint32_t delay);


#endif /* APP_WORKER_H_ */
