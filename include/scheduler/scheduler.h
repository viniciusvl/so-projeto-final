#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_READY_QUEUE_CAPACITY 32

struct PCB;

void scheduler_init(void);
uint32_t scheduler_allocate_pid(void);

void scheduler_set_current(struct PCB *pcb);
struct PCB *scheduler_get_current(void);

int scheduler_enqueue_ready(struct PCB *pcb);
struct PCB *scheduler_dequeue_ready(void);
struct PCB *scheduler_pick_next(void);
uint32_t scheduler_ready_count(void);

#endif