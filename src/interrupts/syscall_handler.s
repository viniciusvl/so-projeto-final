; syscall_handler.s - Handler Assembly para INT 0x80 (syscalls)
;
; Quando o user mode executa "int 0x80", o processador:
;   1. Troca para a stack do kernel (ss0:esp0 do TSS)
;   2. Empilha ss, esp, eflags, cs, eip do user mode
;   3. Salta para este handler
;
; Convencao: EAX contem o numero da syscall

extern syscall_dispatcher

global syscall_handler_128

syscall_handler_128:
    ; Salva registradores de uso geral
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    ; Passa para o dispatcher:
    ;   arg1: numero da syscall (EAX salvo)
    ;   arg2: ponteiro para a syscall_frame atual (ESP apos pushes)
    mov eax, [esp + 24]
    lea ecx, [esp]
    push ecx
    push eax
    call syscall_dispatcher
    add esp, 8

    ; Coloca o valor de retorno (eax) no slot salvo de eax na stack
    ; Para que ao restaurar, eax contenha o retorno da syscall
    mov [esp + 24], eax

    ; Restaura registradores
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax         ; agora eax = valor de retorno do dispatcher

    iret
