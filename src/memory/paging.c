#include "memory/paging.h"
#include "memory/pfa.h"

/* Page Directory definido em loader.s (1024 entradas de 4 bytes) */
extern uint32_t page_directory[];

/* Page Table do kernel: substitui a entrada PSE de 4MB do índice 768
 * Cada entrada mapeia uma página de 4KB.
 * Entrada 1023 é reservada para mapeamento temporário (0xC03FF000). */
static uint32_t kernel_page_table[1024] __attribute__((aligned(4096)));

/* ---- helpers inline ---- */

static inline void invlpg(uint32_t addr)
{
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void reload_cr3(void)
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

void paging_init(void)
{
    extern uint32_t kernel_physical_end;
    uint32_t kp_end = (uint32_t)&kernel_physical_end;

    /* Quantas page frames existem no kernel */
    uint32_t mapped_frames = (kp_end + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Pega o lugar em que a page table do kernel irá COMEÇAR 
    e faz cada endereço apontar para o frame físico correspondente 
    entrada 0000 -> endereço 000 */
    /* Preenche as entradas mapeadas: virtual 0xC0000000+i*4K -> físico i*4K */
    for (uint32_t i = 0; i < mapped_frames && i < TEMP_PT_INDEX; i++)
        kernel_page_table[i] = (i * PAGE_SIZE) | PTE_PRESENT | PTE_WRITABLE;

    /* Entradas restantes ficam não-presentes (heap vai mapeá-las sob demanda) */
    for (uint32_t i = mapped_frames; i < TEMP_PT_INDEX; i++)
        kernel_page_table[i] = 0;

    /* Entrada 1023: reservada para mapeamento temporário */
    kernel_page_table[TEMP_PT_INDEX] = 0;

    /* Endereço físico da page table (virtual - 0xC0000000) */
    uint32_t pt_phys = (uint32_t)kernel_page_table - 0xC0000000;

    /* Atualiza entrada 768 do Page Directory: aponta para a page table
     * (substitui a entrada PSE de 4MB por page table de 4KB) */
    page_directory[KERNEL_PD_INDEX] = pt_phys | PDE_PRESENT | PDE_WRITABLE;

    /* Flush TLB inteiro */
    reload_cr3();
}

void paging_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags)
{
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;

    if (pd_index == KERNEL_PD_INDEX) {
        /* Acesso direto à page table do kernel */
        kernel_page_table[pt_index] = (paddr & 0xFFFFF000) | flags;
        invlpg(vaddr);
        return;
    }

    /* Para outros índices do PD: precisa alocar nova page table */
    if (!(page_directory[pd_index] & PDE_PRESENT)) {
        uint32_t pt_frame = pfa_alloc();
        if (pt_frame == 0)
            return;

        /* Usa mapeamento temporário para inicializar a nova page table */
        uint32_t *new_pt = (uint32_t *)paging_temp_map(pt_frame);
        for (uint32_t i = 0; i < 1024; i++)
            new_pt[i] = 0;

        new_pt[pt_index] = (paddr & 0xFFFFF000) | flags;
        paging_temp_unmap();

        page_directory[pd_index] = pt_frame | PDE_PRESENT | PDE_WRITABLE;
    } else {
        /* Page table já existe: modifica via mapeamento temporário */
        uint32_t pt_phys = page_directory[pd_index] & 0xFFFFF000;
        uint32_t *pt = (uint32_t *)paging_temp_map(pt_phys);
        pt[pt_index] = (paddr & 0xFFFFF000) | flags;
        paging_temp_unmap();
    }

    invlpg(vaddr);
}

void paging_unmap_page(uint32_t vaddr)
{
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;

    if (pd_index == KERNEL_PD_INDEX) {
        kernel_page_table[pt_index] = 0;
        invlpg(vaddr);
    }
}

void *paging_temp_map(uint32_t paddr)
{
    kernel_page_table[TEMP_PT_INDEX] = (paddr & 0xFFFFF000) | PTE_PRESENT | PTE_WRITABLE;
    invlpg(TEMP_MAP_VADDR);
    return (void *)TEMP_MAP_VADDR;
}

void paging_temp_unmap(void)
{
    kernel_page_table[TEMP_PT_INDEX] = 0;
    invlpg(TEMP_MAP_VADDR);
}
