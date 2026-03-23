#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

/*
    Função que envia dados para uma porta de IO

    @param port: endereço da porta
    @param data: dado a ser enviado
*/
void outb(unsigned short port, unsigned char data);

/*
    Função que lê dados de uma porta de IO

    @param port: endereço da porta
    @return: dado lido da porta
*/
unsigned char inb(unsigned short port);

/*
    Escreve um caractere em uma posição do framebuffer

    @param posicaoChar: posição no buffer (offset em bytes)
    @param c: valor do char
    @param fg: cor da letra
    @param bg: cor do fundo
*/
void fb_write_cell(unsigned int posicaoChar, char c, unsigned char fg,
                   unsigned char bg);

/*
    Move o cursor do framebuffer para a posição desejada

    @param pos: posição lógica do cursor (16 bits)
*/
void fb_move_cursor(unsigned short pos);

/* Enumerações de Cores VGA Padrão */
#define FB_COLOR_BLACK         0
#define FB_COLOR_BLUE          1
#define FB_COLOR_GREEN         2
#define FB_COLOR_CYAN          3
#define FB_COLOR_RED           4
#define FB_COLOR_MAGENTA       5
#define FB_COLOR_BROWN         6
#define FB_COLOR_LIGHT_GREY    7
#define FB_COLOR_DARK_GREY     8
#define FB_COLOR_LIGHT_BLUE    9
#define FB_COLOR_LIGHT_GREEN   10
#define FB_COLOR_LIGHT_CYAN    11
#define FB_COLOR_LIGHT_RED     12
#define FB_COLOR_LIGHT_MAGENTA 13
#define FB_COLOR_YELLOW        14
#define FB_COLOR_WHITE         15

/*
    Escreve uma string no framebuffer com cores

    @param buf: ponteiro para a string
    @param len: tamanho da string
    @param fg: cor da letra
    @param bg: cor do fundo
*/
void fb_write_colored(char *buf, unsigned int len, unsigned char fg, unsigned char bg);

/*
    Escreve uma string no framebuffer com as cores padrão
*/
void fb_write(char *buf, unsigned int len);

/*
    Limpa a tela preenchendo o framebuffer com espaços
    e reposiciona o cursor no início
*/
void fb_clear();

/* Serial Ports */
#define SERIAL_COM1_BASE 0x3F8

void serial_configure_baud_rate(unsigned short com, unsigned short divisor);
void serial_configure_line(unsigned short com);
void serial_configure_buffer(unsigned short com);
void serial_configure_modem(unsigned short com);
void serial_write(unsigned short com, char *log);

#endif
