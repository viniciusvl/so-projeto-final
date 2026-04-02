; load_tss.s - Carrega o segment selector do TSS no registrador TR

global load_tss

load_tss:
    mov ax, [esp + 4]   ; segment selector do TSS (0x28)
    ltr ax               ; carrega no Task Register
    ret
