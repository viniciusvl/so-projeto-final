#include "interrupts/idt.h"
#include "io/serial_ports.h"

#define KBD_INTERRUPT 0x21 // 33 em decimal

void interrupt_handler(unsigned int interrupt) {
  serial_write(SERIAL_COM1_BASE, "[SYS - INTERRUPTS] Interrupção recebida: ");
  serial_write_hex(interrupt);


  switch (interrupt) {
    case KBD_INTERRUPT:
      keyboard_handler();
      break;
    default:
      break;
  }

  pic_acknowledge(interrupt);
}
