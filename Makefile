OBJECTS = loader.o kmain.o outb.o inb.o io_c.o serial_ports.o
OBJECTC = kmain
OBJECTA = loader

CC = gcc

CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
-nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c

LDFLAGS = -T src/link.ld -melf_i386

AS = nasm

ASFLAGS = -f elf32

.PHONY: all compile_assembly compile_c_file link_kernel create_folder config_bootloader generate_iso_image run clean
all: clean compile_assembly compile_c_file link_kernel create_folder config_bootloader generate_iso_image

# Compilando o OS
compile_assembly:
	$(AS) $(ASFLAGS) src/$(OBJECTA).s -o $(OBJECTA).o
	$(AS) $(ASFLAGS) src/io/outb.s -o outb.o
	$(AS) $(ASFLAGS) src/io/inb.s -o inb.o

# Compilando código C
compile_c_file:
	$(CC) $(CFLAGS) src/$(OBJECTC).c -o $(OBJECTC).o
	$(CC) $(CFLAGS) src/io/io.c -o io_c.o
	$(CC) $(CFLAGS) src/io/serial_ports.c -o serial_ports.o

# Linkando o código objeto do OS com um kernel
link_kernel:
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

# Criando a pasta que irá gerar a imagem do SO
create_folder:
	mkdir -p iso/boot/grub
	cp src/stage2_eltorito iso/boot/grub/
	cp kernel.elf iso/boot/

# Gerando o arquivo de configuração do bootloader
config_bootloader:
	touch iso/boot/grub/menu.lst
	printf "default=0\n" > iso/boot/grub/menu.lst
	printf "timeout=0\n\n" >> iso/boot/grub/menu.lst
	printf "title os\n" >> iso/boot/grub/menu.lst
	printf "kernel /boot/kernel.elf" >> iso/boot/grub/menu.lst

# Gerando a ISO
generate_iso_image:
	genisoimage -R \
	-b boot/grub/stage2_eltorito \
	-no-emul-boot \
	-boot-load-size 4 \
	-A os \
	-input-charset utf8 \
	-quiet \
	-boot-info-table \
	-o os.iso \
	iso

clean:
	rm -rf *.o kernel.elf os.iso iso out || true

run: all
	bochs -f src/bochsrc.txt -q