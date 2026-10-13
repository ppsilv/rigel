/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_DUART_H
#define RIGEL_DUART_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_duarta_cfg(long baud, long params);
long rigel_duarta_read(void);
long rigel_duarta_write(long value);
long rigel_duartb_cfg(long baud, long params);
long rigel_duartb_read(void);
long rigel_duartb_write(long value);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_DUART_H */
