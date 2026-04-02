#ifndef INCLUDE_SYSCALL_H
#define INCLUDE_SYSCALL_H

#include <stdint.h>

/* Numeros das syscalls */
#define SYSCALL_TEST  0
#define SYSCALL_WRITE 1

/*
    Dispatcher chamado pelo handler assembly.
    Recebe o numero da syscall em syscall_number (valor de EAX)
    e retorna o resultado em EAX.
*/
uint32_t syscall_dispatcher(uint32_t syscall_number);

#endif
