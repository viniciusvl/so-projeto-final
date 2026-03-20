#include "interrupts/idt.h"

void init_idt_desc(unsigned short select, unsigned int offset,
                   unsigned short type, struct idt_descriptor *idt_desc) {
  idt_desc->offset_low = (offset & 0xFFFF);
  idt_desc->segment_selector = select;
  idt_desc->type_attr = type;
  idt_desc->reserved = 0;
  idt_desc->offset_high = (offset & 0xFFFF0000) >> 16;
}
