/*
 * kmalloc_lock.c -- torna kmalloc/kfree "atomicos" para todas as tasks.
 *
 * Problema: o tick de 10 ms pode trocar de task NO MEIO do kmalloc. Se outra task (tcpip, w5100rx
 * ou o shell) chamar kmalloc/kfree nesse instante, ela mexe numa lista de blocos que esta' pela
 * metade e o heap se corrompe -- o sistema trava depois de um tempo, sem mensagem.
 *
 * Solucao: o linker troca toda chamada externa a kmalloc/kfree por __wrap_kmalloc/__wrap_kfree
 * (opcao  -Wl,--wrap=kmalloc  do ld), que chamam o kmalloc original (__real_kmalloc) com as
 * interrupcoes mascaradas. Nao precisa mexer no kmalloc.c.
 *
 * Se o seu kmalloc.c tiver tambem kcalloc/krealloc, acrescente --wrap=kcalloc --wrap=krealloc
 * e as funcoes equivalentes abaixo (mesmo padrao).
 */
#include <stdint.h>
#include <stddef.h>

extern void *__real_kmalloc(size_t n);
extern void  __real_kfree(void *p);

#ifdef __m68k__
static inline uint16_t irq_save(void)
{
    uint16_t sr;
    __asm__ volatile ("move.w %%sr,%0\n\tor.w #0x0700,%%sr"
                      : "=d"(sr) : : "memory", "cc");
    return sr;
}
static inline void irq_restore(uint16_t sr)
{
    __asm__ volatile ("move.w %0,%%sr" : : "d"(sr) : "memory", "cc");
}
#else
static inline uint16_t irq_save(void) { return 0; }
static inline void irq_restore(uint16_t sr) { (void)sr; }
#endif

void *__wrap_kmalloc(size_t n)
{
    uint16_t sr = irq_save();
    void *p = __real_kmalloc(n);
    irq_restore(sr);
    return p;
}

void __wrap_kfree(void *p)
{
    uint16_t sr = irq_save();
    __real_kfree(p);
    irq_restore(sr);
}
