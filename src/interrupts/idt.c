#include "interrupts/idt.h"
#include "io/serial_ports.h"
#include "io/io.h"

#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define IDT_TYPE_KERNEL_PRIVILEGED 0x8E
#define IDT_TYPE_USER_PRIVILEGED 0xEE

void init_idt_desc(unsigned short select, unsigned int offset,
                   unsigned short type, struct idt_descriptor *idt_desc) {
  idt_desc->offset_low = (offset & 0xFFFF);
  idt_desc->segment_selector = select;
  idt_desc->type_attr = type; // bit 8 a 15 da parte mais alta
  idt_desc->reserved = 0; // bit 0 a 7 da parte mais alta
  idt_desc->offset_high = (offset & 0xFFFF0000) >> 16;
}

void init_idt(struct idt *idt_global, struct idt_descriptor *idt) {
  idt_global->adress = idt;
  idt_global->size = (IDT_NUM_ENTRIES * sizeof(struct idt_descriptor)) - 1;

  init_idt_desc(
    KERNEL_CODE_SEGMENT_OFFSET,
    (unsigned int)interrupt_handler_32,
    IDT_TYPE_KERNEL_PRIVILEGED,
    &idt[32]
  );
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou handler do timer");

  init_idt_desc(
    KERNEL_CODE_SEGMENT_OFFSET,
    (unsigned int)interrupt_handler_33,
    IDT_TYPE_KERNEL_PRIVILEGED,
    &idt[33]
  );
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou handler de teclado");

  init_idt_desc(
    KERNEL_CODE_SEGMENT_OFFSET,
    (unsigned int)interrupt_handler_39,
    IDT_TYPE_KERNEL_PRIVILEGED,
    &idt[39]
  );

  init_idt_desc(
    KERNEL_CODE_SEGMENT_OFFSET,
    (unsigned int)interrupt_handler_47,
    IDT_TYPE_KERNEL_PRIVILEGED,
    &idt[47]
  );

  /* Syscall: INT 0x80 (128) com DPL=3 para permitir chamada de ring 3 */
  init_idt_desc(
    KERNEL_CODE_SEGMENT_OFFSET,
    (unsigned int)syscall_handler_128,
    IDT_TYPE_USER_PRIVILEGED,
    &idt[128]
  );
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Registrou handler syscall (0x80)");

  load_lidt(idt_global);
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou IDT");

  pic_init();
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Iniciou PIC");

  /* Habilita IRQ0 (timer) e IRQ1 (teclado) */
  outb(PIC1_PORT_B, 0xFC);
}
