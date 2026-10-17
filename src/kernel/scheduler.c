#include "sys/scheduler.h"
#include "sys/slots.h"
#include <string.h>

/*
 * Defina OS_CPU_68010 na linha de compilacao (-DOS_CPU_68010) se for
 * linkar com ctx_switch_68010.s. Se nao definir, assume 68000 puro e
 * este arquivo deve ser linkado com ctx_switch_68000.s.
 */

tcb_t *current_task = NULL;

static tcb_t task_pool[OS_MAX_TASKS];   /* armazenamento fixo dos TCBs -- sem malloc */
extern void kernel_puts(const char *s) ;
extern uint32_t systemTick;             /* +1 a cada 10 ms (OS_TickISR) */

/*
 * ready_head: cabeca da fila circular de tarefas PRONTAS (TASK_READY
 * ou TASK_RUNNING). So essas tarefas estao encadeadas via 'next'.
 * Uma tarefa TASK_BLOCKED e' removida daqui e nao participa do
 * round robin ate ser desbloqueada.
 */
static tcb_t *ready_head = NULL;

extern void OS_StartHighRdy(void); /* implementada em ctx_switch_68000.s OU ctx_switch_68010.s */

/* ------------------------------------------------------------------
 * Secao critica: mascara todas as interrupcoes enquanto mexe nas
 * estruturas do scheduler, para o tick nao pegar a fila no meio.
 * (No PC, usado so' para testes, vira no-op.)
 * ------------------------------------------------------------------ */
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

/* procura um TCB livre no pool (reaproveita os das tasks que terminaram) */
static tcb_t *tcb_alloc(void)
{
    for (int i = 0; i < OS_MAX_TASKS; i++)
        if (task_pool[i].state == TASK_UNUSED)
            return &task_pool[i];
    return NULL;
}

/* insere 't' no fim da fila circular de prontas */
static void ready_insert(tcb_t *t)
{
    if (ready_head == NULL) {
        ready_head = t;
        t->next    = t;             /* fila de 1 elemento aponta pra si mesma */
        return;
    }
    tcb_t *tail = ready_head;
    while (tail->next != ready_head)
        tail = tail->next;
    tail->next = t;
    t->next    = ready_head;
}

/* remove 't' da fila circular de prontas, se ela estiver la */
static void ready_remove(tcb_t *t)
{
    if (ready_head == NULL)
        return;

    if (t->next == t) {
        /* 't' e' a unica tarefa da fila */
        if (ready_head == t)
            ready_head = NULL;
        t->next = NULL;
        return;
    }

    tcb_t *prev = ready_head;
    while (prev->next != t) {
        prev = prev->next;
        if (prev == ready_head) /* deu a volta inteira e nao achou -- 't' nao esta na fila */
            return;
    }

    prev->next = t->next;
    if (ready_head == t)
        ready_head = t->next;
    t->next = NULL;
}

void OS_Init(void)
{
    memset(task_pool, 0, sizeof(task_pool));   /* state 0 == TASK_UNUSED */
    current_task = NULL;
    ready_head   = NULL;
}

void *OS_TaskArg(void)
{
    return current_task ? current_task->arg : NULL;
}

/* escreve 4 bytes big-endian e devolve o novo topo da pilha (que cresce pra baixo) */
static uint8_t *push32(uint8_t *sp, uint32_t val)
{
    sp -= 4;
    sp[0] = (uint8_t)(val >> 24);
    sp[1] = (uint8_t)(val >> 16);
    sp[2] = (uint8_t)(val >> 8);
    sp[3] = (uint8_t)(val);
    return sp;
}

static uint8_t *push16(uint8_t *sp, uint16_t val)
{
    sp -= 2;
    sp[0] = (uint8_t)(val >> 8);
    sp[1] = (uint8_t)(val);
    return sp;
}

/*
 * Monta a pilha inicial da tarefa como se ela ja tivesse sido
 * interrompida uma vez pelo OS_TickISR -- o mesmo codigo de
 * restauracao (movem.l (a7)+, d0-d7/a0-a6 ; rte) funciona igual pra
 * tarefa nova e pra tarefa retomada.
 *
 *   68000 (OS_CPU_68010 NAO definido):
 *     [ D0..D7, A0..A6 ]   60 bytes
 *     [ SR ]                2 bytes
 *     [ PC ]                4 bytes
 *
 *   68010 (OS_CPU_68010 definido):
 *     [ D0..D7, A0..A6 ]   60 bytes
 *     [ SR ]                2 bytes
 *     [ PC ]                4 bytes
 *     [ Format/Vector ]     2 bytes  (0x0000 = formato 0, frame curto)
 */
tcb_t *OS_TaskCreate(void (*entry)(void), void *arg, uint8_t *stack, uint32_t stack_size, int id)
{
    if (entry == NULL || stack == NULL)
        return NULL;

    uint16_t sr = irq_save();

    tcb_t *tcb = tcb_alloc();
    if (tcb == NULL) {
        irq_restore(sr);
        return NULL;
    }

    tcb->stack_base = stack;
    tcb->stack_size = stack_size;
    tcb->arg        = arg;
    tcb->id         = id;
    tcb->next       = NULL;
    tcb->sleeping   = 0;
    tcb->wake_tick  = 0;

    uint8_t *sp = stack + stack_size;
    sp = (uint8_t *)((uintptr_t)sp & ~(uintptr_t)1);   /* pilha alinhada em word, exigencia do 68k */

#ifdef OS_CPU_68010
    sp = push16(sp, 0x0000);                /* format/vector: formato 0 -- so o 68010 exige isso */
#endif
    sp = push32(sp, (uint32_t)(uintptr_t)entry);       /* PC = ponto de entrada da tarefa */
    sp = push16(sp, 0x2000);                /* SR: modo supervisor, mascara de interrupcao 0 (todas habilitadas) */

    for (int i = 14; i >= 0; i--)
        sp = push32(sp, 0);                 /* D0-D7, A0-A6 comecam zerados */

    tcb->sp    = (uint32_t *)sp;
    tcb->state = TASK_READY;
    ready_insert(tcb);

    irq_restore(sr);
    return tcb;
}

/*
 * A tarefa termina: sai do round robin e fica TASK_DEAD. NAO libera o
 * slot aqui, porque ainda esta rodando na pilha que fica dentro dele.
 * Quem libera e' OS_Reap(), chamada de fora (pelo loader, antes de
 * alocar um slot novo).
 */
void OS_TaskExit(void)
{
    uint16_t sr = irq_save();
    if (current_task) {
        ready_remove(current_task);       /* sai do round robin de vez */
        current_task->state = TASK_DEAD;
    }
    irq_restore(sr);
#ifndef OS_HOST_TEST
    for (;;)
        OS_Yield();   /* sai da CPU na hora; esta tarefa nunca mais volta (nao esta' na fila) */
#endif
}

/*
 * Recolhe as tarefas mortas: libera o slot de memoria (so' das tarefas
 * criadas pelo loader, que usam id = OS_SLOT_ID_BASE + slot) e devolve
 * o TCB ao pool. NUNCA chame de dentro de uma tarefa que ja terminou.
 */
void OS_Reap(void)
{
    uint16_t sr = irq_save();
    for (int i = 0; i < OS_MAX_TASKS; i++) {
        tcb_t *t = &task_pool[i];
        if (t->state != TASK_DEAD)
            continue;
        if (t == current_task)
            continue;                     /* ainda esta' na CPU; fica pro proximo reap */
        if (t->id >= OS_SLOT_ID_BASE)
            Slots_Free(t->id - OS_SLOT_ID_BASE);
        t->sleeping = 0;
        t->state = TASK_UNUSED;
    }
    irq_restore(sr);
}

/* 1 se a task 'id' ainda existe (pronta, rodando ou bloqueada); 0 se terminou
 * (DEAD) ou ja foi recolhida. Usada pelo SYS_WAITPID. So le, nao precisa mascarar IRQ. */
int OS_TaskAlive(int id)
{
    for (int i = 0; i < OS_MAX_TASKS; i++) {
        tcb_t *t = &task_pool[i];
        if (t->id == id && t->state != TASK_UNUSED && t->state != TASK_DEAD)
            return 1;
    }
    return 0;
}

/* acorda as tasks que dormem por OS_SleepTicks e ja cumpriram o tempo (chamada com IRQ mascarada) */
static void wake_sleepers(void)
{
    uint32_t now = *(volatile uint32_t *)&systemTick;

    for (int i = 0; i < OS_MAX_TASKS; i++) {
        tcb_t *t = &task_pool[i];
        if (t->state == TASK_BLOCKED && t->sleeping &&
            (int32_t)(now - t->wake_tick) >= 0) {       /* conta certo mesmo na volta do contador */
            t->sleeping = 0;
            t->state    = TASK_READY;
            ready_insert(t);
        }
    }
}

void OS_Schedule(void)
{
    wake_sleepers();

    if (ready_head == NULL)
        return;   /* ninguem pronto: a task atual (se estiver dormindo) continua girando no laco do
                     OS_SleepTicks ate o tick acordar alguem; nao trava */

    if (current_task && current_task->state == TASK_RUNNING)
        current_task->state = TASK_READY;

    current_task = ready_head;
    ready_head   = ready_head->next;   /* proxima chamada comeca dali -- efetiva o round robin */
    current_task->state = TASK_RUNNING;
}

void OS_Start(void)
{
    if (ready_head == NULL)
        return;   /* nenhuma tarefa criada ainda -- nada pra rodar */

    current_task = ready_head;
    ready_head   = ready_head->next;
    current_task->state = TASK_RUNNING;

    OS_StartHighRdy();   /* nao retorna */
}

void OS_TaskBlock(tcb_t *task)
{
    if (task == NULL)
        return;

    uint16_t sr = irq_save();
    if (task->state != TASK_READY && task->state != TASK_RUNNING) {
        irq_restore(sr);
        return;   /* ja bloqueada ou nao existe -- nao faz nada */
    }

    ready_remove(task);
    task->state = TASK_BLOCKED;
    irq_restore(sr);

    /*
     * Se 'task' e' a que esta rodando agora (bloqueio voluntario), ela
     * so vai realmente sair da CPU no proximo tick -- ver aviso no
     * scheduler.h sobre a falta de um OS_Yield/trap pra reschedule
     * imediato.
     */
}

void OS_TaskWake(tcb_t *task)
{
    if (task == NULL)
        return;

    uint16_t sr = irq_save();
    if (task->state == TASK_BLOCKED) {
        task->sleeping = 0;                /* acorda tambem quem dormia por tempo */
        task->state = TASK_READY;
        ready_insert(task);
    }
    irq_restore(sr);
}

/*
 * A task atual sai da fila por 'ticks' ticks. O OS_Yield troca de contexto na hora (com a IRQ
 * ainda mascarada, entao nao ha janela entre marcar "dormindo" e sair da CPU). Quando a task
 * volta a rodar o escalonador ja a acordou (state RUNNING). Se NINGUEM mais estiver pronto, o
 * OS_Schedule nao troca de task e o OS_Yield volta aqui com a task ainda BLOCKED: ela gira
 * chamando OS_Yield (agora com IRQ liberada) ate o tick cumprir o tempo e acordar ela mesma.
 */
void OS_SleepTicks(uint32_t ticks)
{
    tcb_t *me = current_task;
    uint16_t sr;

    if (me == NULL || ticks == 0) {
        OS_Yield();
        return;
    }

    sr = irq_save();
    me->wake_tick = *(volatile uint32_t *)&systemTick + ticks;
    me->sleeping  = 1;
    ready_remove(me);
    me->state = TASK_BLOCKED;
    OS_Yield();                            /* IRQ mascarada: troca atomica */
    irq_restore(sr);

    while (me->state == TASK_BLOCKED)      /* so' acontece se nao havia mais ninguem pronto */
        OS_Yield();
}
