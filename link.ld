/* Esse arquivo é responsável por estruturar as seções de memória do SO para o Bootloader */
/* Necessário, pois o Bootloader precisa saber em qual parte da memória / espaço armazenar instruções*/

ENTRY(loader)

SECTIONS {
    . = 0x00100000; /* Endereço inicial. Antes desse, espaço reservado para GRUB*/

    .text ALIGN (0x1000) : 
    {
        *(.text)            /* Armazena código fonte*/
    }

    .rodata ALIGN (0x1000) :
    {
        *(.rodata*)         /* Armazena variáveis constantes*/
    }

    .data ALIGN (0x1000) :
    {
        *(.data)            /* Variáveis que já possuem uma inicialização */
    }

    .bss ALIGN (0x1000) :   /* Armazena variáveis declaradas, mas sem valor atribuido */
    {
        *(COMMON)           
        *(.bss)

    }

}