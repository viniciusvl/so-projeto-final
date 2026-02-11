global loader                   ; Usado pelo Linker para referenciar o código do SO          

MAGIC_NUMBER equ 0x1BADB002     ; Define o número mágico lido pelo GRUB
FLAGS        equ 0x0            ; Configurações do GRUB
CHECKSUM     equ -MAGIC_NUMBER  ; Interamente, o GRUB verifica o número por MAGIC_NUMBER + CHECKSUM + FLAGS = 0

section .text:                  ; Indica para o Assembler onde o código começa
align 4                         ; Avisa para o Assembler começar em endereços múltiplos de 4
    dd MAGIC_NUMBER             ; A arquitetura 32 bits lê 4 bytes de uma vez
    dd FLAGS                    ; Se instruções/dados começam em endereços não
    dd CHECKSUM                 ; múltiplos de 4, então há problema de leitura
                                ; dd é uma diretiva que armazena esses valores no ELF

loader:
    mov eax, 0xCAFEBABE         ; colocar CAFEBABABE em um registrador

.loop:
    jmp .loop                   ; sempre em loop, para manter em memória

