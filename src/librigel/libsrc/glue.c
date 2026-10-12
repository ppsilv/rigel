/* glue.c -- o que a newlib pede e que nao precisa do kernel. */
#include <errno.h>
#include <sys/stat.h>

int isatty(int fd){
    return fd >= 0 && fd <= 2;          /* 0,1,2 = console */
}

int fstat(int fd, struct stat *st){
    st->st_mode = (fd >= 0 && fd <= 2) ? S_IFCHR : S_IFREG;
    return 0;
}

int kill(int pid, int sig){
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

/* heap do app: cresce a partir de _end e para 1 KB antes da pilha da propria task */
extern char _end;

void *sbrk(int incr){
    static char *heap = 0;
    char *sp, *prev;

    if (!heap)
        heap = &_end;

    __asm__ volatile ("move.l %%sp,%0" : "=r"(sp));
    if (heap + incr > sp - 1024) {
        errno = ENOMEM;
        return (void *)-1;
    }
    prev = heap;
    heap += incr;
    return prev;
}
