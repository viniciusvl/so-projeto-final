OBJECTS = build/loader.o build/kmain.o build/outb.o build/inb.o build/io_c.o build/serial_ports.o build/lgdt_f.o build/config_segment_selector.o build/far_jump.o build/gdt.o build/keyboard.o build/interrupt_handler.o build/interrupt_handler_asm.o build/timer_handler_asm.o build/pic.o build/load_lidt.o build/idt.o build/pit.o build/module_loader.o build/pfa.o build/paging.o build/kheap.o build/process.o build/scheduler.o build/user_mode.o build/tss.o build/load_tss.o build/syscall_handler_asm.o build/syscall.o

OBJECTC = kmain
OBJECTA = loader

CC = gcc

CFLAGS = -Iinclude -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c
ASFLAGS = -f elf32
LDFLAGS = -T linker/link.ld -melf_i386

AS = nasm

ASFLAGS = -f elf32

.PHONY: all compile_assembly compile_c_file link_kernel generate_iso_image run clean
all: clean compile_assembly compile_c_file compile_modules link_kernel generate_iso_image

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
	$(AS) $(ASFLAGS) src/interrupts/timer_handler.s -o build/timer_handler_asm.o
	$(AS) $(ASFLAGS) src/interrupts/load_lidt.s -o build/load_lidt.o
	$(AS) $(ASFLAGS) src/process/user_mode.s -o build/user_mode.o
	$(AS) $(ASFLAGS) src/segment/load_tss.s -o build/load_tss.o
	$(AS) $(ASFLAGS) src/interrupts/syscall_handler.s -o build/syscall_handler_asm.o

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
	$(CC) $(CFLAGS) src/interrupts/pit.c -o build/pit.o
	$(CC) $(CFLAGS) src/kernel/module_loader.c -o build/module_loader.o
	$(CC) $(CFLAGS) src/memory/pfa.c -o build/pfa.o
	$(CC) $(CFLAGS) src/memory/paging.c -o build/paging.o
	$(CC) $(CFLAGS) src/memory/kheap.c -o build/kheap.o
	$(CC) $(CFLAGS) src/process/process.c -o build/process.o
	$(CC) $(CFLAGS) src/scheduler/scheduler.c -o build/scheduler.o
	$(CC) $(CFLAGS) src/segment/tss.c -o build/tss.o
	$(CC) $(CFLAGS) src/syscall/syscall.c -o build/syscall.o

# Compilando módulos do userspace
compile_modules:
	@mkdir -p build/modules iso/modules
	$(AS) -f elf32 src/modules_pre_compiled/starter.s -o build/modules/starter.o
	$(CC) -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -fno-pic -c src/modules_pre_compiled/task_short.c -o build/modules/task_short.o
	$(CC) -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -fno-pic -c src/modules_pre_compiled/task_medium.c -o build/modules/task_medium.o
	$(CC) -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -fno-pic -c src/modules_pre_compiled/task_long.c -o build/modules/task_long.o
	ld -T linker/link_modules.ld -melf_i386 build/modules/starter.o build/modules/task_short.o -o iso/modules/task_short
	ld -T linker/link_modules.ld -melf_i386 build/modules/starter.o build/modules/task_medium.o -o iso/modules/task_medium
	ld -T linker/link_modules.ld -melf_i386 build/modules/starter.o build/modules/task_long.o -o iso/modules/task_long

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
	rm -rf build/* out/* iso/boot/kernel.elf iso/modules/task_short iso/modules/task_medium iso/modules/task_long || true

run: all
	bochs -f config/bochsrc.txt -q