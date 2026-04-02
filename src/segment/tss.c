#include "segment/tss.h"
#include "io/serial_ports.h"

/* TSS global */
static struct tss_entry tss;

void tss_init(uint32_t ss0, uint32_t esp0, struct gdt_seg_descriptor *gdt)
{
    /* Zera toda a estrutura */
    uint8_t *ptr = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss); i++)
        ptr[i] = 0;

    /* Configura a stack do kernel para transicoes ring3 -> ring0 */
    tss.ss0  = ss0;
    tss.esp0 = esp0;

    /* iomap_base aponta para alem do TSS (sem IO bitmap) */
    tss.iomap_base = sizeof(tss);

    /* Adiciona o TSS descriptor na GDT[5] (offset 0x28)
     *   Base   = endereco da struct tss
     *   Limite = sizeof(tss) - 1
     *   Access = 0x89 (Present=1, DPL=0, S=0, Type=0x9 = 32-bit TSS Available)
     *   Flags  = 0x0  (granularidade byte, sem D/B)
     */
    init_gdt_descriptor(
        (uint32_t)&tss,
        sizeof(tss) - 1,
        0x89,
        0x0,
        &gdt[5]
    );

    /* Carrega o TSS no registrador TR (offset 0x28 = index 5 * 8) */
    load_tss(0x28);

    serial_write(SERIAL_COM1_BASE, "[SYS - TSS] TSS inicializado");
}

void tss_set_kernel_stack(uint32_t esp0)
{
    tss.esp0 = esp0;
}
