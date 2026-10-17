/*
 * sys_proc.c -- SYS_SLEEP, SYS_YIELD e SYS_GETPID do kernel Rigel.
 *
 *   long sys_sleep(long ms)   espera NO MINIMO 'ms' milissegundos (resolucao de 10 ms) e retorna 0
 *   long sys_yield(void)      larga o resto da fatia de tempo e retorna 0 quando a task voltar a rodar
 *
 * sys_sleep tira a task do round robin (OS_SleepTicks): enquanto dorme ela nao gasta CPU e o
 * proprio tick de 10 ms a acorda. sys_yield troca de task na hora (OS_Yield).
 */
#include <stdint.h>

#include <sys/scheduler.h>        /* OS_SleepTicks, OS_Yield, current_task */

#define MS_PER_TICK 10

long sys_sleep(long ms)
{
    uint32_t ticks;

    if (ms <= 0)
        return 0;

    /* +1 tick: o primeiro tick pode chegar a qualquer instante, entao so' assim
     * garantimos que passou pelo menos 'ms'. */
    ticks = ((uint32_t)ms + (MS_PER_TICK - 1)) / MS_PER_TICK + 1;
    OS_SleepTicks(ticks);
    return 0;
}

long sys_yield(void)
{
    OS_Yield();
    return 0;
}

long sys_getpid(void)
{
    return current_task ? (long)current_task->id : 0;
}