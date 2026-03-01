; Esse arquivo define para onde os
; registradores de segmento devem apontar

global config_segment_selector

config_segment_selector:
    mov ax, [esp + 4]  ; usei ax, porque armazena 16 bits de baixo
    mov ds, ax         ; segment selectors armazenam 16 bits somente
    mov ss, ax
    mov es, ax
    ret
