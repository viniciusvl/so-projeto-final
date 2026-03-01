/* Funções de C */

#include "io/io.h"
#include "segment/gdt.h"

void kmain() {
    unsigned short size = 3;
    struct gdt_seg_descriptor descriptors[3];
    struct gdt gdt_global;
    struct gdt_seg_descriptor *adress_descriptor = descriptors; 

    gdt_global.adress = adress_descriptor;
    gdt_global.size = (size * 8);

    init_gdt(adress_descriptor, &gdt_global);

    fb_write("hello, world", 12);
    serial_write(SERIAL_COM1_BASE, 'B');
}