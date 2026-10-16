/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_NET_H
#define RIGEL_NET_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_socket(long domain, long type, long protocol);
long rigel_connect(long fd, const void *addr, long len);
long rigel_bind(long fd, const void *addr, long len);
long rigel_listen(long fd, long backlog);
long rigel_accept(long fd, void *addr, long *len);
long rigel_setsockopt(long fd, long level, long opt, const void *val, long len);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_NET_H */
