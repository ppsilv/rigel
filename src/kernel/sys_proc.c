/*
 * sys_proc.c -- SYS_SLEEP e SYS_YIELD do kernel Rigel.
 *
 *   long sys_sleep(long ms)   espera NO MINIMO 'ms' milissegundos (resolucao de 10 ms) e retorna 0
 *   long sys_yield(void)      larga o resto da fatia de tempo e retorna 0 quando a task voltar a rodar
 *
 * Usam o contador systemTick (+1 a cada 10 ms) do kernel. O tick de 10 ms preempta a task
 * normalmente, entao as outras tasks continuam rodando enquanto esta espera.
 *
 * Limitacao: a task que dorme continua "na fila" e gasta a propria fatia girando num laco
 * (como o SYS_WAITPID). Um sleep que realmente tira a task da fila precisa de um gancho
 * dentro do OS_TickISR (para acordar quando o tempo acabar) e de um yield por software.
 */
#include <stdint.h>

#define MS_PER_TICK 10

extern uint32_t systemTick;                       /* +1 a cada 10 ms (kernel) */

static inline uint32_t now_tick(void)
{
    return *(volatile uint32_t *)&systemTick;     /* sempre le da memoria */
}

long sys_sleep(long ms)
{
    if (ms <= 0)
        return 0;

    /* +1 tick: o primeiro tick pode chegar a qualquer instante, entao so' assim
     * garantimos que passou pelo menos 'ms'. */
    uint32_t ticks = ((uint32_t)ms + (MS_PER_TICK - 1)) / MS_PER_TICK + 1;
    uint32_t start = now_tick();

    while ((uint32_t)(now_tick() - start) < ticks)
        ;                                         /* sem perigo com a volta do contador */
    return 0;
}

long sys_yield(void)
{
    /* Espera o proximo tick: nele o escalonador troca de task. Se houver outras
     * prontas, esta so' volta aqui quando chegar a sua vez de novo. */
    uint32_t t = now_tick();
    while (now_tick() == t)
        ;
    return 0;
}
