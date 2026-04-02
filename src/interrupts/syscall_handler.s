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

    ; Passa o numero da syscall (valor original de eax) como argumento
    ; eax foi o primeiro push, esta em [esp + 24] (6 pushes * 4 bytes acima)
    push dword [esp + 24]
    call syscall_dispatcher
    add esp, 4

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
