#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

/*
    Função que envia a posição do cursor para o display

    @param data: posição do cursor
*/
void outb(unsigned short port, unsigned char data);

#endif