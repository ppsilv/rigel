/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_PROC_H
#define RIGEL_PROC_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_openpty(int fds[2]);
long rigel_spawn(char *path, char **argv);
long rigel_spawn_io(const char *p, char **a, int i, int o, int e);
long rigel_waitpid(long pid);
long rigel_sleep(long ticks);
long rigel_yield(void);
long rigel_setramvector(long stub_addr, long handler_addr);
long rigel_getuptime(void);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_PROC_H */
