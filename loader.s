global loader                   

MAGIC_NUMBER equ 0x1BADB002     ; Define o número mágico lido pelo GRUB
FLAGS        equ 0x0            ; Configurações do GRUB
CHECKSUM     equ -MAGIC_NUMBER  ; Interamente, o GRUB verifica o número por MAGIC_NUMBER + CHECKSUM + FLAGS = 0

