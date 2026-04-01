; user_mode.s - Funções Assembly para entrar em User Mode (ring 3)

global update_cr3
global enter_user_mode

; void update_cr3(uint32_t pdt_physical)
; Atualiza o registrador cr3 para apontar para a nova Page Directory
update_cr3:
    mov eax, [esp + 4]      ; endereco fisico da PDT
    mov cr3, eax
    ret

; void enter_user_mode(uint32_t ss, uint32_t esp, uint32_t eflags, uint32_t cs, uint32_t eip)
; Monta o stack frame para iret e entra em user mode (ring 3)
;
; O iret espera na stack:
;   [esp + 16] ss
;   [esp + 12] esp (user)
;   [esp +  8] eflags
;   [esp +  4] cs
;   [esp +  0] eip
enter_user_mode:
    ; Salva os parametros em registradores antes de montar a stack
    mov eax, [esp + 4]      ; ss
    mov ebx, [esp + 8]      ; user esp
    mov ecx, [esp + 12]     ; eflags
    mov edx, [esp + 16]     ; cs
    mov esi, [esp + 20]     ; eip

    ; Configura os registradores de dados para o segmento user data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Monta o stack frame que iret espera
    push eax                ; ss
    push ebx                ; user esp
    push ecx                ; eflags
    push edx                ; cs
    push esi                ; eip
    iret
