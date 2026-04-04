#include "scheduler/scheduler.h"
#include "process/process.h"

static struct PCB *current_pcb;

static uint32_t next_pid = 1;
static struct PCB *ready_queue[SCHEDULER_READY_QUEUE_CAPACITY];
static uint32_t ready_head;
static uint32_t ready_tail;
static uint32_t ready_len;

void scheduler_init(void)
{
    current_pcb = 0;
    next_pid = 1;
    ready_head = 0;
    ready_tail = 0;
    ready_len = 0;
}

uint32_t scheduler_allocate_pid(void)
{
    uint32_t pid = next_pid;
    next_pid++;

    if (next_pid == 0)
        next_pid = 1;

    return pid;
}

void scheduler_set_current(struct PCB *pcb)
{
    current_pcb = pcb;
}

struct PCB *scheduler_get_current(void)
{
    return current_pcb;
}

int scheduler_enqueue_ready(struct PCB *pcb)
{
    if (pcb == 0)
        return -1;

    if (ready_len >= SCHEDULER_READY_QUEUE_CAPACITY)
        return -1;

    ready_queue[ready_tail] = pcb;
    ready_tail = (ready_tail + 1) % SCHEDULER_READY_QUEUE_CAPACITY;
    ready_len++;
    return 0;
}

struct PCB *scheduler_dequeue_ready(void)
{
    struct PCB *pcb;

    if (ready_len == 0)
        return 0;

    pcb = ready_queue[ready_head];
    ready_head = (ready_head + 1) % SCHEDULER_READY_QUEUE_CAPACITY;
    ready_len--;
    return pcb;
}

uint32_t scheduler_ready_count(void)
{
    return ready_len;
}
