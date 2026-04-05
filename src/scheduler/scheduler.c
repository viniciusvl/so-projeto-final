#include "scheduler/scheduler.h"
#include "process/process.h"
#include "segment/tss.h"

static struct PCB *current_pcb;

/* Controle global: 0 = cooperativo, 1 = preemptivo. */
volatile uint8_t preemption_enabled = 0;

static uint32_t next_pid = 1;
static struct PCB *ready_queue[SCHEDULER_READY_QUEUE_CAPACITY];
static uint32_t ready_head;
static uint32_t ready_tail;
static uint32_t ready_len;

void scheduler_init(void)
{
    current_pcb = 0;
    preemption_enabled = 0;
    next_pid = 1;
    ready_head = 0;
    ready_tail = 0;
    ready_len = 0;
}

void kernel_enable_preemption(void)
{
    preemption_enabled = 1;
}

void kernel_disable_preemption(void)
{
    preemption_enabled = 0;
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

struct PCB *scheduler_pick_next(void)
{
    return scheduler_dequeue_ready();
}

uint32_t scheduler_ready_count(void)
{
    return ready_len;
}

static void scheduler_init_context_from_pcb(struct PCB *pcb)
{
    pcb->context.ebp = 0;
    pcb->context.edi = 0;
    pcb->context.esi = 0;
    pcb->context.edx = 0;
    pcb->context.ecx = 0;
    pcb->context.ebx = 0;
    pcb->context.eax = 0;
    pcb->context.eip = pcb->eip;
    pcb->context.cs = pcb->cs;
    pcb->context.eflags = pcb->eflags;
    pcb->context.user_esp = pcb->esp;
    pcb->context.user_ss = pcb->ss;
}

int scheduler_schedule_from_context(struct process_context *context, int requeue_current)
{
    struct PCB *current;
    struct PCB *next;

    if (context == 0)
        return -1;

    current = scheduler_get_current();
    if (current == 0)
        return -1;

    current->context = *context;
    current->eip = context->eip;
    current->esp = context->user_esp;
    current->eflags = context->eflags;
    current->cs = context->cs;
    current->ss = context->user_ss;

    if (requeue_current) {
        current->state = PROCESS_STATE_READY;
        if (scheduler_enqueue_ready(current) < 0) {
            current->state = PROCESS_STATE_RUNNING;
            return 0;
        }
    }

    next = scheduler_pick_next();
    if (next == 0) {
        current->state = PROCESS_STATE_RUNNING;
        return 0;
    }

    scheduler_set_current(next);
    next->state = PROCESS_STATE_RUNNING;

    if (next->pdt != current->pdt)
        update_cr3(next->pdt);

    tss_set_kernel_stack(next->kernel_esp0);

    if (next->context.cs == 0)
        scheduler_init_context_from_pcb(next);

    *context = next->context;
    context->eflags |= 0x00000200;
    return 1;
}
