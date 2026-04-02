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
    ; Guarda valores antigos dos registradores
    push eax 
    push ebx

    ; Adiciona na pilha o número de interrupção que foi lançado
    ; dword informa quantos bytes ler 
    push dword [esp + 8]

    ; chama função do C
    call interrupt_handler
    
    ; Retira valor da interrupção do topo da pilha
    add esp, 4

    ; pop retira da pilha e envia para registradores
    pop ebx
    pop eax

    ; Faz pilha voltar ao estado original
    add esp, 8

    iret

; IRQ1 - teclado (interrupção 33)
no_error_code_interrupt_handler 33