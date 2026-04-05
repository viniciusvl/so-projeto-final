/*
 * task_medium.c
 * Carga CPU-bound media para benchmark de escalonador.
 */

#define SYSCALL_EXIT 5

static void sys_exit(int status)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYSCALL_EXIT), "b"(status));
    (void)ret;
}

static void do_cpu_work(volatile unsigned int iterations)
{
    volatile unsigned int i;
    volatile unsigned int acc = 0;

    for (i = 0; i < iterations; i++)
        acc += (i & 0xFU);

    (void)acc;
}

int main(void)
{
    do_cpu_work(10000000U);
    sys_exit(0);

    for (;;)
        __asm__ volatile("hlt");
}
