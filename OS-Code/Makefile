.PHONY: lisrun compile_assembly link_kernel create_folder generate_iso_image config_bootloader
lisrun: compile_assembly link_kernel create_folder config_bootloader generate_iso_image create_bochs_file

# Compilando o OS
compile_assembly:
	nasm -f elf32 src/loader.s -o out/loader.o

# Linkando o código objeto do OS com um kernel
link_kernel:
	ld -T src/link.ld -melf_i386 out/loader.o -o kernel.elf

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

# || true => Continua apagando mesmo que uma falhe
clean: # Apaga todos os arquivos
	rm -r iso || true
	rm out/loader.o || true
	rm kernel.elf || true
	rm os.iso || true
	rm *.txt

create_bochs_file:
	touch bochsrc.txt
	printf "megs: 32\n" > bochsrc.txt
	printf "display_library: x\n" >> bochsrc.txt
	printf "romimage: file=/usr/share/bochs/BIOS-bochs-legacy\n" >> bochsrc.txt
	printf "vgaromimage: file=/usr/share/bochs/VGABIOS-lgpl-latest\n" >> bochsrc.txt
	printf "ata0-master: type=cdrom, path=os.iso, status=inserted\n" >> bochsrc.txt
	printf "boot: cdrom\n" >> bochsrc.txt
	printf "log: bochslog.txt\n" >> bochsrc.txt
	printf "clock: sync=realtime, time0=local\n" >> bochsrc.txt
	printf "cpu: count=1, ips=1000000\n" >> bochsrc.txt

run:
	bochs -f bochsrc.txt -q