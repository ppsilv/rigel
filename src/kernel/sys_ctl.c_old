/* sys_ctl.c - syscalls ps e kill (lado do kernel)
 *
 * Usa o que o scheduler ja tem: OS_Reap, OS_TaskAlive, OS_TaskExit,
 * vfs_task_exit e Slots_BaseAddr. As funcoes novas do scheduler
 * (OS_TaskAt, OS_TaskKillBegin, OS_TaskKillEnd) estao em scheduler.c.
 */
#include <stdint.h>
#include <string.h>
#include <sys/scheduler.h>
#include <sys/slots.h>
#include <sys/ksyscall.h>
#include <sys/sys_ctl.h>

/* vfs.c: fecha todos os fds da task */
extern void vfs_task_exit(tcb_t *t);

/* o mesmo que o SYS_EXIT do trap0.c faz antes do OS_TaskExit (limpeza por id da task) */
extern int sys_fs_task_exit(int id);
extern int sys_dir_task_exit(int id);
extern int sys_net_task_exit(int id);

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

/* ------------------------------- ps ------------------------------- */
int32_t sys_ps(task_info_t *buf, uint32_t max)
{
    uint32_t n = 0;
    uint16_t sr;
    int i;

    /* 68000: palavra em endereco impar da address error */
    if (buf == NULL || ((uint32_t)(uintptr_t)buf & 1) || max == 0)
        return K_EINVAL;
    if (max > OS_MAX_TASKS)
        max = OS_MAX_TASKS;

    OS_Reap();                      /* recolhe tasks mortas antes de listar */

    sr = irq_save();                /* foto consistente da tabela */
    for (i = 0; i < OS_MAX_TASKS && n < max; i++) {
        tcb_t *t = OS_TaskAt(i);
        task_info_t *o;

        if (t == NULL || t->state == TASK_UNUSED)
            continue;

        o = &buf[n++];
        o->pid   = (uint32_t)t->id;
        o->ticks = t->ticks;
        o->state = (uint8_t)t->state;
        o->flags = 0;
        o->base  = 0;
        if (t->id < OS_SLOT_ID_BASE)
            o->flags |= PS_F_KERNEL;            /* task linkada no kernel */
        else
            o->base = Slots_BaseAddr(t->id - OS_SLOT_ID_BASE);
        if (t->sleeping)
            o->flags |= PS_F_SLEEPING;
        strncpy(o->name, t->name, TASK_NAME_MAX - 1);
        o->name[TASK_NAME_MAX - 1] = '\0';
    }
    irq_restore(sr);

    return (int32_t)n;
}

/* ------------------------------ kill ------------------------------ */
static void close_task_resources(int id)
{
    sys_fs_task_exit(id);
    sys_dir_task_exit(id);
    sys_net_task_exit(id);
}

int32_t sys_kill(uint32_t pid, uint32_t sig)
{
    tcb_t *t;

    if (pid == 0 || pid > 0x7FFFFFFFUL)
        return K_EINVAL;
    if (sig != SIG_CHECK && sig != SIGKILL && sig != SIGTERM)
        return K_EINVAL;

    if (!OS_TaskAlive((int)pid))
        return K_ESRCH;
    if ((int)pid < OS_SLOT_ID_BASE)
        return K_EPERM;                         /* task do kernel */
    if (sig == SIG_CHECK)
        return 0;

    /* matou a si mesma: mesmo caminho do SYS_EXIT, nao volta */
    if (current_task != NULL && current_task->id == (int)pid) {
        close_task_resources((int)pid);
        OS_TaskExit();
        return 0;
    }

    /* outra task: sai do round robin (KILLING), fecha tudo, vira DEAD, OS_Reap libera o slot */
    t = OS_TaskKillBegin((int)pid);
    if (t == NULL)
        return K_ESRCH;                         /* morreu entre o OS_TaskAlive e agora */

    close_task_resources((int)pid);
    vfs_task_exit(t);
    OS_TaskKillEnd(t);
    OS_Reap();
    return 0;
}
