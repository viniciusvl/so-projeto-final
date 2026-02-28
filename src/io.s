; Este arquivo define a função usada para mover o cursor na tela
; A função outb envia apenas 8 bits, mas o protocolo do display
; exige 16 bits. Assim, precisamos chamar a função 2x

global outb

outb:                   
    mov al, [esp + 8]   ; armazena posição do cursor
    mov dx, [esp + 4]   ; armazena endereço da porta
    out dx, al          ; manda para a porta o conteúdo do cursor
    ret                 ; retorna função