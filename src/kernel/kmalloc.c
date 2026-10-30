/*

Perfeito — isso simplifica bastante, é um heap único, sem precisar isolar por task. 
Vou te dar a família completa: kmalloc_init, kmalloc, kfree, kcalloc e krealloc.

Duas decisões de design que valem explicação antes do código:

Proteção contra interrupção — agora que o systick preempta tasks de verdade, se o 
OS_TickISR/Int2Handler disparar bem no meio de um kmalloc mexendo nos headers da 
lista, e a task escolhida em seguida também chamar kmalloc, a lista fica corrompida. 

Por isso usei o mesmo padrão LOCK/UNLOCK (desliga interrupção) que você já usa no 
ata.c — aqui é ainda mais crítico porque essa heap vai ser mexida por qualquer parte 
do kernel a qualquer momento.

Header de 4 bytes só — no 68000 puro, acesso a word/long em endereço ímpar dá address 
error (trap). Fiz o header caber em exatamente 4 bytes (uint32_t, usando o bit mais 
alto como flag "livre" e o resto como tamanho) pra garantir que tudo fique sempre 
alinhado, sem depender de padding do compilador.

*/
#include <sys/kmalloc.h>
#include "sys/interrupt.h"   /* pra m68k_disable_all_interrupts, usado dentro de LOCK() */
#include <sys/critical.h>    /* macros LOCK/UNLOCK -- precisa vir depois de interrupt.h */
#include <string.h>

/*
 * LOCK/UNLOCK (definidas em critical.h): salvam o SR, desligam todas
 * as interrupcoes, e restauram depois. Precisa disso porque o systick
 * pode preemptar bem no meio de um kmalloc/kfree mexendo nos headers
 * da lista -- sem isso a heap corrompe silenciosamente.
 */

/*
 * Header de 4 bytes. Bit mais alto (KM_FREE_BIT) = 1 quando o bloco
 * esta livre. Os outros 31 bits sao o tamanho da area de dados (sem
 * contar o proprio header). Cabe tranquilo: nao tem RAM homebrew
 * nesse projeto que chegue perto de 2GB.
 */
typedef struct {
    uint32_t size_and_flag;
} km_header_t;


#define KM_FREE_BIT     0x80000000UL
#define KM_SIZE(h)      ((h)->size_and_flag & ~KM_FREE_BIT)
#define KM_IS_FREE(h)   (((h)->size_and_flag & KM_FREE_BIT) != 0)
#define KM_SET(h, sz, f) ((h)->size_and_flag = ((sz) & ~KM_FREE_BIT) | ((f) ? KM_FREE_BIT : 0))

#define HDR_SIZE        ((uint32_t) sizeof(km_header_t))
#define KM_ALIGN(x)     (((x) + 3u) & ~3u)
#define MIN_SPLIT       16u   /* nao compensa dividir se sobrar menos que isso de bloco novo */

static uint8_t  *heap_base = NULL;
static uint32_t  heap_size = 0;

/* bytes de DADOS em uso agora (soma dos tamanhos dos blocos ocupados; nao conta os cabecalhos) */
static uint32_t mem_used=0;

uint32_t get_mem_used(){
    return mem_used;
}

uint32_t get_heap_size(){
    return heap_size;
}

uint32_t get_heap_base(){
    return (uint32_t )heap_base;
}

/*
 * Percorre o heap inteiro, bloco por bloco (do mesmo jeito que o kmalloc procura),
 * e soma os blocos livres.
 *   *free_bytes : total de bytes de dados livres (soma de todos os blocos livres)
 *   *largest    : maior bloco livre unico (e' o maior kmalloc que ainda daria certo)
 *   *free_blocks: quantos blocos livres existem (muitos = heap fragmentado)
 * Qualquer ponteiro pode ser NULL.
 */
static uint32_t heap_bad_addr = 0;      /* endereco do 1o cabecalho invalido achado (0 = nenhum) */

/* endereco do cabecalho corrompido achado pela ultima varredura (0 = heap integro) */
uint32_t get_heap_corrupt_addr(void)
{
    return heap_bad_addr;
}

void get_heap_stats(uint32_t *free_bytes, uint32_t *largest, uint32_t *free_blocks)
{
    short saved_sr;
    uint8_t *p, *end;
    uint32_t total = 0, big = 0, nfree = 0;

    if (heap_base != NULL) {
        end = heap_base + heap_size;
        saved_sr = cs_enter();
        heap_bad_addr = 0;
        p = heap_base;
        while (p + HDR_SIZE <= end) {
            km_header_t *h;
            uint32_t block_size;

            if ((uint32_t) p & 1) {                 /* endereco impar: nem le (address error) */
                heap_bad_addr = (uint32_t) p;
                break;
            }
            h = (km_header_t *) p;
            block_size = KM_SIZE(h);
            if ((block_size & 3) || p + HDR_SIZE + block_size > end) {   /* cabecalho corrompido */
                heap_bad_addr = (uint32_t) p;
                break;
            }
            if (KM_IS_FREE(h)) {
                total += block_size;
                nfree++;
                if (block_size > big)
                    big = block_size;
            }
            p += HDR_SIZE + block_size;
        }
        cs_leave(saved_sr);
    }
    if (free_bytes)  *free_bytes  = total;
    if (largest)     *largest     = big;
    if (free_blocks) *free_blocks = nfree;
}

/* total de bytes livres no heap (percorre todos os blocos) */
uint32_t get_heap_free(void)
{
    uint32_t f;
    get_heap_stats(&f, NULL, NULL);
    return f;
}

/* maior bloco livre unico */
uint32_t get_heap_largest_free(void)
{
    uint32_t b;
    get_heap_stats(NULL, &b, NULL);
    return b;
}

void kmalloc_init(void *base, uint32_t size)
{
    heap_base = (uint8_t *) base;
    heap_size = size;

    km_header_t *h = (km_header_t *) heap_base;
    KM_SET(h, size - HDR_SIZE, 1);
}

void *kmalloc(uint32_t size)
{
    short saved_sr;
    uint8_t *p, *end;
    void *result = NULL;

    if (heap_base == NULL || size == 0)
        return NULL;

    size = KM_ALIGN(size);
    end = heap_base + heap_size;

    saved_sr=cs_enter();

    p = heap_base;
    while (p < end) {
        km_header_t *h = (km_header_t *) p;
        uint32_t block_size = KM_SIZE(h);

        if (KM_IS_FREE(h) && block_size >= size) {
            /* sobra espaco suficiente pra virar outro bloco util? divide. */
            if (block_size >= size + HDR_SIZE + MIN_SPLIT) {
                km_header_t *next = (km_header_t *) (p + HDR_SIZE + size);
                KM_SET(next, block_size - size - HDR_SIZE, 1);
                KM_SET(h, size, 0);
            } else {
                KM_SET(h, block_size, 0);   /* usa o bloco inteiro, sem dividir */
            }
            mem_used += KM_SIZE(h);     /* tamanho REAL do bloco (se nao dividiu, e' o bloco inteiro) */
            result = p + HDR_SIZE;
            break;
        }

        p += HDR_SIZE + block_size;
    }

    cs_leave(saved_sr);
    return result;
}

void kfree(void *ptr)
{
    short saved_sr;
    km_header_t *h, *next;
    uint8_t *next_addr;

    if (ptr == NULL || heap_base == NULL)
        return;

    saved_sr=cs_enter();

    h = (km_header_t *) ((uint8_t *) ptr - HDR_SIZE);
    if (KM_IS_FREE(h)) {                /* free duplo: ignora (senao mem_used e a lista se corrompem) */
        cs_leave(saved_sr);
        return;
    }
    mem_used -= KM_SIZE(h);
    KM_SET(h, KM_SIZE(h), 1);

    /* funde com o bloco seguinte se ele tambem estiver livre.
     * (so funde pra frente -- fundir pra tras exigiria varrer a heap
     * inteira do inicio; se isso virar problema de fragmentacao na
     * pratica, da pra trocar por uma lista com ponteiro 'prev'.) */
    next_addr = (uint8_t *) h + HDR_SIZE + KM_SIZE(h);
    if (next_addr < heap_base + heap_size) {
        next = (km_header_t *) next_addr;
        if (KM_IS_FREE(next)) {
            KM_SET(h, KM_SIZE(h) + HDR_SIZE + KM_SIZE(next), 1);
        }
    }

    cs_leave(saved_sr);
}

void *kcalloc(uint32_t nmemb, uint32_t size)
{
    uint32_t total = nmemb * size;   /* projeto homebrew, sem overflow check aqui de proposito -- valores pequenos */
    void *p = kmalloc(total);
    if (p != NULL)
        memset(p, 0, total);
    return p;
}

void *krealloc(void *ptr, uint32_t size)
{
    km_header_t *h;
    uint32_t old_size;
    void *new_ptr;

    if (ptr == NULL)
        return kmalloc(size);

    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    h = (km_header_t *) ((uint8_t *) ptr - HDR_SIZE);
    old_size = KM_SIZE(h);

    if (KM_ALIGN(size) <= old_size)
        return ptr;   /* ja cabe no bloco atual, nao mexe em nada */

    new_ptr = kmalloc(size);
    if (new_ptr == NULL)
        return NULL;   /* ptr original continua valido, igual realloc() de verdade */

    memcpy(new_ptr, ptr, old_size);
    kfree(ptr);
    return new_ptr;
}

/*
 * Imprime na tela do kernel os 'max' primeiros blocos do heap:
 *   endereco do cabecalho, valor bruto do cabecalho, e se esta livre (F) ou ocupado (U).
 * Para no primeiro cabecalho invalido e mostra "CORROMPIDO". Use para achar quem estragou.
 */
extern void kernel_puts(const char *s);
extern void kernel_puthex(uint32_t v);

void heap_dump(uint32_t max)
{
    uint8_t *p, *end;
    uint32_t n = 0;

    if (heap_base == NULL) {
        kernel_puts("heap: nao iniciado\n");
        return;
    }
    end = heap_base + heap_size;
    kernel_puts("heap base="); kernel_puthex((uint32_t) heap_base);
    kernel_puts(" size=");     kernel_puthex(heap_size);
    kernel_puts("\n");

    p = heap_base;
    while (p + HDR_SIZE <= end && n < max) {
        uint32_t raw, block_size;

        if ((uint32_t) p & 1) {
            kernel_puts(" CORROMPIDO: endereco impar "); kernel_puthex((uint32_t) p); kernel_puts("\n");
            return;
        }
        raw = ((km_header_t *) p)->size_and_flag;
        block_size = raw & ~KM_FREE_BIT;
        kernel_puts(" "); kernel_puthex((uint32_t) p);
        kernel_puts(" hdr="); kernel_puthex(raw);
        kernel_puts((raw & KM_FREE_BIT) ? " F " : " U ");
        kernel_puthex(block_size);
        if ((block_size & 3) || p + HDR_SIZE + block_size > end) {
            kernel_puts("  <-- CORROMPIDO\n");
            return;
        }
        kernel_puts("\n");
        p += HDR_SIZE + block_size;
        n++;
    }
    kernel_puts(n >= max ? " ...\n" : " fim do heap OK\n");
}
