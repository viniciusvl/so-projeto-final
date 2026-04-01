#include "process/process.h"
#include "memory/pfa.h"
#include "memory/paging.h"
#include "memory/kheap.h"
#include "io/serial_ports.h"
#include "io/io.h"

/* Segmentos de user mode com RPL=3 (bits 0-1 do selector) */
#define USER_CODE_SEGMENT_SELECTOR  (0x18 | 0x3)   /* index 3, RPL=3 = 0x1B */
#define USER_DATA_SEGMENT_SELECTOR  (0x20 | 0x3)   /* index 4, RPL=3 = 0x23 */

/* PDT do kernel (definida em loader.s) */
extern uint32_t page_directory[];

/* Indice da PDE do kernel em 3GB */
#define KERNEL_PD_INDEX 768

/* ---- Funções auxiliares ---- */

/*
 * alloc_page_frames: aloca dois page frames consecutivos na Page Table.
 * PT[0] e PT[1] recebem frames fisicos com flags de user mode.
 */
static void alloc_page_frames(uint32_t pt_phys)
{
    uint32_t frame_0 = pfa_alloc();
    uint32_t frame_1 = pfa_alloc();

    serial_write(SERIAL_COM1_BASE, "[process] alloc_page_frames: frame_0=");
    serial_write_hex(frame_0);
    serial_write(SERIAL_COM1_BASE, "[process] alloc_page_frames: frame_1=");
    serial_write_hex(frame_1);

    uint32_t *pt = (uint32_t *)paging_temp_map(pt_phys);
    pt[0] = frame_0 | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    pt[1] = frame_1 | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    paging_temp_unmap();
}

/*
 * copy_module: copia byte a byte o binario do modulo GRUB para o page frame.
 * O modulo esta acessivel via identity mapping (PDT[0] mapeia 0-4MB).
 */
static void copy_module(uint32_t page_frame_1, uint32_t mod_start, uint32_t mod_end)
{
    char *src_start = (char *)mod_start;
    char *src_end   = (char *)mod_end;
    uint32_t size   = (uint32_t)(src_end - src_start);

    serial_write(SERIAL_COM1_BASE, "[process] copy_module: tamanho=");
    serial_write_hex(size);

    /* Limita a copia ao tamanho de um page frame (4KB) */
    if (size > PAGE_SIZE)
        size = PAGE_SIZE;

    /* Mapeia o page frame destino via temp map e copia byte a byte */
    char *dest = (char *)paging_temp_map(page_frame_1);

    for (uint32_t i = 0; i < size; i++) {
        dest[i] = src_start[i];
    }

    /* Zera o restante do page frame */
    for (uint32_t i = size; i < PAGE_SIZE; i++) {
        dest[i] = 0;
    }

    paging_temp_unmap();

    serial_write(SERIAL_COM1_BASE, "[process] copy_module: copia concluida");
}

/*
 * alloc_stack: aloca page table e page frame para a stack do processo.
 * A stack comeca em 0xBFFFFFFB (logo abaixo de 3GB), crescendo para baixo.
 *
 * 0xBFFFFFFB:
 *   PDT index = 0xBFFFFFFB >> 22          = 767
 *   PT  index = (0xBFFFFFFB >> 12) & 0x3FF = 1023
 */
static void alloc_stack(uint32_t pdt_phys)
{
    uint32_t stack_vaddr = 0xBFFFF000;  /* pagina que contem 0xBFFFFFFB */
    uint32_t pd_index = stack_vaddr >> 22;              /* 767 */
    uint32_t pt_index = (stack_vaddr >> 12) & 0x3FF;    /* 1023 */

    serial_write(SERIAL_COM1_BASE, "[process] alloc_stack: pd_index=");
    serial_write_hex(pd_index);
    serial_write(SERIAL_COM1_BASE, "[process] alloc_stack: pt_index=");
    serial_write_hex(pt_index);

    /* Aloca page table para PDT[767] */
    uint32_t pt_phys = pfa_alloc();

    /* Zera a page table e insere o frame da stack */
    uint32_t *pt = (uint32_t *)paging_temp_map(pt_phys);
    for (uint32_t i = 0; i < 1024; i++)
        pt[i] = 0;

    uint32_t stack_frame = pfa_alloc();
    pt[pt_index] = stack_frame | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    paging_temp_unmap();

    serial_write(SERIAL_COM1_BASE, "[process] alloc_stack: stack_frame=");
    serial_write_hex(stack_frame);

    /* Registra a page table no PDT */
    uint32_t *pdt = (uint32_t *)paging_temp_map(pdt_phys);
    pdt[pd_index] = pt_phys | PDE_PRESENT | PDE_WRITABLE | PDE_USER;
    paging_temp_unmap();

    serial_write(SERIAL_COM1_BASE, "[process] alloc_stack: concluido");
}

/* ---- Funcao principal ---- */

struct PCB *create_pcb_grub_modules(uint32_t mod_start, uint32_t mod_end)
{
    serial_write(SERIAL_COM1_BASE, "[process] create_pcb_grub_modules: inicio");

    /* 3.1 - Aloca o PCB */
    struct PCB *pcb = (struct PCB *)kmalloc(sizeof(struct PCB));
    serial_write(SERIAL_COM1_BASE, "[process] PCB alocado em:");
    serial_write_hex((uint32_t)pcb);

    /* 3.2 - Aloca a PDT (Page Directory Table) */
    uint32_t pdt_phys = pfa_alloc();
    pcb->pdt = pdt_phys;

    serial_write(SERIAL_COM1_BASE, "[process] PDT alocada em:");
    serial_write_hex(pdt_phys);

    /* Zera toda a PDT */
    uint32_t *pdt = (uint32_t *)paging_temp_map(pdt_phys);
    for (uint32_t i = 0; i < 1024; i++)
        pdt[i] = 0;
    paging_temp_unmap();

    /* 3.3 - Aloca uma Page Table para PDT[0] (mapeia 0x00000000 - 0x003FFFFF) */
    uint32_t pt_phys = pfa_alloc();

    /* Zera a page table */
    uint32_t *pt = (uint32_t *)paging_temp_map(pt_phys);
    for (uint32_t i = 0; i < 1024; i++)
        pt[i] = 0;
    paging_temp_unmap();

    /* Insere a PT na PDT[0] com flags de user mode */
    pdt = (uint32_t *)paging_temp_map(pdt_phys);
    pdt[0] = pt_phys | PDE_PRESENT | PDE_WRITABLE | PDE_USER;
    paging_temp_unmap();

    serial_write(SERIAL_COM1_BASE, "[process] PT alocada para PDT[0]:");
    serial_write_hex(pt_phys);

    /* 3.5 - Aloca dois page frames na PT[0] e PT[1] */
    alloc_page_frames(pt_phys);

    /* 3.6 - Copia o binario do modulo GRUB para o primeiro page frame */
    /* Recupera o endereco fisico do frame em PT[0] */
    uint32_t *pt_mapped = (uint32_t *)paging_temp_map(pt_phys);
    uint32_t page_frame_1 = pt_mapped[0] & 0xFFFFF000;
    paging_temp_unmap();

    copy_module(page_frame_1, mod_start, mod_end);

    /* 3.7 - Aloca a stack do processo (mapeada em 0xBFFFFFFB) */
    alloc_stack(pdt_phys);
    pcb->esp = 0xBFFFFFFB;

    /* 3.8 - Copia a PDE do kernel (3GB) para a nova PDT */
    pdt = (uint32_t *)paging_temp_map(pdt_phys);
    pdt[KERNEL_PD_INDEX] = page_directory[KERNEL_PD_INDEX];
    paging_temp_unmap();

    serial_write(SERIAL_COM1_BASE, "[process] PDE do kernel copiada (index 768)");

    /* Configura registradores do PCB */
    pcb->eip    = 0x00000000;   /* entry point do codigo user mode */
    pcb->eflags = 0x00000000;   /* interrupts desabilitadas por enquanto */
    pcb->cs     = USER_CODE_SEGMENT_SELECTOR;   /* 0x1B */
    pcb->ss     = USER_DATA_SEGMENT_SELECTOR;   /* 0x23 */

    serial_write(SERIAL_COM1_BASE, "[process] PCB configurado:");
    serial_write(SERIAL_COM1_BASE, "  eip=");
    serial_write_hex(pcb->eip);
    serial_write(SERIAL_COM1_BASE, "  esp=");
    serial_write_hex(pcb->esp);
    serial_write(SERIAL_COM1_BASE, "  cs=");
    serial_write_hex(pcb->cs);
    serial_write(SERIAL_COM1_BASE, "  ss=");
    serial_write_hex(pcb->ss);

    serial_write(SERIAL_COM1_BASE, "[process] create_pcb_grub_modules: concluido");
    return pcb;
}

/* ---- Executa processo em user mode ---- */

void run_user_mode_module(struct PCB *pcb)
{
    serial_write(SERIAL_COM1_BASE, "[process] run_user_mode_module: trocando cr3");
    serial_write_hex(pcb->pdt);

    /* Atualiza cr3 para a PDT do processo */
    update_cr3(pcb->pdt);

    serial_write(SERIAL_COM1_BASE, "[process] run_user_mode_module: entrando em user mode");

    /* Entra em user mode via iret */
    enter_user_mode(pcb->ss, pcb->esp, pcb->eflags, pcb->cs, pcb->eip);
}
