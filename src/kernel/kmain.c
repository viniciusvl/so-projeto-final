/* Funções de C */
#include <stdint.h>       
#include "types.h"   
#include "memory/pfa.h"
#include "interrupts/idt.h"
#include "io/io.h"
#include "segment/gdt.h"
#include "kernel/module_loader.h"
#include "multiboot.h"


void kmain(unsigned int ebx)
{

  extern uint32_t kernel_physical_end;

  // Define memória livre
  uint32_t mem_start = ((uint32_t)&kernel_physical_end + 0xFFF) & ~0xFFF;
  uint32_t mem_end   = 64 * 1024 * 1024;

  // Inicializa PFA
  pfa_init(mem_start, mem_end);

  serial_write(SERIAL_COM1_BASE, "PFA OK\n");

  // Teste

  pfa_alloc();
  pfa_alloc();

  serial_write(SERIAL_COM1_BASE, "Frames alocados\n");


  // Módulos GRUB
  multiboot_module_t *mod = get_module_list((multiboot_info_t *) ebx); 
  //reserva a memória dos módulos GRUB 
if(mod != NULL) {
    for(uint32_t addr = mod->mod_start; addr < mod->mod_end; addr += PAGE_SIZE) {
        // Marca cada página do módulo como alocada
        pfa_alloc(); // Aqui você poderia implementar pfa_reserve(addr) se tiver
    }
}
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