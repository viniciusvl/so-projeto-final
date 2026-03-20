/* Funções de C */

#include "interrupts/idt.h"
#include "io/io.h"
#include "segment/gdt.h"
#include "multiboot.h"
#include "kernel/module_loader.h"

void kmain(unsigned int ebx)
{
  // Módulos GRUB
  multiboot_module_t *mod = get_module_list((multiboot_info_t *) ebx); 
  
  // GDT
  unsigned short size = 3;
  struct gdt_seg_descriptor descriptors[3];
  struct gdt gdt_global;
  struct gdt_seg_descriptor *adress_descriptor = descriptors;

  gdt_global.adress = adress_descriptor;
  gdt_global.size = (size * 8);

  init_gdt(adress_descriptor, &gdt_global);
  serial_write(SERIAL_COM1_BASE, "Iniciou GDT");

  // IDT
  struct idt_descriptor idt[IDT_NUM_ENTRIES];
  struct idt idt_global;

  idt_global.adress = idt;
  idt_global.size = (IDT_NUM_ENTRIES * sizeof(struct idt_descriptor)) - 1;

  init_idt_desc(0x08, (unsigned int)interrupt_handler_33, 0x8E, &idt[33]);
  serial_write(SERIAL_COM1_BASE, "Iniciou handler de teclado");

  load_lidt(&idt_global);
  serial_write(SERIAL_COM1_BASE, "Iniciou IDT");

  pic_init();
  serial_write(SERIAL_COM1_BASE, "Iniciou PIC");

  outb(PIC1_PORT_B, 0xFD);

  serial_write(SERIAL_COM1_BASE, "Finalizou progama");

  serial_write(SERIAL_COM1_BASE, "Rodou módulo"); 
  run_module(mod); // Executando o módulo

  asm volatile("sti");


  while(1) {
    asm volatile("hlt");
  }
}