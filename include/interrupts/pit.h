#ifndef INCLUDE_PIT_H
#define INCLUDE_PIT_H

#include <stdint.h>
#include "process/process.h"

void pit_init(uint32_t ms);
uint32_t pit_get_ticks(void);
void timer_interrupt_handler(struct process_context *frame);

#endif