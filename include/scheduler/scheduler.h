#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_READY_QUEUE_CAPACITY 32

/* Politicas de escalonamento */
#define SCHEDULER_POLICY_FCFS 0
#define SCHEDULER_POLICY_SJF  1
#define SCHEDULER_POLICY_RR   2

struct PCB;
struct process_context;

void scheduler_init(void);
uint32_t scheduler_allocate_pid(void);

void scheduler_set_current(struct PCB *pcb);
struct PCB *scheduler_get_current(void);

int scheduler_enqueue_ready(struct PCB *pcb);
struct PCB *scheduler_dequeue_ready(void);
struct PCB *scheduler_pick_next(void);
uint32_t scheduler_ready_count(void);

void scheduler_set_policy(uint32_t policy);
void scheduler_set_quantum(uint32_t ticks);
uint32_t scheduler_get_quantum(void);

int scheduler_schedule_from_context(struct process_context *context, int requeue_current);

#endif