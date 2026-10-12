/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_PROC_H
#define RIGEL_PROC_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_spawn(char *path);
long rigel_sleep(long ticks);
long rigel_yield(void);
long rigel_setramvector(long stub_addr, long handler_addr);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_PROC_H */
