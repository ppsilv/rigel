#ifndef SYS_SYSCALL_PTY_H
#define SYS_SYSCALL_PTY_H
/* Numeros dos syscalls novos. CONFIRA se nao colidem com sys/syscall.h; se colidirem, troque aqui
 * (o mesmo valor tem que estar no lado do app: app/rigel_pty.h). Ou mova estas duas linhas
 * para o seu sys/syscall.h. */
#ifndef SYS_OPENPTY
#define SYS_OPENPTY   0x70
#endif
#ifndef SYS_SPAWN_IO
#define SYS_SPAWN_IO  0x71
#endif
#endif
