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
#include "process/process.h"


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

  serial_write(SERIAL_COM1_BASE, "[SYS - PAGING] PFA inicializado com sucesso");

  /* Modifica para que a PDT alocada em section data aponte para PT com frames de 4KB */
  paging_init();
  serial_write(SERIAL_COM1_BASE, "[SYS - PAGING] Kernel modificada para páginas com 4KB");

  // Inicializa Kernel Heap (malloc/free)
  kheap_init();
  serial_write(SERIAL_COM1_BASE, "[SYS - HEAP] Kernel Heap inicializado");

  
  // GDT
  unsigned short size = 5;
  struct gdt_seg_descriptor descriptors[5];
  struct gdt gdt_global;
  struct gdt_seg_descriptor *adress_descriptor = descriptors;

  init_gdt(adress_descriptor, &gdt_global, size);
  serial_write(SERIAL_COM1_BASE, "[SYS] Iniciou GDT");

  // IDT
  struct idt_descriptor idt[IDT_NUM_ENTRIES];
  struct idt idt_global;

  init_idt(&idt_global, idt);

  /* Cria PCB e entra em user mode */
  if (mod_start != 0 && mod_end != 0) {
    serial_write(SERIAL_COM1_BASE, "Criando PCB para modulo GRUB");
    struct PCB *pcb = create_pcb_grub_modules(mod_start, mod_end);

    serial_write(SERIAL_COM1_BASE, "Executando modulo em user mode");
    run_user_mode_module(pcb);
  } else {
    serial_write(SERIAL_COM1_BASE, "Nenhum modulo encontrado");
  }

  while(1) {
    asm volatile("hlt");
  }
}