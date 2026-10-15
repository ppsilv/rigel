/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_RTC_H
#define RIGEL_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_readtime(void *tm_out);
long rigel_writetime(void *tm_in);
long rigel_readrtc(void);
long rigel_writertc(long ts);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_RTC_H */
