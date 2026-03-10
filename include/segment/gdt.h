#ifndef INCLUDE_GDT
#define INCLUDE_GDT


/*
 ==== FORMATO DE UM SEGMENT DESCRIPTION ========= 
Fonte: https://wiki.osdev.org/Global_Descriptor_Table#Segment_Descriptor

O binário da descrição de segmento é formado por
    32 bits: definem o endereço onde segmento começa
    20 bits: definem a maior unidade de endereço endereçável
    8  bits: definem restrições de acesso
    4  bits: definem as flags 

Esse binário é organizado da seguinte forma.
Eles devem seguir essa ordem, definida como padrão

0 - 15: os primeiros 16 bits baixos de limit
16 - 31: os primeiros 16 bits baixos de base
32 - 39: os oito bits do meio de base
40 - 47: Access Byte
48 - 51: os 4 bits mais altos de Limite
52 - 55: Flags
56 - 63: os 8 bits mais altos de Base
*/
struct gdt_seg_descriptor{
    unsigned short int limit_0_15;
    unsigned short int base_0_15;
    unsigned char base_16_23;
    unsigned char access_byte;
    unsigned char limit_16_19:4;
    unsigned char flags:4;
    unsigned char base_24_31; 
} __attribute__((packed));

/*
    Define o endereço inicial e o 
tamanho da tabela GDT
*/
struct gdt {
    unsigned short size; // armazena tamanho da tabela
    struct gdt_seg_descriptor *adress; // aponta para primeiro elemento da tabela GDT
} __attribute__((packed));

/*
    Função do Assembly que carrega o endereço
onde o GDT está
    @param gdt_adress: endereço que aponta para o descritor
geral (struct) da tabela GDT
*/
void lgdt_f(struct gdt *gdt_adress);

/*
    Altera os segments selectors para apontar 
para o segmento adequado
    @param segment: número de 16 bits que aponta para segmento
*/
void config_segment_selector(unsigned short int segment);

/*
    Função que realiza far_jump do cs
*/
void far_jump();

void init_gdt_descriptor(unsigned int base, unsigned int limite, unsigned char acces, unsigned char flags, struct gdt_seg_descriptor *s);

void init_gdt(struct gdt_seg_descriptor *gdt, struct gdt *gdt_global);

#endif