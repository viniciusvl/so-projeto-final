#include "syscall/syscall.h"
#include "io/serial_ports.h"

uint32_t syscall_dispatcher(uint32_t syscall_number)
{
    serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall recebida: ");
    serial_write_hex(syscall_number);

    switch (syscall_number) {
        case SYSCALL_TEST:
            serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall_test chamada!");
            return 0;

        case SYSCALL_WRITE:
            serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall_write chamada!");
            return 0;

        default:
            serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall desconhecida!");
            return (uint32_t)-1;
    }
}
