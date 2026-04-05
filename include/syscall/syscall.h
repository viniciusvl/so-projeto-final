#ifndef INCLUDE_SYSCALL_H
#define INCLUDE_SYSCALL_H

#include <stdint.h>

/* Numeros das syscalls */
#define SYSCALL_TEST  0
#define SYSCALL_WRITE 1
#define SYSCALL_EXEC  2
#define SYSCALL_FORK  3
#define SYSCALL_YIELD 4
#define SYSCALL_EXIT  5

/*
 * Layout da stack no syscall handler apos salvar registradores:
 *   ebp, edi, esi, edx, ecx, ebx, eax, eip, cs, eflags, user_esp, user_ss
 */
struct syscall_frame {
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t user_ss;
};

/*
    Dispatcher chamado pelo handler assembly.
    Recebe o numero da syscall em syscall_number (valor de EAX)
    e retorna o resultado em EAX.
*/
uint32_t syscall_dispatcher(uint32_t syscall_number, struct syscall_frame *frame);

#endif
