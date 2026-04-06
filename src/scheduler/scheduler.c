#include "scheduler/scheduler.h"
#include "process/process.h"
#include "segment/tss.h"
#include "interrupts/pit.h"
#include "io/serial_ports.h"

static struct PCB *current_pcb;

/* Controle global: 0 = cooperativo, 1 = preemptivo. */
volatile uint8_t preemption_enabled = 0;

static uint32_t next_pid = 1;
static struct PCB *ready_queue[SCHEDULER_READY_QUEUE_CAPACITY];
static uint32_t ready_head;
static uint32_t ready_tail;
static uint32_t ready_len;

/* Politica de escalonamento ativa (FCFS padrao) */
static uint32_t scheduling_policy = SCHEDULER_POLICY_FCFS;

/* Quantum para Round Robin (em ticks do PIT) */
static uint32_t quantum_ticks = 1;

/* Contador de ticks restantes para o processo corrente (RR) */
static uint32_t remaining_ticks;

static uint32_t append_u32(char *buf, uint32_t pos, uint32_t value)
{
    char tmp[10];
    uint32_t len = 0;

    if (value == 0) {
        buf[pos++] = '0';
        return pos;
    }

    while (value > 0 && len < 10) {
        tmp[len++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (len > 0)
        buf[pos++] = tmp[--len];

    return pos;
}

static uint32_t append_literal(char *buf, uint32_t pos, const char *text)
{
    while (*text != '\0')
        buf[pos++] = *text++;
    return pos;
}

static uint32_t predicted_burst_for_sjf(const struct PCB *pcb)
{
    if (pcb == 0)
        return 0xFFFFFFFFU;

    if (pcb->state != PROCESS_STATE_READY)
        return 0xFFFFFFFFU;

    if (pcb->predicted_burst == 0)
        return 0xFFFFFFFFU;

    return pcb->predicted_burst;
}

static void log_sjf_pick(uint32_t pid, uint32_t burst)
{
    char line[96];
    uint32_t pos = 0;

    pos = append_literal(line, pos, "[SJF] Picking PID ");
    pos = append_u32(line, pos, pid);
    pos = append_literal(line, pos, " with Burst ");
    pos = append_u32(line, pos, burst);
    line[pos] = '\0';

    serial_write(SERIAL_COM1_BASE, line);
}

static void scheduler_update_burst_prediction(struct PCB *pcb, uint32_t now_ticks)
{
    uint32_t elapsed_ticks;

    if (pcb == 0)
        return;

    if (now_ticks >= pcb->last_dispatch_tick)
        elapsed_ticks = now_ticks - pcb->last_dispatch_tick;
    else
        elapsed_ticks = 1;

    if (elapsed_ticks == 0)
        elapsed_ticks = 1;

    pcb->burst_time = elapsed_ticks;

    /* Media Exponencial com alpha=0.5: tau(n+1) = 0.5*t(n) + 0.5*tau(n) */
    if (pcb->predicted_burst == 0)
        pcb->predicted_burst = elapsed_ticks;
    else
        pcb->predicted_burst = (pcb->predicted_burst + elapsed_ticks) / 2U;

    if (pcb->predicted_burst == 0)
        pcb->predicted_burst = 1;
}

void log_process_stat(uint32_t pid, uint32_t event_id, uint32_t context_id)
{
    char line[64];
    uint32_t pos = 0;
    uint32_t ticks = pit_get_ticks();
    const char prefix[] = "[STAT] ";

    for (uint32_t i = 0; i < (sizeof(prefix) - 1); i++)
        line[pos++] = prefix[i];

    pos = append_u32(line, pos, pid);
    line[pos++] = ',';
    pos = append_u32(line, pos, ticks);
    line[pos++] = ',';
    pos = append_u32(line, pos, event_id);
    line[pos++] = ',';
    pos = append_u32(line, pos, context_id);
    line[pos] = '\0';

    serial_write(SERIAL_COM1_BASE, line);
}

void log_process_exit(uint32_t pid)
{
    log_process_stat(pid, STAT_EVENT_TERMINATED, STAT_CONTEXT_EXIT);
}

void scheduler_init(void)
{
    current_pcb = 0;
    preemption_enabled = 0;
    next_pid = 1;
    ready_head = 0;
    ready_tail = 0;
    ready_len = 0;
    remaining_ticks = quantum_ticks;
}

void scheduler_set_policy(uint32_t policy)
{
    scheduling_policy = policy;
}

void scheduler_set_quantum(uint32_t ticks)
{
    if (ticks == 0)
        ticks = 1;
    quantum_ticks = ticks;
    remaining_ticks = ticks;
}

uint32_t scheduler_get_quantum(void)
{
    return quantum_ticks;
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
    if (pcb != 0)
        pcb->last_dispatch_tick = pit_get_ticks();
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

/*
 * scheduler_dequeue_sjf: retira da fila o processo com menor predicted_burst.
 * Percorre toda a fila circular para encontrar o menor valor.
 */
static struct PCB *scheduler_dequeue_sjf(void)
{
    uint32_t i;
    uint32_t best_index;
    uint32_t best_burst;
    struct PCB *best_pcb;
    const uint32_t invalid_index = SCHEDULER_READY_QUEUE_CAPACITY;

    if (ready_len == 0)
        return 0;

    /* Seleciona o READY com menor predicted_burst. */
    best_index = invalid_index;
    best_burst = 0xFFFFFFFFU;

    for (i = 0; i < ready_len; i++) {
        uint32_t idx = (ready_head + i) % SCHEDULER_READY_QUEUE_CAPACITY;
        struct PCB *candidate = ready_queue[idx];
        uint32_t candidate_burst = predicted_burst_for_sjf(candidate);

        if (best_index == invalid_index || candidate_burst < best_burst) {
            best_burst = candidate_burst;
            best_index = i;
        }
    }

    if (best_index == invalid_index)
        return 0;

    /* Indice absoluto na fila circular */
    uint32_t abs_index = (ready_head + best_index) % SCHEDULER_READY_QUEUE_CAPACITY;
    best_pcb = ready_queue[abs_index];

    /* Remove o elemento deslocando os posteriores para frente */
    for (i = best_index; i < ready_len - 1; i++) {
        uint32_t dst = (ready_head + i) % SCHEDULER_READY_QUEUE_CAPACITY;
        uint32_t src = (ready_head + i + 1) % SCHEDULER_READY_QUEUE_CAPACITY;
        ready_queue[dst] = ready_queue[src];
    }

    /* Ajusta o tail */
    if (ready_tail == 0)
        ready_tail = SCHEDULER_READY_QUEUE_CAPACITY - 1;
    else
        ready_tail--;
    ready_len--;

    log_sjf_pick(best_pcb->pid, best_pcb->predicted_burst);

    return best_pcb;
}

struct PCB *scheduler_pick_next(void)
{
    switch (scheduling_policy) {
        case SCHEDULER_POLICY_SJF:
            return scheduler_dequeue_sjf();

        case SCHEDULER_POLICY_RR:
            /* RR usa a mesma fila FIFO; a preempcao eh controlada pelo quantum */
            return scheduler_dequeue_ready();

        case SCHEDULER_POLICY_FCFS:
        default:
            return scheduler_dequeue_ready();
    }
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

int scheduler_schedule_from_context(struct process_context *context, int requeue_current, uint32_t context_id)
{
    struct PCB *current;
    struct PCB *next;
    uint32_t outgoing_event;
    uint32_t now_ticks;

    if (context == 0)
        return -1;

    current = scheduler_get_current();
    if (current == 0)
        return -1;

    /*
     * Round Robin: so troca de processo quando o quantum esgota.
     * Se ainda tem ticks restantes, decrementa e permanece no processo atual.
     */
    if (scheduling_policy == SCHEDULER_POLICY_RR && requeue_current) {
        if (remaining_ticks > 1) {
            remaining_ticks--;
            return 0;
        }
        /* Quantum esgotado: reseta e continua para trocar de processo */
        remaining_ticks = quantum_ticks;
    }

    current->context = *context;
    current->eip = context->eip;
    current->esp = context->user_esp;
    current->eflags = context->eflags;
    current->cs = context->cs;
    current->ss = context->user_ss;

    if (requeue_current) {
        now_ticks = pit_get_ticks();
        scheduler_update_burst_prediction(current, now_ticks);
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

    if (context_id != STAT_CONTEXT_EXIT) {
        outgoing_event = (context_id == STAT_CONTEXT_PIT) ? STAT_EVENT_PREEMPTED : STAT_EVENT_YIELDED;
        log_process_stat(current->pid, outgoing_event, context_id);
    }
    log_process_stat(next->pid, STAT_EVENT_SCHEDULED, context_id);

    /* Reset do quantum ao trocar para novo processo (RR) */
    if (scheduling_policy == SCHEDULER_POLICY_RR)
        remaining_ticks = quantum_ticks;

    if (next->pdt != current->pdt)
        update_cr3(next->pdt);

    tss_set_kernel_stack(next->kernel_esp0);

    if (next->context.cs == 0)
        scheduler_init_context_from_pcb(next);

    *context = next->context;
    context->eflags |= 0x00000200;
    return 1;
}
