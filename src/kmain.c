/* Funções de C */

int sum_of_three(int arg1, int arg2, int arg3)
{
    return arg1 + arg2 + arg3;
}

/*
    Para escrever um caractere na tabela, apontamos o ponteiro
fb para a área de memória RESERVADA para o FRAMEBUFFER. 

    O caractere ocupa a posição fb[i] e fb[i+1], pois utiliza 
16 bits para representar

Parâmetros da função fb_write_cell():
    unsigned int i: posição que ocupa na tela
    char c: valor do char
    unsigned char fg: cor da letra
    unsigned char bg: cor do fundo
*/

char *fb = (char *) 0x000B8000;
void fb_write_cell(unsigned int posicaoChar, char c, unsigned char fg, unsigned char bg){
    fb[posicaoChar] = c; 
    
    // (fg & 0x0F) é uma operação bit a bit para garantir que o número seja
    // menor que 15, e << 4 move os 4 bits para a esquerda
    // Por fim, junta os dois binários da cor em um só
    fb[posicaoChar + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F);
}