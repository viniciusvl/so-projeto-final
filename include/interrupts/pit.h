#ifndef INCLUDE_PIT_H
#define INCLUDE_PIT_H

#include <stdint.h>

struct pit_irq_frame {
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t eax;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
};

struct pit_irq_frame_user {
	struct pit_irq_frame common;
	uint32_t user_esp;
	uint32_t user_ss;
};

void pit_init(uint32_t ms);
uint32_t pit_get_ticks(void);
void timer_interrupt_handler(struct pit_irq_frame *frame);

#endif