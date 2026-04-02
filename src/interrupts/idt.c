#include "interrupts/idt.h"
#include "io/serial_ports.h"
#include "io/io.h"

void init_idt_desc(unsigned short select, unsigned int offset,
                   unsigned short type, struct idt_descriptor *idt_desc) {
  idt_desc->offset_low = (offset & 0xFFFF);
  idt_desc->segment_selector = select;
  idt_desc->type_attr = type;
  idt_desc->reserved = 0;
  idt_desc->offset_high = (offset & 0xFFFF0000) >> 16;
}

void init_idt(struct idt *idt_global, struct idt_descriptor *idt) {
  idt_global->adress = idt;
  idt_global->size = (IDT_NUM_ENTRIES * sizeof(struct idt_descriptor)) - 1;

  init_idt_desc(0x08, (unsigned int)interrupt_handler_33, 0x8E, &idt[33]);
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou handler de teclado");

  load_lidt(idt_global);
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou IDT");

  pic_init();
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou PIC");

  outb(PIC1_PORT_B, 0xFD);
}
