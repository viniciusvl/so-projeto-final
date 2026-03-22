/* Este arquivo é de linker do kernel */
/* - Define o começo do código */
/* - Define qual o tipo de dados de cada seção */
/* - Define o tamanho da página de memória adotada. No caso, 4MB para paginação */
/* - O kernel é relocado para 3GB + 1MB (0xC0100000) no espaço virtual */
/* - Mas carregado fisicamente em 1MB (0x00100000) via AT() */

ENTRY(loader)

SECTIONS {
    . = 0xC0100000; /* Código relocado para 3GB + 1MB no espaço virtual */

    /* Labels do começo do kernel (virtual/físico) */
    kernel_virtual_start = .;
    kernel_physical_start = . - 0xC0000000;


    .multiboot ALIGN(4) : AT(ADDR(.multiboot) - 0xC0000000)
    /* GRUB encontra o header na posição física 0x00100000 */
    {
        *(.multiboot)
    }

    .text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000)
    /* Código carregado fisicamente em 1MB; virtualmente em 3GB+1MB */
    {
        *(.text) /* Seção que armazena as instruções */
    }

    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata*) /* Armazena as variáveis constantes */
    }

    .data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data) /* Variáveis globais ou estáticas inicializadas */
    }

    .bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON) /* Armazena variáveis globais NÃO INICIALIZADAS */
        *(.bss)   /* Variáveis estáticas que não tiveram valor declarado */
    }

    /* Labels do fim do kernel (virtual/físico) */
    kernel_virtual_end = .;
    kernel_physical_end = . - 0xC0000000;
}

