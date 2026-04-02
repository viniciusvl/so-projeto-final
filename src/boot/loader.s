global loader
global page_directory
global kernel_stack
global kernel_stack_top
extern kmain

MAGIC_NUMBER        equ 0x1BADB002
FLAGS               equ 0x00000001
CHECKSUM            equ -(MAGIC_NUMBER + FLAGS)
KERNEL_VIRTUAL_BASE equ 0xC0000000  ; offset entre endereço virtual (3GB) e físico

section .multiboot
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

section .data
align 4096
; Page Directory com 1024 entradas (4 bytes cada = 4096 bytes = 1 página)
; Com PSE ativo, cada entrada cobre 4MB de memória física
page_directory:
    dd 0x00000083               ; entry 0: identity map
    times (768 - 1) dd 0        
    dd 0x00000083               ; 3GB do kernel: higher half 
    times (1024 - 769) dd 0     

section .text
loader:
    cli
    mov edi, ebx                ; salva ponteiro multiboot 

    mov eax, page_directory     ; endereço virtual da PDT 
    sub eax, KERNEL_VIRTUAL_BASE ; converte para endereço físico real

    mov cr3, eax                ; carrega PDT no cr3

    mov ebx, cr4                ; lê cr4 atual
    or  ebx, 0x00000010         ; set PSE: habilita páginas de 4MB (bit 4)
    mov cr4, ebx                ; atualiza cr4

    mov ebx, cr0                ; lê cr0 atual
    or  ebx, 0x80000000         ; set PG: ativa paginação (bit 31)
    mov cr0, ebx                ; atualiza cr0
                                ; paginação ativada

    lea ebx, [higher_half]      ; carrega endereço virtual do label (0xC01XXXXX)
    jmp ebx                     ; salta para o higher half; eip > 0xC0000000

higher_half:
    mov esp, kernel_stack + KERNEL_STACK_SIZE ; configura pilha no higher half

    add edi, KERNEL_VIRTUAL_BASE ; ajusta ponteiro multiboot para endereço virtual
                                 ; físico <1MB -> virtual 0xC00XXXXX via PDT[768]
    push edi                    ; argumento para kmain
    call kmain

.loop:
    jmp .loop

section .bss
align 4
KERNEL_STACK_SIZE equ 4096

kernel_stack:
    resb KERNEL_STACK_SIZE
kernel_stack_top:
