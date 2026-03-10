/* Funções de C */

#include "interrupts/idt.h"
#include "io/io.h"
#include "segment/gdt.h"
#include "multiboot.h"
#include "kernel/module_loader.h"

void kmain(unsigned int ebx)
{
  // Coletando o endereço do programa a partir do módulo do GRUB
  multiboot_info_t *mbinfo = (multiboot_info_t *) ebx;

  multiboot_module_t *mod; // Armazenará o início da lista de módulos

  mod = get_module_list(mbinfo); // Coletando o início da lista de módulos

  // GDT
  unsigned short size = 3;
  struct gdt_seg_descriptor descriptors[3];
  struct gdt gdt_global;
  struct gdt_seg_descriptor *adress_descriptor = descriptors;

  gdt_global.adress = adress_descriptor;
  gdt_global.size = (size * 8);

  init_gdt(adress_descriptor, &gdt_global);
  serial_write(SERIAL_COM1_BASE, "Iniciou GDT");

  struct idt_descriptor idt[IDT_NUM_ENTRIES];
  struct idt idt_global;

  idt_global.adress = idt;
  idt_global.size = (IDT_NUM_ENTRIES * sizeof(struct idt_descriptor)) - 1;

  /* Configurar handler do teclado na posição 33 */
  /* como é 32 bits, não dará problema ao rodar*/
  init_idt_desc(0x08, (unsigned int)interrupt_handler_33, 0x8E, &idt[33]);
  serial_write(SERIAL_COM1_BASE, "Iniciou handler de teclado");

  load_lidt(&idt_global);
  serial_write(SERIAL_COM1_BASE, "Iniciou IDT");

  pic_init();
  serial_write(SERIAL_COM1_BASE, "Iniciou PIC");

  outb(PIC1_PORT_B, 0xFD);
  // mostra hello world na tela
  fb_write("Hello, world", 12);
  // gera arquivo com1.out
  serial_write(SERIAL_COM1_BASE, "Finalizou progama");

  /*
  asm volatile é específico para o GNU, que envia comandos
  específicos
  */
  asm volatile("sti");

  if(mbinfo->mods_count)
  {
    fb_clear();
    char debbug[] = "Existe módulos";
    fb_write(debbug, sizeof(debbug) - 1);
  }

  run_module(mod); // Executando o módulo

  while(1) {
    asm volatile("hlt");
  }
}