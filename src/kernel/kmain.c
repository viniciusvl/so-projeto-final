/* Funções de C */
#include "io/serial_ports.h"
#include <stdint.h>       
#include "types.h"   
#include "memory/pfa.h"
#include "memory/paging.h"
#include "memory/kheap.h"
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

  // Módulos GRUB (antes de pfa_init para passar o range)
  multiboot_module_t *mod = get_module_list((multiboot_info_t *) ebx);
  uint32_t mod_start = mod ? mod->mod_start : 0;
  uint32_t mod_end   = mod ? mod->mod_end   : 0;

  // Inicializa PFA (reserva frames do módulo internamente)
  pfa_init(mem_start, mem_end, mod_start, mod_end);

  serial_write(SERIAL_COM1_BASE, "PFA OK");

  // Inicializa Page Table do kernel (troca PSE 4MB por page table 4KB)
  paging_init();
  serial_write(SERIAL_COM1_BASE, "Paging OK");

  // Inicializa Kernel Heap (malloc/free)
  kheap_init();
  serial_write(SERIAL_COM1_BASE, "Heap OK");

  // Teste: aloca memória com kmalloc
  uint32_t *ptr = (uint32_t *)kmalloc(sizeof(uint32_t) * 4);
  if (ptr) {
      ptr[0] = 0xCAFEBABE;
      serial_write(SERIAL_COM1_BASE, "kmalloc OK: ");
      serial_write_hex((uint32_t)ptr);
      serial_write(SERIAL_COM1_BASE, " val: ");
      serial_write_hex(ptr[0]);
      kfree(ptr);
      serial_write(SERIAL_COM1_BASE, "kfree OK");
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