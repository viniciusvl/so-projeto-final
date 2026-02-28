global loader

MAGIC_NUMBER equ 0x1BADB002
FLAGS        equ 0x0
CHECKSUM     equ -MAGIC_NUMBER

section .text
align 4                     ; O CPU lê 4 bytes em chuncks para aumentar perfomance
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

loader:
    mov esp, kernel_stack + KERNEL_STACK_SIZE

    extern kmain
    call kmain

.loop:
    jmp .loop                   ; Trava o CPU aqui após a função

section .bss                    ; Seção de DADOS NÃO INICIALIZADOS
align 4
KERNEL_STACK_SIZE equ 4096
kernel_stack:
    resb KERNEL_STACK_SIZE      ; Apenas reserva o espaço