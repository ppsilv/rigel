/* sys_ctl.h - ps e kill: tipos e numeros compartilhados entre kernel e apps */
#ifndef SYS_CTL_H
#define SYS_CTL_H

#include <stdint.h>

#define TASK_NAME_MAX   32          /* igual a OS_TASK_NAME_MAX do scheduler.h */
#define PS_MAX          12          /* igual a OS_MAX_TASKS do scheduler.h */

/* estados mostrados pelo ps (mesmos valores do task_state_t do scheduler.h) */
#define PS_READY        1
#define PS_RUNNING      2
#define PS_BLOCKED      3
#define PS_DEAD         4
#define PS_KILLING      5

/* bits de task_info_t.flags */
#define PS_F_KERNEL     0x01        /* task do kernel: o kill recusa */
#define PS_F_SLEEPING   0x02        /* bloqueada por OS_SleepTicks (dormindo) */

/* "sinais": por enquanto so existe matar */
#define SIG_CHECK       0           /* so testa se o pid existe */
#define SIGKILL         9
#define SIGTERM         15          /* tratado igual ao SIGKILL */

/* retornos (negativos, mesmos valores do Linux) */
#define K_EPERM         (-1)
#define K_ESRCH         (-3)
#define K_EINVAL        (-22)

/* o que o ps recebe de volta, uma entrada por task */
typedef struct {
    uint32_t pid;                   /* = tcb->id (o mesmo do SYS_GETPID) */
    uint32_t base;                  /* endereco do slot de 256K (0 se task do kernel) */
    uint32_t ticks;                 /* ticks de 10 ms de CPU */
    uint8_t  state;                 /* PS_* */
    uint8_t  flags;                 /* PS_F_* */
    char     name[TASK_NAME_MAX];
} task_info_t;

/* lado do kernel (chamadas pelo trap0.c) */
int32_t sys_ps(task_info_t *buf, uint32_t max);
int32_t sys_kill(uint32_t pid, uint32_t sig);

/* lado do app: wrappers da librigel (mesmo padrao dos outros wrappers de syscall) */
int rigel_ps(task_info_t *buf, unsigned max);
int rigel_kill(unsigned pid, unsigned sig);

#endif /* SYS_CTL_H */
