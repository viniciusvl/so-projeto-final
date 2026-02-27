/* Este arquivo é de linker do kernel */
/* - Define o começo do código */
/* - Define qual o tipo de dados de cada seção */
/* - Define o tamanho da página de memória adotada. No caso, 4KB*/
/* - Quando uma seção ocupa mais do que 4KB, então é criada uma segunda página com + 4KB*/

/* Esse arquivo aloca e define as áreas de memória e tipo de dados, para facilitar a execução*/

ENTRY(loader)

SECTIONS {
    . = 0x00100000; /* Código carregado após 1MB, pois <1MB é reservado para GRUB */

    .text ALIGN (0x1000) : /* Para paginação, usa-se tamanho 4KB*/
    {
        *(.text) /* Seção que armazena as instruções */
    }

    .rodata ALIGN (0x1000) :
    {
        *(.rodata*) /* Armazena as variáveis contantes */
    }

    .data ALIGN (0x1000) : 
    {
        *(.data) /* Variáveis globais ou estáticas inicializadas */
    }

    .bss ALIGN (0x1000) : /* align at 4 KB */
    {
        *(COMMON) /* Armazena variaveis globais NÃO INICIALIZADAS */
        *(.bss) /* Variáveis estáticas que não tiveram valor declarado */
    }
}