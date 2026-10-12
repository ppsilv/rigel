/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_NET_H
#define RIGEL_NET_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_connect(long sockfd, void *addr, long addrlen);
long rigel_bind(long sockfd, void *addr, long addrlen);
long rigel_listen(long sockfd, long backlog);
long rigel_accept(long sockfd, void *addr, long *addrlen);
long rigel_setsockopt(long sockfd, long level, long optname, void *optval, long optlen);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_NET_H */
