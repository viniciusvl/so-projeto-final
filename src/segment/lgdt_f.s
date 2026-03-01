; Esse arquivo define a função que carrega
; o GDT na memória

global lgdt_f

lgdt_f:
    mov eax, [esp + 4] ; sistema pula 4 bytes da pilha, pois os 4 primeiros são para armazenar o retorno da função
    lgdt [eax]         ; salva o endereço em eax como endereço da tabela GDT
    ret                ; retorna função