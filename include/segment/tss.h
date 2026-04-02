#ifndef INCLUDE_TSS_H
#define INCLUDE_TSS_H

#include <stdint.h>
#include "segment/gdt.h"

/*
    Task State Segment (TSS)
    Estrutura definida pela Intel (figura 7-2, secao 7.2.1 do manual).
    O processador usa o TSS para encontrar a stack do kernel (ss0:esp0)
    quando ocorre uma interrupcao em ring 3.
*/
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/*
    Inicializa o TSS, adiciona o descritor na GDT[5] e carrega no TR.
    @param ss0: segment selector da stack do kernel (0x10)
    @param esp0: topo da stack do kernel
    @param gdt: ponteiro para o array de descritores da GDT
*/
void tss_init(uint32_t ss0, uint32_t esp0, struct gdt_seg_descriptor *gdt);

/*
    Atualiza o esp0 do TSS (usado ao trocar de processo).
    @param esp0: novo topo da stack do kernel
*/
void tss_set_kernel_stack(uint32_t esp0);

/* Assembly: carrega o segment selector do TSS no registrador TR */
void load_tss(uint16_t tss_segment_selector);

#endif
