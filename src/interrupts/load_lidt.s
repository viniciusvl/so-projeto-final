; Esse arquivo carrega a tabela de descrição de interrupções

global load_lidt

load_lidt:
    mov eax, [esp + 4]  ; sobe o ponteiro da stack 4 bytes para ler o endereçco
    lidt [eax]          ; informa ao computador onde está a IDT
    ret
