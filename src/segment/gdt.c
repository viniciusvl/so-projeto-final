#include "segment/gdt.h"

/*Configura GDT Descriptor que descreve cada segmento na memória*/
void init_gdt_descriptor(unsigned int base, unsigned int limite, unsigned char acces, unsigned char flags, struct gdt_seg_descriptor *s){
	s->limit_0_15 = (limite & 0xffff);
	s->base_0_15 = (base & 0xffff);
	s->base_16_23 = (base & 0xff0000) >> 16;
	s->access_byte = acces;
	s->limit_16_19 = (limite & 0xf0000) >> 16;
	s->flags = (flags & 0xf);
	s->base_24_31 = (base & 0xff000000) >> 24;
}

void init_gdt(struct gdt_seg_descriptor *gdt, struct gdt *gdt_global, unsigned short size){
	gdt_global->adress = gdt;
	gdt_global->size = (size * 8) - 1;

	/*Iniciando cada segmento*/
	init_gdt_descriptor(0x0, 0x0, 0x0, 0x0, &gdt[0]);
    init_gdt_descriptor(0x0, 0xFFFFFFFF, 0x9B, 0xC, &gdt[1]);
    init_gdt_descriptor(0x0, 0xFFFFFFFF, 0x92, 0xC, &gdt[2]);
    /* User mode: code segment (DPL=3, offset 0x18) */
    init_gdt_descriptor(0x0, 0xFFFFFFFF, 0xFB, 0xC, &gdt[3]);
    /* User mode: data segment (DPL=3, offset 0x20) */
    init_gdt_descriptor(0x0, 0xFFFFFFFF, 0xF2, 0xC, &gdt[4]);
	/*Usa o código Assembly que configura o GDT de forma global*/
    lgdt_f(gdt_global);

    far_jump();
	/*Configura como os registradores do segmento devem iniciar*/
    config_segment_selector(0x10);
}