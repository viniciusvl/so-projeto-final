#pragma once
#include <stdint.h>
#include "memory/pfa.h"

/* Flags para entradas do Page Directory */
#define PDE_PRESENT   0x01
#define PDE_WRITABLE  0x02
#define PDE_USER      0x04
#define PDE_PAGE_SIZE 0x80   /* PSE: página de 4MB */

/* Flags para entradas do Page Table */
#define PTE_PRESENT   0x01
#define PTE_WRITABLE  0x02
#define PTE_USER      0x04

/* Índices e endereços */
#define KERNEL_PD_INDEX   768
#define TEMP_PT_INDEX     1023
#define TEMP_MAP_VADDR    0xC03FF000  /* (768 << 22) | (1023 << 12) */

void paging_init(void);
void paging_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags);
void paging_unmap_page(uint32_t vaddr);
void *paging_temp_map(uint32_t paddr);
void paging_temp_unmap(void);
