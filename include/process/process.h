#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

struct PCB {
    uint32_t pdt;       /* Page Directory Table (endereço fisico) */

    uint32_t eip;       /* Instruction Pointer */
    uint32_t esp;       /* Stack Pointer */
    uint32_t eflags;    /* Flags da CPU */

    uint32_t cs;        /* Code Segment */
    uint32_t ss;        /* Stack Segment */
};

/* Cria um PCB para um modulo GRUB, configurando PDT, page tables, stack e codigo */
struct PCB *create_pcb_grub_modules(uint32_t mod_start, uint32_t mod_end);

/* Executa o processo em user mode (ring 3) */
void run_user_mode_module(struct PCB *pcb);

/* Assembly: atualiza cr3 com o endereco fisico da nova PDT */
void update_cr3(uint32_t pdt_physical);

/* Assembly: entra em user mode via iret */
void enter_user_mode(uint32_t ss, uint32_t esp, uint32_t eflags, uint32_t cs, uint32_t eip);

#endif
