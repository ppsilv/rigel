/* ps.c - lista as tasks do Rigel */
#include <stdio.h>
#include <stdint.h>
#include <sys/sys_ctl.h>

static const char *state_name(const task_info_t *t)
{
    switch (t->state) {
    case PS_READY:   return "READY";
    case PS_RUNNING: return "RUN";
    case PS_BLOCKED: return (t->flags & PS_F_SLEEPING) ? "SLEEP" : "BLOCK";
    case PS_DEAD:    return "DEAD";
    case PS_KILLING: return "KILL";
    default:         return "?";
    }
}

int main(void)
{
    task_info_t tab[PS_MAX];
    int n, i;

    n = rigel_ps(tab, PS_MAX);
    if (n < 0) {
        printf("ps: erro %d\n", n);
        return 1;
    }

    printf("  PID ESTADO    TICKS    BASE    NOME\n");
    for (i = 0; i < n; i++) {
        printf("%5lu %-6s %8lu  %06lX  %s%s\n",
               (unsigned long)tab[i].pid,
               state_name(&tab[i]),
               (unsigned long)tab[i].ticks,
               (unsigned long)tab[i].base,
               tab[i].name[0] ? tab[i].name : "-",
               (tab[i].flags & PS_F_KERNEL) ? " [kernel]" : "");
    }
    return 0;
}
