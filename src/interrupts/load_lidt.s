; Esse arquivo carrega a tabela de descrição de interrupções
; Quando o usuário digita, o teclado envia para a CPU
; Uma interrupção junto de um número
; Interrupções são implementações em hardware

global load_lidt

load_lidt:
    mov eax, [esp + 4]  ; sobe o ponteiro da stack 4 bytes para ler o endereçco
    lidt [eax]          ; informa ao computador onde está a IDT
    ret
