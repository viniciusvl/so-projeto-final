#ifndef INCLUDE_GDT
#define INCLUDE_GDT

/*
    Define o endereço inicial e o 
tamanho da tabela GDT
*/
struct gdt {
    unsigned int adress;
    unsigned short size;
} __attribute__((packed));

/*
 ==== FORMATO DE UM SEGMENT DESCRIPTION ========= 
Fonte: https://wiki.osdev.org/Global_Descriptor_Table#Segment_Descriptor

0 - 15: Limit
16 - 31: Base
32 - 39: Base
40 - 47: Access Byte
48 - 51: Limite
52 - 55: Flags
56 - 63: Base
*/
struct gdt_seg_descriptor{

} __attribute__((packed));

#endif