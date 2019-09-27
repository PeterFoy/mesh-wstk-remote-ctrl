#include "worker.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>

struct worker {
	worker_f exec;
	void *arg;
};


static struct worker pool[NUM_WORKERS];
static woreker_timer_f set_timer = NULL;

static int wid(struct worker* w) {
	return w-pool;
}

static struct worker* walloc(void) {
	struct worker *w;
	for(w = pool; w < &pool[NUM_WORKERS]; w++) {
		if (!w->exec)
			return w;
	}
	return NULL;
}

static void wfree(struct worker* w) {
	w->exec = NULL;
	w->arg = NULL;
}

void workers_init(woreker_timer_f timer) {
	set_timer = timer;
}

void execute_later(worker_f exec, void* arg, uint32_t delay) {
	struct worker* new = walloc();
	if (!new) {
		printf("fatal. no available workers\r\n");
		return;
	}
	new->exec = exec;
	new->arg = arg;
	set_timer(wid(new), delay);
}

void workers_check(uint8_t wid) {
	struct worker *w = &pool[wid];
	w->exec(w->arg);
	wfree(w);
}
