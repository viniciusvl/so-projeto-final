/* Programa de teste em user mode.
   Invoca syscall via INT 0x80 para validar o sistema de syscalls. */

int main()
{
    int ret;

    /* Syscall 0 (SYSCALL_TEST): coloca 0 em EAX e dispara INT 0x80 */
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(0));

    return ret;
}
