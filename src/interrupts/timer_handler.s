; timer_handler.s - Handler dedicado da IRQ0 (PIT)
;
; Salva registradores gerais e passa ponteiro do frame para C.

extern timer_interrupt_handler

global interrupt_handler_32

interrupt_handler_32:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    lea eax, [esp]
    push eax
    call timer_interrupt_handler
    add esp, 4

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax

    iret
