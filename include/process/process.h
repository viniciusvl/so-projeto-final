#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROCESS_STATE_READY   0
#define PROCESS_STATE_RUNNING 1
#define PROCESS_STATE_BLOCKED 2

#define PROCESS_KERNEL_STACK_SIZE 4096

struct process_context {
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t user_ss;
};

struct PCB {
    uint32_t pid;       /* Process ID */
    uint32_t ppid;      /* Parent Process ID */

    uint32_t pdt;       /* Page Directory Table (endereço fisico) */

    uint32_t eip;       /* Instruction Pointer */
    uint32_t esp;       /* Stack Pointer */
    uint32_t eflags;    /* Flags da CPU */

    uint32_t cs;        /* Code Segment */
    uint32_t ss;        /* Stack Segment */

    uint32_t kernel_stack_base;  /* Base virtual da kernel stack do processo */
    uint32_t kernel_esp0;        /* Topo (esp0) usado pelo TSS em ring3 -> ring0 */

    uint32_t state;     /* Estado para escalonamento cooperativo */

    uint32_t burst_time; /* Tempo de execucao estimado (usado pelo SJF) */

    struct process_context context; /* Contexto salvo para scheduler cooperativo */
};

/* Cria um PCB alocando apenas a PDT (sem page frames, stack ou codigo) */
struct PCB *create_pcb(void);

/* Cria um PCB para um modulo GRUB, configurando PDT, page tables, stack e codigo */
struct PCB *create_pcb_grub_modules(uint32_t mod_start, uint32_t mod_end);

/* Executa o processo em user mode (ring 3) */
void run_user_mode_module(struct PCB *pcb);

/* Assembly: atualiza cr3 com o endereco fisico da nova PDT */
void update_cr3(uint32_t pdt_physical);

/* Assembly: entra em user mode via iret */
void enter_user_mode(uint32_t ss, uint32_t esp, uint32_t eflags, uint32_t cs, uint32_t eip);

#endif
