#pragma once
#include <stdint.h>

#define PAGE_SIZE 4096

void pfa_init(uint32_t mem_start, uint32_t mem_end, uint32_t mod_start, uint32_t mod_end);
uint32_t pfa_alloc(void);
void pfa_reserve(uint32_t addr);
void pfa_free(uint32_t addr);