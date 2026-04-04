#include "interrupts/pit.h"
#include "interrupts/idt.h"
#include "io/io.h"
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

void timer_interrupt_handler(struct process_context *frame)
{
    pit_ticks++;

    /* Preempta apenas quando a interrupcao veio de ring 3. */
    if (frame != 0 && (frame->cs & 0x3) == 0x3)
        scheduler_schedule_from_context(frame, 1);

    /* EOI no PIC mestre para liberar proximas IRQ0. */
    pic_acknowledge(32);
}
