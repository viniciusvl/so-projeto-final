#include "memory/pfa.h"

#define MAX_FRAMES 16384   // 64MB / 4KB

static uint8_t bitmap[MAX_FRAMES / 8];

static uint32_t start_frame;
static uint32_t total_frames;

void pfa_reserve(uint32_t addr)
{
    uint32_t frame = addr / PAGE_SIZE;
    uint32_t byte  = frame / 8;
    uint32_t bit   = frame % 8;
    bitmap[byte] |= (1 << bit);
}

void pfa_init(uint32_t mem_start, uint32_t mem_end, uint32_t mod_start, uint32_t mod_end)
{
    // tamanho do bitmap
    uint32_t bitmap_size = MAX_FRAMES / 8;

    // alinhar início
    mem_start = (mem_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    // reservar espaço pro bitmap
    uint32_t bitmap_start = mem_start;
    uint32_t bitmap_end   = bitmap_start + bitmap_size;

    // mover início da memória livre
    mem_start = (bitmap_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    start_frame = mem_start / PAGE_SIZE;
    total_frames = mem_end / PAGE_SIZE;

    // marca tudo como usado
    for (uint32_t i = 0; i < MAX_FRAMES / 8; i++)
        bitmap[i] = 0xFF;

    // libera só memória válida
    for (uint32_t i = start_frame; i < total_frames; i++) {
        uint32_t byte = i / 8;
        uint32_t bit  = i % 8;
        bitmap[byte] &= ~(1 << bit);
    }

    // reserva frames do módulo GRUB
    if (mod_start != 0 || mod_end != 0) {
        for (uint32_t addr = mod_start; addr < mod_end; addr += PAGE_SIZE)
            pfa_reserve(addr);
    }
}

uint32_t pfa_alloc()
{
    for (uint32_t i = start_frame; i < total_frames; i++) {
        uint32_t byte = i / 8;
        uint32_t bit  = i % 8;

        if (!(bitmap[byte] & (1 << bit))) {
            bitmap[byte] |= (1 << bit);
            return i * PAGE_SIZE;
        }
    }

    return 0; // sem memória
}