#ifndef PTY_H
#define PTY_H
#include <sys/vfs.h>

/* Cria o par. master = ponta do telnetd, slave = ponta do shell (stdin/stdout/stderr dele).
 * Os File devolvidos ainda nao estao em nenhuma tabela de descritores. 0 = ok, -1 = sem memoria. */
int pty_create(File **master, File **slave);

/* SYS_OPENPTY: cria o par e coloca na tabela da task atual. fds[0] = master, fds[1] = slave. 0 ou -1. */
int pty_open_fds(int fds[2]);

#endif
