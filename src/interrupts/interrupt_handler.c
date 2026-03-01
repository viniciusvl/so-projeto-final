#include "idt.h"

#define KBD_INTERRUPT 0x21

void interrupt_handler(unsigned int interrupt) {
  switch (interrupt) {
  case KBD_INTERRUPT:
    keyboard_handler();
    break;
  default:
    break;
  }

  pic_acknowledge(interrupt);
}
