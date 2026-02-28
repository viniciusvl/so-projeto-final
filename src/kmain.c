/* Funções de C */

#include "io.h"

int sum_of_three(int arg1, int arg2, int arg3)
{
    return arg1 + arg2 + arg3;
}

/*
    Para escrever um caractere na tabela, apontamos o ponteiro
fb para a área de memória RESERVADA para o FRAMEBUFFER. 

    O caractere ocupa a posição fb[i] e fb[i+1], pois utiliza 
16 bits para representar

    @param i: posição que ocupa na tela
    @param c: valor do char
    @param fg: cor da letra
    @param bg: cor do fundo
*/

char *fb = (char *) 0x000B8000;
void fb_write_cell(unsigned int posicaoChar, char c, unsigned char fg, unsigned char bg){
    fb[posicaoChar] = c; 
    
    // (fg & 0x0F) é uma operação bit a bit para garantir que o número seja
    // menor que 15, e << 4 move os 4 bits para a esquerda
    // Por fim, junta os dois binários da cor em um só
    fb[posicaoChar + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F);
}


#define FB_COMMAND_PORT 0x3D4 // porta para enviar 
#define FB_DATA_PORT 0x3D5

#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND 15

// unsigned short é usado porque mede 16 bits (formato do bit do cursor)
void fb_move_cursor(unsigned short pos){
    /*
        Informa ao registrador do framebuffer que o
    próximo dado está nos 8 bits mais significativos
    */
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    /*
        O registrador armazena os 8 bits da parte alta,
    'pos' é uma variável de 16 bits, então não conseguimos
    enviar diretamente para o registrador. 
        Então, deslocamos os 8 bits da parte alta para a parte
    baixa, porque lá no registrador eles voltam para a parte 
    alta.
        0x00FF deixa a parte alta 0 e a parte baixa com
    seu valor original  
    */
    outb(FB_DATA_PORT, ((pos >> 8) & 0x00FF));
    /*
        Informa que os próximos bits são para a parte baixa
    */
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    /*
        Envia a parte baixa da variável para 
    o registrador que armazena a parte baixa.
        Não necessita deslocar bits
    */
    outb(FB_DATA_PORT, pos & 0x00FF);
}

void fb_write(char *buf, unsigned int len){
    char *caractere = buf;
    unsigned short pos_cursor = 0;
    unsigned int pos_char = 0;

    fb_move_cursor(pos_cursor);
    
    for (unsigned int i = 0; i < len; i++){
        fb_write_cell(pos_char, caractere[i], 10, 15);
        
        pos_char += 2;
        pos_cursor++;
        
        fb_move_cursor(pos_cursor);
    }
}


void kmain(){
    char s[12] = "hello, world";

    fb_write(s, 12);
    //fb_write_cell(0, 'A', 10, 15);
}