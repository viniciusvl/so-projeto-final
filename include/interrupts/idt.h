#ifndef INCLUDE_IDT_H
#define INCLUDE_IDT_H

#define IDT_NUM_ENTRIES 256

/*
    Formato de uma entrada da IDT (8 bytes):
    Bytes 0-1: offset_low (bits 0-15 do endereço do handler)
    Bytes 2-3: segment_selector (segmento de código, normalmente 0x08)
    Byte  4:   reserved (sempre zero)
    Byte  5:   type_attr (P, DPL, tipo do gate)
    Bytes 6-7: offset_high (bits 16-31 do endereço do handler)
*/
struct idt_descriptor {
  unsigned short offset_low;
  unsigned short segment_selector;
  unsigned char reserved;
  unsigned char type_attr;
  unsigned short offset_high;
} __attribute__((packed));

struct idt {
  unsigned short size;
  struct idt_descriptor *adress;
} __attribute__((packed));

void load_lidt(struct idt *ptr);
void init_idt_desc(unsigned short select, unsigned int offset,
                   unsigned short type, struct idt_descriptor *idt_desc);

/* Handler do teclado (assembly) */
void interrupt_handler_33(void);

void interrupt_handler(unsigned int interrupt);

#define PIC1_PORT_A 0x20
#define PIC1_PORT_B 0x21
#define PIC2_PORT_A 0xA0
#define PIC2_PORT_B 0xA1

#define PIC1_START_INTERRUPT 0x20
#define PIC2_START_INTERRUPT 0x28
#define PIC2_END_INTERRUPT PIC2_START_INTERRUPT + 7
#define PIC_ACK 0x20

void pic_init(void);
void pic_acknowledge(unsigned int interrupt);

unsigned char read_scan_code(void);
void keyboard_handler(void);

#endif