/*
 * syscalls.c -- ponte entre a newlib e o hardware/kernel do Rigel
 *
 * A newlib chama estas funcoes (com underscore). Aqui fica tudo o que
 * ela precisa do sistema: console, heap e saida.
 * Arquivos de verdade (_open/_read em fd >= 3) ficam para o VFS depois.
 */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stddef.h>
#include "duarts.h"

#undef errno
extern int errno;

const char _heap_limit = 0x100000-(32*1024);     /* fim do heap, onde comeca a reserva da pilha */
const char _end= _heap_limit - (256*1024);            /* inicio do heap (rigel.ld) */

static char *heap_ptr = 0;

void *sbrk(ptrdiff_t incr)
{
    char *prev;

    if (heap_ptr == 0)
        heap_ptr = &_end;

    if (heap_ptr + incr > &_heap_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }
    prev = heap_ptr;
    heap_ptr += incr;
    return prev;
}

int write(int fd, const char *buf, int len)
{
    if (fd != 1 && fd != 2) {       /* so stdout/stderr por enquanto */
        errno = EBADF;
        return -1;
    }
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n')
            duart_putc('\r');       /* terminal espera CR+LF */
        duart_putc(buf[i]);
    }
    return len;
}

int read(int fd, char *buf, int len)
{
    int n = 0;

    if (fd != 0) {
        errno = EBADF;
        return -1;
    }
    if (len <= 0)
        return 0;

    /* linha simples com eco; retorna ao receber ENTER */
    while (n < len) {
        char c = (char)duart_getc();
        if (c == '\r')
            c = '\n';
        if (c == 0x7f || c == '\b') {       /* backspace */
            if (n > 0) {
                n--;
                write(1, "\b \b", 3);
            }
            continue;
        }
        write(1, &c, 1);
        buf[n++] = c;
        if (c == '\n')
            break;
    }
    return n;
}

int close(int fd)               { (void)fd; errno = EBADF; return -1; }
int lseek(int fd, int off, int whence) { (void)fd; (void)off; (void)whence; return 0; }
int isatty(int fd)              { return fd >= 0 && fd <= 2; }

int fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int open(const char *path, int flags, int mode)
{
    (void)path; (void)flags; (void)mode;
    errno = ENOENT;                 /* VFS ainda nao existe */
    return -1;
}

int getpid(void)                { return 1; }

int kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void)status;
    for (;;)
        __asm__ volatile ("stop #0x2700");
}
