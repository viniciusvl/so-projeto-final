; Esse arquivo configura o comando 'in' que lê o conteúdo de uma porta

global inb

inb: 
    mov dx, [esp + 4]   ; lê 4 bytes da pilha que contém o endereço da porta 
    in al, dx           ; move o conteúdo da porta para um registrador
    ret                 ; retorna