#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_READY_QUEUE_CAPACITY 32

struct PCB;
struct process_context;

extern volatile uint8_t preemption_enabled;

void scheduler_init(void);
uint32_t scheduler_allocate_pid(void);

void kernel_enable_preemption(void);
void kernel_disable_preemption(void);

void scheduler_set_current(struct PCB *pcb);
struct PCB *scheduler_get_current(void);

int scheduler_enqueue_ready(struct PCB *pcb);
struct PCB *scheduler_dequeue_ready(void);
struct PCB *scheduler_pick_next(void);
uint32_t scheduler_ready_count(void);

int scheduler_schedule_from_context(struct process_context *context, int requeue_current);

#endif