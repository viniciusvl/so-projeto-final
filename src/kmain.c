/* Funções de C */

#include "interrupts/idt.h"
#include "io/io.h"
#include "segment/gdt.h"

void kmain() {
  // GDT
  unsigned short size = 3;
  struct gdt_seg_descriptor descriptors[3];
  struct gdt gdt_global;
  struct gdt_seg_descriptor *adress_descriptor = descriptors;

  gdt_global.adress = adress_descriptor;
  gdt_global.size = (size * 8);

  init_gdt(adress_descriptor, &gdt_global);

  struct idt_descriptor idt[IDT_NUM_ENTRIES];
  struct idt idt_global;

  idt_global.adress = idt;
  idt_global.size = (IDT_NUM_ENTRIES * sizeof(struct idt_descriptor)) - 1;

  /* Configurar handler do teclado na posição 33 */
  init_idt_desc(0x08, (unsigned int)interrupt_handler_33, 0x8E, &idt[33]);

  load_lidt(&idt_global);

  pic_init();

  outb(PIC1_PORT_B, 0xFD);

  /*
  asm volatile é específico para o GNU, que envia comandos
  específicos
  */
  asm volatile("sti");

  while(1) {
    asm volatile("hlt");
  }
}