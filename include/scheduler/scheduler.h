#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_READY_QUEUE_CAPACITY 32

/* Politicas de escalonamento */
#define SCHEDULER_POLICY_FCFS 0
#define SCHEDULER_POLICY_SJF  1
#define SCHEDULER_POLICY_RR   2

/* Telemetria de escalonamento (event IDs) */
#define STAT_EVENT_CREATED    0
#define STAT_EVENT_SCHEDULED  1
#define STAT_EVENT_YIELDED    2
#define STAT_EVENT_PREEMPTED  3
#define STAT_EVENT_TERMINATED 4

/* Contexto de origem do evento */
#define STAT_CONTEXT_NONE     0
#define STAT_CONTEXT_YIELD    1
#define STAT_CONTEXT_PIT      2
#define STAT_CONTEXT_FORK     3
#define STAT_CONTEXT_EXEC     4
#define STAT_CONTEXT_EXIT     5

/*
* Formato de saída serial
* PID, PIT Tick, Event ID, Context ID
*/

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

void scheduler_set_policy(uint32_t policy);
void scheduler_set_quantum(uint32_t ticks);
uint32_t scheduler_get_quantum(void);

void log_process_stat(uint32_t pid, uint32_t event_id, uint32_t context_id);
void log_process_exit(uint32_t pid);

int scheduler_schedule_from_context(struct process_context *context, int requeue_current, uint32_t context_id);

#endif