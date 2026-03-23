OBJECTS = build/loader.o build/kmain.o build/outb.o build/inb.o build/io_c.o build/serial_ports.o build/lgdt_f.o build/config_segment_selector.o build/far_jump.o build/gdt.o build/keyboard.o build/interrupt_handler.o build/interrupt_handler_asm.o build/pic.o build/load_lidt.o build/idt.o build/module_loader.o build/pfa.o build/paging.o build/kheap.o

OBJECTC = kmain
OBJECTA = loader

CC = gcc

CFLAGS = -Iinclude -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c
ASFLAGS = -f elf32
LDFLAGS = -T linker/link.ld -melf_i386

AS = nasm

ASFLAGS = -f elf32

.PHONY: all compile_assembly compile_c_file link_kernel generate_iso_image run clean
all: clean compile_assembly compile_c_file link_kernel generate_iso_image

# Compilando o OS
compile_assembly:
	@mkdir -p build
	$(AS) $(ASFLAGS) src/boot/$(OBJECTA).s -o build/$(OBJECTA).o
	$(AS) $(ASFLAGS) src/io/outb.s -o build/outb.o
	$(AS) $(ASFLAGS) src/io/inb.s -o build/inb.o
	$(AS) $(ASFLAGS) src/segment/lgdt_f.s -o build/lgdt_f.o
	$(AS) $(ASFLAGS) src/segment/segment_selector.s -o build/config_segment_selector.o
	$(AS) $(ASFLAGS) src/segment/far_jump.s -o build/far_jump.o
	$(AS) $(ASFLAGS) src/interrupts/interrupt_handler.s -o build/interrupt_handler_asm.o
	$(AS) $(ASFLAGS) src/interrupts/load_lidt.s -o build/load_lidt.o

# Compilando código C
compile_c_file:
	@mkdir -p build
	$(CC) $(CFLAGS) src/kernel/$(OBJECTC).c -o build/$(OBJECTC).o
	$(CC) $(CFLAGS) src/io/io.c -o build/io_c.o
	$(CC) $(CFLAGS) src/io/serial_ports.c -o build/serial_ports.o
	$(CC) $(CFLAGS) src/segment/gdt.c -o build/gdt.o
	$(CC) $(CFLAGS) src/interrupts/keyboard.c -o build/keyboard.o
	$(CC) $(CFLAGS) src/interrupts/interrupt_handler.c -o build/interrupt_handler.o
	$(CC) $(CFLAGS) src/interrupts/pic.c -o build/pic.o
	$(CC) $(CFLAGS) src/interrupts/idt.c -o build/idt.o
	$(CC) $(CFLAGS) src/kernel/module_loader.c -o build/module_loader.o
	$(CC) $(CFLAGS) src/memory/pfa.c -o build/pfa.o
	$(CC) $(CFLAGS) src/memory/paging.c -o build/paging.o
	$(CC) $(CFLAGS) src/memory/kheap.c -o build/kheap.o

# Linkando o código objeto do OS com um kernel
link_kernel:
	@mkdir -p iso/boot
	ld $(LDFLAGS) $(OBJECTS) -o iso/boot/kernel.elf

# Gerando a ISO
generate_iso_image:
	@mkdir -p out
	genisoimage -R \
	-b boot/grub/stage2_eltorito \
	-no-emul-boot \
	-boot-load-size 4 \
	-A os \
	-input-charset utf8 \
	-quiet \
	-boot-info-table \
	-o out/os.iso \
	iso

clean:
	rm -rf build/* out/* iso/boot/kernel.elf || true

run: all
	bochs -f config/bochsrc.txt -q