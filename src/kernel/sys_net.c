/*
 * sys_net.c -- syscalls de socket TCP do Rigel, em cima da API de sockets do lwIP.
 *
 *   long sys_socket(long domain, long type, long protocol)          fd  ou -1   (AF_INET, SOCK_STREAM)
 *   long sys_bind(long fd, const rigel_sockaddr_in *a, long len)    0   ou -1
 *   long sys_listen(long fd, long backlog)                          0   ou -1
 *   long sys_accept(long fd, rigel_sockaddr_in *a, long *len)       fd2 ou -1
 *   long sys_connect(long fd, const rigel_sockaddr_in *a, long len) 0   ou -1
 *   long sys_setsockopt(long fd, long level, long opt, const void *val, long len)  0 ou -1
 *   void sys_net_task_exit(int owner)       (fecha os sockets que a task deixou abertos)
 *
 * O socket e' um fd do VFS: read/write/close normais funcionam nele.
 *   read  : espera chegar dado; 0 = o outro lado fechou (EOF); -1 = erro ou tempo esgotado (RCVTIMEO)
 *   write : devolve quantos bytes foram enviados (ou -1)
 */
#include <stdint.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/kmalloc.h>
#include <sys/scheduler.h>        /* current_task */
#include <sys/rigel_net.h>

#include "lwip/opt.h"
#include "lwip/sockets.h"

#define MAX_NSOCK  12

typedef struct {
    int  used;
    int  lfd;                     /* descritor do lwIP */
    int  owner;                   /* id da task dona */
} nsock_t;

static nsock_t ns[MAX_NSOCK];

static int cur_owner(void) { return current_task ? (int)current_task->id : 0; }

/* ---------------------------------------------------------------- File ops */
static int sock_read(File *f, void *buffer, size_t size)
{
    nsock_t *s = (nsock_t *)f->private_data;
    if (!s || !s->used) return -1;
    if (size == 0) return 0;
    return (int)lwip_recv(s->lfd, buffer, size, 0);
}

static int sock_write(File *f, const void *buffer, size_t size)
{
    nsock_t *s = (nsock_t *)f->private_data;
    size_t total = 0;
    const uint8_t *p = (const uint8_t *)buffer;

    if (!s || !s->used) return -1;
    while (total < size) {
        int n = (int)lwip_send(s->lfd, p + total, size - total, 0);
        if (n <= 0) return total ? (int)total : -1;
        total += (size_t)n;
    }
    return (int)total;
}

static int sock_close(File *f)
{
    nsock_t *s = (nsock_t *)f->private_data;
    if (!s) return 0;
    f->private_data = NULL;
    if (s->used) {
        lwip_close(s->lfd);
        s->used = 0;
    }
    return 0;
}

static nsock_t *alloc_ns(int lfd)
{
    int i;
    for (i = 0; i < MAX_NSOCK; i++) {
        if (!ns[i].used) {
            ns[i].used  = 1;
            ns[i].lfd   = lfd;
            ns[i].owner = cur_owner();
            return &ns[i];
        }
    }
    return NULL;
}

/* cria File + fd para um socket do lwIP; em erro fecha o socket do lwIP e devolve -1 */
static long wrap_socket(int lfd)
{
    nsock_t *s;
    File *f;
    int fd;

    if (lfd < 0) return -1;
    s = alloc_ns(lfd);
    if (!s) { lwip_close(lfd); return -1; }

    f = (File *)kmalloc(sizeof(File));
    if (!f) { s->used = 0; lwip_close(lfd); return -1; }
    memset(f, 0, sizeof(File));
    f->name = strdup("socket");
    if (!f->name) { kfree(f); s->used = 0; lwip_close(lfd); return -1; }
    f->type         = FILE_TYPE_DEVICE;
    f->private_data = s;
    f->read         = sock_read;
    f->write        = sock_write;
    f->close        = sock_close;

    fd = allocate_fd(f);
    if (fd < 0) {
        kfree(f->name); kfree(f);
        s->used = 0;
        lwip_close(lfd);
        return -1;
    }
    return fd;
}

static nsock_t *get_ns(long fd)
{
    File *f = get_file_from_fd((int)fd);
    nsock_t *s;
    if (!f || f->read != sock_read) return NULL;
    s = (nsock_t *)f->private_data;
    return (s && s->used) ? s : NULL;
}

/* ---------------------------------------------------------------- enderecos
 * 68000 e rede sao big-endian: porta e IP ja estao em ordem de rede. */
static void to_lwip(const rigel_sockaddr_in *a, struct sockaddr_in *l)
{
    memset(l, 0, sizeof *l);
    l->sin_len         = sizeof *l;
    l->sin_family      = AF_INET;
    l->sin_port        = a->sin_port;
    l->sin_addr.s_addr = a->sin_addr;
}

static void from_lwip(const struct sockaddr_in *l, rigel_sockaddr_in *a)
{
    memset(a, 0, sizeof *a);
    a->sin_family = RIGEL_AF_INET;
    a->sin_port   = l->sin_port;
    a->sin_addr   = l->sin_addr.s_addr;
}

/* ---------------------------------------------------------------- syscalls */
long sys_socket(long domain, long type, long protocol)
{
    (void)protocol;
    if (domain != RIGEL_AF_INET || type != RIGEL_SOCK_STREAM) return -1;
    return wrap_socket(lwip_socket(AF_INET, SOCK_STREAM, 0));
}

long sys_bind(long fd, const rigel_sockaddr_in *a, long len)
{
    nsock_t *s = get_ns(fd);
    struct sockaddr_in l;

    if (!s || !a || len < (long)sizeof(rigel_sockaddr_in)) return -1;
    if (a->sin_family != RIGEL_AF_INET) return -1;
    to_lwip(a, &l);
    return lwip_bind(s->lfd, (struct sockaddr *)&l, sizeof l) == 0 ? 0 : -1;
}

long sys_listen(long fd, long backlog)
{
    nsock_t *s = get_ns(fd);
    if (!s) return -1;
    if (backlog < 1) backlog = 1;
    return lwip_listen(s->lfd, (int)backlog) == 0 ? 0 : -1;
}

long sys_accept(long fd, rigel_sockaddr_in *a, long *len)
{
    nsock_t *s = get_ns(fd);
    struct sockaddr_in l;
    socklen_t ll = sizeof l;
    int nl;

    if (!s) return -1;
    nl = lwip_accept(s->lfd, (struct sockaddr *)&l, &ll);
    if (nl < 0) return -1;
    if (a) {
        from_lwip(&l, a);
        if (len) *len = (long)sizeof *a;
    }
    return wrap_socket(nl);
}

long sys_connect(long fd, const rigel_sockaddr_in *a, long len)
{
    nsock_t *s = get_ns(fd);
    struct sockaddr_in l;

    if (!s || !a || len < (long)sizeof(rigel_sockaddr_in)) return -1;
    if (a->sin_family != RIGEL_AF_INET) return -1;
    to_lwip(a, &l);
    return lwip_connect(s->lfd, (struct sockaddr *)&l, sizeof l) == 0 ? 0 : -1;
}

long sys_setsockopt(long fd, long level, long opt, const void *val, long len)
{
    nsock_t *s = get_ns(fd);
    int v;

    if (!s || !val || len < (long)sizeof(uint32_t)) return -1;
    v = (int)*(const uint32_t *)val;

    if (level == RIGEL_SOL_SOCKET) {
        switch (opt) {
        case RIGEL_SO_REUSEADDR:
            lwip_setsockopt(s->lfd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof v);
            return 0;                                   /* se o lwIP nao suporta, ignora */
        case RIGEL_SO_KEEPALIVE:
            lwip_setsockopt(s->lfd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof v);
            return 0;
        case RIGEL_SO_RCVTIMEO:                         /* v = milissegundos (0 = sem limite) */
            return lwip_setsockopt(s->lfd, SOL_SOCKET, SO_RCVTIMEO, &v, sizeof v) == 0 ? 0 : -1;
        case RIGEL_SO_SNDTIMEO:
            return lwip_setsockopt(s->lfd, SOL_SOCKET, SO_SNDTIMEO, &v, sizeof v) == 0 ? 0 : -1;
        default:
            return -1;
        }
    }
    if (level == RIGEL_IPPROTO_TCP_LV && opt == RIGEL_TCP_NODELAY)
        return lwip_setsockopt(s->lfd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof v) == 0 ? 0 : -1;
    return -1;
}

/* chamado pelo SYS_EXIT: fecha os sockets que a task deixou abertos */
void sys_net_task_exit(int owner)
{
    int fd;
    File *f;

    for (fd = 3; fd < 256; fd++) {
        f = get_file_from_fd(fd);
        if (f && f->read == sock_read && f->private_data &&
            ((nsock_t *)f->private_data)->owner == owner)
            vfs_close(fd);
    }
}
