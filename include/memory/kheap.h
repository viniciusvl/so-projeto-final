#pragma once
#include <stdint.h>

void  kheap_init(void);
void *kmalloc(uint32_t nbytes);
void  kfree(void *ptr);
