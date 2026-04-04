/*
 * Teste de preempcao por timer (PIT):
 * - Cria pai + filho via fork.
 * - Nenhum dos dois chama yield.
 * - Cada processo faz trabalho CPU-bound e, periodicamente, dispara
 *   syscalls para gerar log serial e permitir observar alternancia.
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

static int sys_write(void)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(1));
    return ret;
}

static void busy_work(volatile unsigned int rounds)
{
    volatile unsigned int i;

    for (i = 0; i < rounds; i++) {
        __asm__ volatile("nop");
    }
}

int main(void)
{
    int pid = sys_fork();
    unsigned int heartbeat = 0;

    if (pid < 0) {
        for (;;) {
            sys_test();
            busy_work(200000U);
        }
    }

    if (pid == 0) {
        /* Filho: sem yield, apenas carga de CPU + heartbeat. */
        for (;;) {
            busy_work(350000U);
            heartbeat++;

            if ((heartbeat & 0x1F) == 0) {
                sys_test();
                sys_write();
            }
        }
    }

    /* Pai: frequencia de heartbeat diferente para facilitar observacao. */
    for (;;) {
        busy_work(180000U);
        heartbeat++;

        if ((heartbeat & 0x3F) == 0) {
            sys_test();
        }
    }
}
