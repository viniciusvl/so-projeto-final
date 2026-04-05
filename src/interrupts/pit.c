#include "interrupts/pit.h"
#include "interrupts/idt.h"
#include "io/io.h"
#include "process/process.h"
#include "scheduler/scheduler.h"

#define PIT_BASE_FREQUENCY_HZ 1193182U
#define PIT_COMMAND_PORT      0x43
#define PIT_CHANNEL0_PORT     0x40
#define PIT_COMMAND_LOHI_MODE 0x36

static volatile uint32_t pit_ticks;

void pit_init(uint32_t ms)
{
    uint32_t divisor;

    if (ms == 0)
        ms = 1;

    /* divisor = 1193182 / (1000 / ms) */
    divisor = (PIT_BASE_FREQUENCY_HZ * ms) / 1000U;

    if (divisor == 0)
        divisor = 1;

    if (divisor > 0xFFFF)
        divisor = 0xFFFF;

    outb(PIT_COMMAND_PORT, PIT_COMMAND_LOHI_MODE);
    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t pit_get_ticks(void)
{
    return pit_ticks;
}

static void frame_user_to_context(const struct pit_irq_frame_user *frame,
                                  struct process_context *ctx)
{
    ctx->ebp = frame->common.ebp;
    ctx->edi = frame->common.edi;
    ctx->esi = frame->common.esi;
    ctx->edx = frame->common.edx;
    ctx->ecx = frame->common.ecx;
    ctx->ebx = frame->common.ebx;
    ctx->eax = frame->common.eax;
    ctx->eip = frame->common.eip;
    ctx->cs = frame->common.cs;
    ctx->eflags = frame->common.eflags;
    ctx->user_esp = frame->user_esp;
    ctx->user_ss = frame->user_ss;
}

static void frame_context_to_user(struct pit_irq_frame_user *frame,
                                  const struct process_context *ctx)
{
    frame->common.ebp = ctx->ebp;
    frame->common.edi = ctx->edi;
    frame->common.esi = ctx->esi;
    frame->common.edx = ctx->edx;
    frame->common.ecx = ctx->ecx;
    frame->common.ebx = ctx->ebx;
    frame->common.eax = ctx->eax;
    frame->common.eip = ctx->eip;
    frame->common.cs = ctx->cs;
    frame->common.eflags = ctx->eflags;
    frame->user_esp = ctx->user_esp;
    frame->user_ss = ctx->user_ss;
}

void timer_interrupt_handler(struct pit_irq_frame *frame)
{
    struct process_context context;

    pit_ticks++;

    /* Ring 3: frame possui SS/ESP de user e pode ser escalonado. */
    if (preemption_enabled && frame != 0 && (frame->cs & 0x3) == 0x3) {
        struct pit_irq_frame_user *user_frame = (struct pit_irq_frame_user *)frame;

        frame_user_to_context(user_frame, &context);

        if (scheduler_schedule_from_context(&context, 1, STAT_CONTEXT_PIT) > 0)
            frame_context_to_user(user_frame, &context);
    }

    /* EOI no PIC mestre para liberar proximas IRQ0. */
    pic_acknowledge(32);
}
