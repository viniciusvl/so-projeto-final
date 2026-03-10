global loader
extern kmain

MAGIC_NUMBER equ 0x1BADB002
FLAGS        equ 0x00000001
CHECKSUM     equ -(MAGIC_NUMBER + FLAGS)

section .multiboot
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

section .text
loader:
    cli
    mov esp, kernel_stack + KERNEL_STACK_SIZE
    push ebx
    call kmain

.loop:
    jmp .loop

section .bss
align 4
KERNEL_STACK_SIZE equ 4096

kernel_stack:
    resb KERNEL_STACK_SIZE