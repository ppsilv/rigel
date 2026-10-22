#ifndef SYS_CRITICAL_H
#define SYS_CRITICAL_H
/* Secao critica curta: mascara todas as interrupcoes (mesmo metodo do scheduler.c).
 * cs_enter() devolve o SR anterior; cs_leave(sr) restaura (pode aninhar). */
#include <stdint.h>
#ifdef __m68k__
static inline uint16_t cs_enter(void)
{
    uint16_t sr;
    __asm__ volatile ("move.w %%sr,%0\n\tor.w #0x0700,%%sr" : "=d"(sr) : : "memory", "cc");
    return sr;
}
static inline void cs_leave(uint16_t sr)
{
    __asm__ volatile ("move.w %0,%%sr" : : "d"(sr) : "memory", "cc");
}
#else
uint16_t cs_enter(void);      /* no PC (testes) vem do mock */
void     cs_leave(uint16_t sr);
#endif
#endif
