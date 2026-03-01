extern interrupt_handler

%macro no_error_code_interrupt_handler 1

global interrupt_handler_%1

interrupt_handler_%1:
    push dword 0        ; push falso error code
    push dword %1       ; coloca o número de interrupção
    jmp common_interrupt_handler

%endmacro

; função que executa passos que as interrupções tem em comum
common_interrupt_handler: 
    push eax
    push ebx

    ; o número da interrupção está em [esp + 8] (depois de eax e ebx)
    push dword [esp + 8]
    call interrupt_handler
    add esp, 4

    ; restaurar registradores
    pop ebx
    pop eax

    add esp, 8

    iret

; IRQ1 - teclado (interrupção 33)
no_error_code_interrupt_handler 33