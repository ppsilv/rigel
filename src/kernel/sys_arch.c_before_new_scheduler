/*
 * sys_arch.c -- porta do lwIP (NO_SYS = 0) para o kernel Rigel.
 *
 * Como o kernel e' round robin com tick de 10 ms e ainda nao tem bloqueio
 * "de verdade" (OS_Yield), toda espera aqui e' por polling: a task confere
 * a condicao e, se nao deu, espera o proximo tick (quando o escalonador
 * troca de task). Isso e' o mesmo estilo do sys_sleep/waitpid do kernel.
 *
 * Todas as estruturas sao protegidas mascarando interrupcoes por instantes.
 */
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "arch/sys_arch.h"

#if !NO_SYS

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/kmalloc.h>
#include <sys/scheduler.h>

#define MS_PER_TICK       10
#define LWIP_THREAD_ID0   2000      /* ids das threads do lwIP: 2000, 2001, ... (tasks de usuario sao 1000+slot) */

extern uint32_t systemTick;         /* +1 a cada 10 ms */

/* ---------------- secao critica ---------------- */
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

static inline uint32_t now_tick(void)
{
    return *(volatile uint32_t *)&systemTick;
}

/* larga o resto da fatia: espera o proximo tick */
static void wait_tick(void)
{
    uint32_t t = now_tick();
    while (now_tick() == t)
        ;
}

/* ================================================================
 * 0. errno (o lwIP escreve nele; global do kernel) e memoria (kmalloc com IRQ mascarada)
 * ================================================================ */
#undef errno
int errno;

void *rigel_malloc(size_t n)
{
    uint16_t sr = irq_save();
    void *p = kmalloc(n);
    irq_restore(sr);
    return p;
}

void rigel_free(void *p)
{
    uint16_t sr = irq_save();
    kfree(p);
    irq_restore(sr);
}

#if RIGEL_LWIP_OWN_HEAP
/* area do heap proprio do lwIP (chamada uma vez, por mem_init); vem do kmalloc */
void *rigel_lwip_heap(void)
{
    static void *h;
    if (h == NULL) {
        h = kmalloc((size_t)MEM_SIZE + 64);
        if (h == NULL) {
            extern void kernel_puts(const char *s);
            kernel_puts("lwIP: sem memoria para o heap\r\n");
            for (;;) ;
        }
    }
    return h;
}
#endif

void *rigel_calloc(size_t count, size_t size)
{
    size_t n = count * size;
    void *p = rigel_malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

/* ================================================================
 * 0b. sys_*_valid / sys_*_set_invalid como FUNCOES reais.
 * Se a lib do lwIP foi compilada com um sys_arch.h sem as macros, ela chama estas funcoes; se foi
 * compilada com as macros, estas ficam sem uso (inofensivo). O nome entre parenteses impede que a
 * macro do sys_arch.h expanda aqui.
 * ================================================================ */
int (sys_sem_valid)(sys_sem_t *s)          { return s != NULL && *s != NULL; }
void (sys_sem_set_invalid)(sys_sem_t *s)   { if (s != NULL) *s = NULL; }
int (sys_mutex_valid)(sys_mutex_t *m)      { return m != NULL && *m != NULL; }
void (sys_mutex_set_invalid)(sys_mutex_t *m) { if (m != NULL) *m = NULL; }
int (sys_mbox_valid)(sys_mbox_t *m)        { return m != NULL && *m != NULL; }
void (sys_mbox_set_invalid)(sys_mbox_t *m) { if (m != NULL) *m = NULL; }

/* ================================================================
 * 1. Tempo
 * ================================================================ */
u32_t sys_now(void)
{
    return (u32_t)(now_tick() * MS_PER_TICK);
}

/* ================================================================
 * 2. Protecao leve (so' usada se SYS_LIGHTWEIGHT_PROT = 1)
 * ================================================================ */
#if SYS_LIGHTWEIGHT_PROT
sys_prot_t sys_arch_protect(void)
{
    return (sys_prot_t)irq_save();
}

void sys_arch_unprotect(sys_prot_t pval)
{
    irq_restore((uint16_t)pval);
}
#endif

/* ================================================================
 * 3. Semaforos (e mutexes, que sao semaforos binarios)
 * ================================================================ */
struct rigel_sem {
    volatile uint32_t count;
};

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    struct rigel_sem *s = (struct rigel_sem *)kmalloc(sizeof(*s));
    if (s == NULL) {
        *sem = SYS_SEM_NULL;
        return ERR_MEM;
    }
    s->count = count;
    *sem = s;
    return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem)
{
    if (sem && *sem) {
        kfree(*sem);
        *sem = SYS_SEM_NULL;
    }
}

void sys_sem_signal(sys_sem_t *sem)
{
    uint16_t sr = irq_save();
    (*sem)->count++;
    irq_restore(sr);
}

/* tenta pegar; 1 = pegou */
static int sem_try(struct rigel_sem *s)
{
    int ok = 0;
    uint16_t sr = irq_save();
    if (s->count > 0) {
        s->count--;
        ok = 1;
    }
    irq_restore(sr);
    return ok;
}

/* timeout em ms; 0 = infinito. Retorna ms esperados ou SYS_ARCH_TIMEOUT */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    struct rigel_sem *s = *sem;
    uint32_t start = now_tick();
    uint32_t limit = (timeout + MS_PER_TICK - 1) / MS_PER_TICK;

    for (;;) {
        if (sem_try(s))
            return (u32_t)((now_tick() - start) * MS_PER_TICK);
        if (timeout != 0 && (uint32_t)(now_tick() - start) >= limit)
            return SYS_ARCH_TIMEOUT;
        wait_tick();
    }
}

/* ================================================================
 * 4. Mutexes
 * ================================================================ */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    return sys_sem_new((sys_sem_t *)mutex, 1);
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    sys_sem_free((sys_sem_t *)mutex);
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    (void)sys_arch_sem_wait((sys_sem_t *)mutex, 0);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    sys_sem_signal((sys_sem_t *)mutex);
}

/* ================================================================
 * 5. Mailboxes (fila circular de ponteiros)
 * ================================================================ */
struct rigel_mbox {
    void  **slot;
    int     size;
    volatile int head;     /* proxima leitura */
    volatile int count;    /* itens na fila  */
};

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    struct rigel_mbox *m;

    if (size <= 0)
        size = 1;
    m = (struct rigel_mbox *)kmalloc(sizeof(*m));
    if (m == NULL) {
        *mbox = SYS_MBOX_NULL;
        return ERR_MEM;
    }
    m->slot = (void **)kmalloc((size_t)size * sizeof(void *));
    if (m->slot == NULL) {
        kfree(m);
        *mbox = SYS_MBOX_NULL;
        return ERR_MEM;
    }
    m->size  = size;
    m->head  = 0;
    m->count = 0;
    *mbox = m;
    return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    if (mbox && *mbox) {
        kfree((*mbox)->slot);
        kfree(*mbox);
        *mbox = SYS_MBOX_NULL;
    }
}

/* tenta colocar; ERR_OK ou ERR_MEM se cheia */
static err_t mbox_put(struct rigel_mbox *m, void *msg)
{
    err_t r = ERR_MEM;
    uint16_t sr = irq_save();
    if (m->count < m->size) {
        m->slot[(m->head + m->count) % m->size] = msg;
        m->count++;
        r = ERR_OK;
    }
    irq_restore(sr);
    return r;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    while (mbox_put(*mbox, msg) != ERR_OK)
        wait_tick();                    /* cheia: espera abrir espaco */
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    return mbox_put(*mbox, msg);
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    return mbox_put(*mbox, msg);
}

/* 1 = tirou um item */
static int mbox_get(struct rigel_mbox *m, void **msg)
{
    int ok = 0;
    uint16_t sr = irq_save();
    if (m->count > 0) {
        void *v = m->slot[m->head];
        m->head = (m->head + 1) % m->size;
        m->count--;
        if (msg)
            *msg = v;
        ok = 1;
    }
    irq_restore(sr);
    return ok;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    struct rigel_mbox *m = *mbox;
    uint32_t start = now_tick();
    uint32_t limit = (timeout + MS_PER_TICK - 1) / MS_PER_TICK;

    for (;;) {
        if (mbox_get(m, msg))
            return (u32_t)((now_tick() - start) * MS_PER_TICK);
        if (timeout != 0 && (uint32_t)(now_tick() - start) >= limit)
            return SYS_ARCH_TIMEOUT;
        wait_tick();
    }
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    return mbox_get(*mbox, msg) ? 0 : SYS_MBOX_EMPTY;
}

/* ================================================================
 * 6. Init e threads
 * ================================================================ */
void sys_init(void)
{
    /* nada a inicializar */
}

/* ---- controle de uso de pilha: a pilha e' preenchida com 0xA5; o que continuar 0xA5 nunca foi usado ---- */
#define MAX_TRACK  6
#define STACK_FILL 0xA5
static struct { const char *name; uint8_t *base; int size; } trk[MAX_TRACK];
static int ntrk;

extern void kernel_puts(const char *s);

static void put_num(int v)
{
    char b[12];
    int i = 11;
    b[i] = 0;
    if (v <= 0) { kernel_puts("0"); return; }
    while (v > 0 && i > 0) { b[--i] = (char)('0' + v % 10); v /= 10; }
    kernel_puts(&b[i]);
}

/* all = 0: so' imprime as threads com menos de 25% de pilha livre; all = 1: imprime todas */
void rigel_stack_report(int all)
{
    int i;
    for (i = 0; i < ntrk; i++) {
        int freeb = 0;
        while (freeb < trk[i].size && trk[i].base[freeb] == STACK_FILL) freeb++;
        if (all || freeb < trk[i].size / 4) {
            kernel_puts(all ? "    stack " : "    STACK BAIXA: ");
            kernel_puts(trk[i].name);
            kernel_puts(": livre ");
            put_num(freeb);
            kernel_puts(" de ");
            put_num(trk[i].size);
            kernel_puts(" bytes\n");
        }
    }
}

struct thread_start {
    void (*fn)(void *);
    void     *arg;
};

/* ponto de entrada comum: le fn/arg do TCB, roda, e encerra a task se a funcao retornar */
static void thread_tramp(void)
{
    struct thread_start *ts = (struct thread_start *)current_task->arg;
    void (*fn)(void *) = ts->fn;
    void     *arg = ts->arg;

    kfree(ts);
    fn(arg);
    OS_TaskExit();
    for (;;)
        ;
}

sys_thread_t sys_thread_new(const char *name, void (*function)(void *), void *arg,
                            int stacksize, int prio)
{
    static int next_id = LWIP_THREAD_ID0;
    struct thread_start *ts;
    uint8_t *stack;
    int id;
    (void)prio;                          /* round robin: sem prioridade */

    if (stacksize < 2048)
        stacksize = 2048;

    ts    = (struct thread_start *)kmalloc(sizeof(*ts));
    stack = (uint8_t *)kmalloc((size_t)stacksize);
    if (ts == NULL || stack == NULL) {
        if (ts)    kfree(ts);
        if (stack) kfree(stack);
        return -1;
    }
    ts->fn  = function;
    ts->arg = arg;
    memset(stack, STACK_FILL, (size_t)stacksize);
    if (ntrk < MAX_TRACK) {
        trk[ntrk].name = name ? name : "?";
        trk[ntrk].base = stack;
        trk[ntrk].size = stacksize;
        ntrk++;
    }

    id = next_id++;
    if (OS_TaskCreate(thread_tramp, ts, stack, (uint32_t)stacksize, id) == NULL) {
        kfree(ts);
        kfree(stack);
        return -1;
    }
    return (sys_thread_t)id;
}

#endif /* !NO_SYS */
