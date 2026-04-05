/* Funções de C */
#include "io/serial_ports.h"
#include <stdint.h>       
#include "types.h"   
#include "memory/pfa.h"
#include "memory/paging.h"
#include "memory/kheap.h"
#include "interrupts/idt.h"
#include "interrupts/pit.h"
#include "io/io.h"
#include "segment/gdt.h"
#include "kernel/module_loader.h"
#include "multiboot.h"
#include "process/process.h"
#include "scheduler/scheduler.h"
#include "segment/tss.h"


/* GDT e IDT em storage estatico para nao serem corrompidos
   quando o TSS troca a stack para kernel_stack_top em interrupts de ring 3 */
static struct gdt_seg_descriptor gdt_descriptors[6];
static struct gdt gdt_global;
static struct idt idt_global;
static struct idt_descriptor idt_entries[IDT_NUM_ENTRIES];

void kmain(unsigned int ebx)
{

  extern uint32_t kernel_physical_end;

  multiboot_info_t *mbinfo = (multiboot_info_t *)ebx;

  // Define memória livre
  uint32_t mem_start = ((uint32_t)&kernel_physical_end + 0xFFF) & ~0xFFF;
  uint32_t mem_end   = 64 * 1024 * 1024;

  // Módulos GRUB (antes de pfa_init para passar o range)
  multiboot_module_t *mod = get_module_list(mbinfo);
  uint32_t mod_start = mod ? mod->mod_start : 0;
  uint32_t mod_end   = mod ? mod->mod_end   : 0;

  module_loader_set_modules(mod, mbinfo->mods_count);

  // Inicializa PFA (reserva frames do módulo internamente)
  pfa_init(mem_start, mem_end, mod_start, mod_end);

  serial_write(SERIAL_COM1_BASE, "[SYS - PAGING] PFA inicializado com sucesso");

  /* Modifica para que a PDT alocada em section data aponte para PT com frames de 4KB */
  paging_init();
  serial_write(SERIAL_COM1_BASE, "[SYS - PAGING] Kernel modificada para páginas com 4KB");

  // Inicializa Kernel Heap (malloc/free)
  kheap_init();
  serial_write(SERIAL_COM1_BASE, "[SYS - HEAP] Kernel Heap inicializado");

  
  // GDT (6 entradas: null + kernel code/data + user code/data + TSS)
  extern uint32_t kernel_stack_top;

  init_gdt(gdt_descriptors, &gdt_global, 6);
  serial_write(SERIAL_COM1_BASE, "[SYS] Iniciou GDT");

  // TSS (deve ser inicializado apos a GDT)
  tss_init(0x10, (uint32_t)&kernel_stack_top, gdt_descriptors);

  // IDT
  init_idt(&idt_global, idt_entries);

  /* Timer preemptivo a cada 10ms */
  pit_init(10);

  scheduler_init();

  /* Define o quantum (time slice) para Round Robin: 3 ticks = 30ms (PIT a 10ms) */
  scheduler_set_quantum(3);

  /* Seleciona a politica de escalonamento:
   *   SCHEDULER_POLICY_FCFS - First Come First Served (padrao)
   *   SCHEDULER_POLICY_SJF  - Shortest Job First
   *   SCHEDULER_POLICY_RR   - Round Robin
   */
  scheduler_set_policy(SCHEDULER_POLICY_RR);
  
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