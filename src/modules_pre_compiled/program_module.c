/*
 * Teste de cooperative scheduling (fork + yield).
 *
 * Como o agendamento e cooperativo, cada processo precisa chamar yield
 * voluntariamente para ceder CPU ao proximo READY.
 */

static int sys_test(void)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(0));
    return ret;
}

static int sys_fork(void)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(3));
    return ret;
}

static int sys_yield(void)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(4));
    return ret;
}

int main(void)
{
    int pid = sys_fork();

    if (pid < 0) {
        for (;;) {
            sys_test();
        }
    }

    if (pid == 0) {
        /* Caminho do filho: faz trabalho e cede CPU. */
        for (;;) {
            sys_test();
            sys_yield();
        }
    }

    /* Caminho do pai: tambem coopera chamando yield em loop. */
    for (;;) {
        sys_test();
        sys_yield();
    }
}
