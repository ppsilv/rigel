#ifndef RIGEL_PTY_H
#define RIGEL_PTY_H
/*
 * Lado do app dos dois syscalls novos. Junte ao rigel.h e implemente no librigel
 * do mesmo jeito que o rigel_spawn (mesma convencao do TRAP #0).
 *
 *   long rigel_openpty(int fds[2]);
 *       cria um pty. fds[0] = master (o telnetd le a tela e escreve as teclas),
 *       fds[1] = slave (vai para o stdin/stdout/stderr do shell). 0 ou -1.
 *
 *   long rigel_spawn_io(const char *path, char **argv, int fd_in, int fd_out, int fd_err);
 *       como rigel_spawn, mas o filho recebe fd_in/fd_out/fd_err como stdin/stdout/stderr.
 *       Devolve o pid ou -1.
 *
 * Numeros (tem que bater com kernel/sys/syscall_pty.h):
 */
#define SYS_OPENPTY   0x70
#define SYS_SPAWN_IO  0x71

long rigel_openpty(int fds[2]);
long rigel_spawn_io(const char *path, char **argv, int fd_in, int fd_out, int fd_err);

#endif
