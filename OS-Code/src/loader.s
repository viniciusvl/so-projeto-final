global loader

MAGIC_NUMBER equ 0x1BADB002
FLAGS        equ 0x0
CHECKSUM     equ -MAGIC_NUMBER

section .text
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

loader:
    mov esp, kernel_stack + KERNEL_STACK_SIZE

    ; Chama a função externa
    extern sum_of_three
    push dword 3                ; arg3
    push dword 2                ; arg2
    push dword 1                ; arg1
    call sum_of_three           ; o resultado estará em eax

.loop:
    jmp .loop                   ; Trava o CPU aqui após a função

section .bss                    ; Seção de DADOS NÃO INICIALIZADOS
align 4
KERNEL_STACK_SIZE equ 4096
kernel_stack:
    resb KERNEL_STACK_SIZE      ; Apenas reserva o espaço