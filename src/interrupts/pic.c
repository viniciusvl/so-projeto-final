#include "../io/io.h"
#include "idt.h"

void pic_init(void) {
  outb(PIC1_PORT_A, 0x11);
  outb(PIC2_PORT_A, 0x11);

  outb(PIC1_PORT_B, PIC1_START_INTERRUPT);
  outb(PIC2_PORT_B, PIC2_START_INTERRUPT);

  outb(PIC1_PORT_B, 0x04);
  outb(PIC2_PORT_B, 0x02);

  outb(PIC1_PORT_B, 0x01);
  outb(PIC2_PORT_B, 0x01);

  outb(PIC1_PORT_B, 0xFF);
  outb(PIC2_PORT_B, 0xFF);
}
/*
    Quando o PIC recebe uma interrupção, ele precisa enviar
um ACK para o dispositivo que gerou a interrupção
*/
void pic_acknowledge(unsigned int interrupt) {
  if (interrupt < PIC1_START_INTERRUPT || interrupt > PIC2_END_INTERRUPT) {
    return;
  }

  if (interrupt < PIC2_START_INTERRUPT) {
    outb(PIC1_PORT_A, PIC_ACK);
  } else {
    outb(PIC2_PORT_A, PIC_ACK);
  }
}