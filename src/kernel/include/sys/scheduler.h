#ifndef OS_SCHED_H
#define OS_SCHED_H

#include <stdint.h>

#define OS_MAX_TASKS   12   /* era 8: uma sessao telnet usa 2 tasks (telnetd + shell) */
#define OS_MAX_FDS     32   /* descritores por task (a tabela fica dentro do tcb) */
#define OS_SLOT_ID_BASE  1000
#define OS_TASK_NAME_MAX 32   /* 31 caracteres + NUL (mostrado pelo ps) */

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD,         /* <- NOVO */
    TASK_KILLING       /* kill em andamento: ja saiu da fila, ainda fechando fds (ninguem reaproveita o TCB) */
} task_state_t;

/*
 * IMPORTANTE: 'sp' TEM que ser o primeiro campo da struct.
 * O assembly de context switch acessa o stack pointer salvo
 * fazendo move.l (a0), a7 -- ou seja, ele assume offset 0.
 * Se voce reordenar os campos, o assembly quebra silenciosamente.
 */
struct File;

typedef struct tcb {
    uint32_t     *sp;          /* stack pointer salvo desta tarefa */
    task_state_t  state;
    struct tcb   *next;        /* proxima tarefa na fila circular de PRONTAS (NULL se nao estiver na fila) */
    void         *arg;         /* argumento da tarefa, lido via OS_TaskArg() */
    uint8_t      *stack_base;
    uint32_t      stack_size;
    int           id;
    uint32_t      wake_tick;   /* se 'sleeping': acorda quando systemTick chegar aqui */
    uint8_t       sleeping;    /* 1 = bloqueada por OS_SleepTicks (o tick a acorda sozinho) */
    struct File  *fds[OS_MAX_FDS]; /* descritores da task (vfs.c); 0..2 herdados do pai. Fica DEPOIS de 'sp' (offset 0) */
    uint32_t      ticks;       /* ticks de CPU usados (OS_AccountTick, chamada pelo OS_TickISR) -- para o ps */
    char          name[OS_TASK_NAME_MAX]; /* nome mostrado pelo ps (OS_TaskSetName) */
} tcb_t;

/* current_task precisa ser global e visivel pro assembly (ctx_switch_*.s usa .extern current_task) */
extern tcb_t *current_task;

void   OS_Init(void);

/*
 * Cria uma tarefa. 'entry' NAO recebe argumentos (void entry(void)) --
 * de proposito, pra nao depender de detalhes da ABI de passagem de
 * parametro do seu compilador. Dentro da tarefa, leia o argumento com
 * OS_TaskArg(). A tarefa ja entra na fila de prontas.
 */
tcb_t *OS_TaskCreate(void (*entry)(void), void *arg,
                      uint8_t *stack, uint32_t stack_size, int id);
tcb_t *OS_TaskCreatePIC(void (*entry)(void), void *arg, uint8_t *stack,
                         uint32_t stack_size, int id, uint32_t a5_value);
void   OS_Start(void);        /* nunca retorna -- entra na primeira tarefa pronta */
void   OS_Schedule(void);     /* chamado pelo OS_TickISR (asm) a cada tick de timer */
void   OS_TaskExit(void);     /* para onde uma tarefa cai se a funcao dela retornar */
void  *OS_TaskArg(void);      /* le o 'arg' da tarefa atualmente rodando */
void   OS_Reap(void);

/* ---- usadas pelo ps e kill (sys_ctl.c) ---- */
void   OS_TaskSetName(tcb_t *t, const char *name);   /* copia o nome (corta em 31 caracteres) */
void   OS_AccountTick(void);                         /* chamada pelo OS_TickISR (asm): current_task->ticks++ */
tcb_t *OS_TaskAt(int index);                         /* TCB do pool (0..OS_MAX_TASKS-1); so' leitura */
tcb_t *OS_TaskKillBegin(int id);                     /* tira a task do round robin; NULL se nao existe/e' a atual */
void   OS_TaskKillEnd(tcb_t *t);                     /* depois de fechar os fds: marca DEAD (o OS_Reap libera o slot) */
int    OS_TaskAlive(int id);

/*
 * OS_TaskBlock: tira 'task' da fila de prontas. Ela para de receber
 * tempo de CPU ate alguem chamar OS_TaskWake pra ela. Pensada pra ser
 * chamada por um driver (serial, ATA/FatFS) quando uma tarefa fica
 * esperando um evento externo -- assim o scheduler nem perde tempo
 * escalando ela enquanto ela nao tem nada pra fazer.
 *
 * Bloqueio voluntario (a task bloqueia a si mesma): logo depois de OS_TaskBlock chame OS_Yield()
 * para sair da CPU na hora; sem isso ela so' cede no proximo tick. Para "dormir um tempo" use
 * OS_SleepTicks, que ja faz tudo (bloqueia, cede e e' acordada pelo tick).
 */
void   OS_TaskBlock(tcb_t *task);

/* OS_TaskWake: devolve 'task' pra fila de prontas (ex: driver recebeu o dado esperado). */
void   OS_TaskWake(tcb_t *task);

/*
 * OS_Yield: a task atual larga o resto da fatia AGORA (troca de contexto na hora, sem esperar o
 * tick) e volta quando o round robin chegar nela de novo. Implementada em ctx_switch_68000.S.
 * So' chame de dentro de uma task (modo supervisor), nunca de dentro do proprio OS_TickISR.
 */
void   OS_Yield(void);

/*
 * OS_SleepTicks(n): a task atual sai do round robin por 'n' ticks (10 ms cada) e e' acordada
 * pelo escalonador quando o tempo acabar. Enquanto dorme nao gasta CPU. n = 0 equivale a OS_Yield.
 * "Dormir 1 tick" = esperar o proximo tick (substitui o laco "while (systemTick == t);").
 */
void   OS_SleepTicks(uint32_t ticks);

#endif
