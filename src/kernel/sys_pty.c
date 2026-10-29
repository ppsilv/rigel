/*
 * pty.c -- pseudo-terminal do Rigel: liga o telnetd a um shell.
 *
 * Sao dois buffers circulares e duas pontas (cada uma e' um File):
 *
 *   ponta do SHELL (slave): vira stdin/stdout/stderr do shell.
 *      read  : disciplina de linha (linedisc.c) sobre o buffer "teclas": linha, eco,
 *              backspace, TTY_RAW/NOECHO/NONBLOCK -- igual ao /dev/vcon. Bloqueia dormindo.
 *              Devolve 0 quando o telnetd fechou a sua ponta (fim de arquivo).
 *      write : poe no buffer "tela". Bloqueia (dormindo) se encher. -1 se o telnetd fechou.
 *      ioctl : TIOC_SETMODE / TIOC_GETMODE / TIOC_CLEAR, como o /dev/vcon.
 *
 *   ponta do TELNETD (master): NUNCA bloqueia (o telnetd tem que atender rede e shell no mesmo laco).
 *      read  : o que houver no buffer "tela". 0 = nada agora; -1 = nada e o shell fechou (fim).
 *      write : poe no buffer "teclas" o que couber e devolve quantos bytes coube (0 = cheio);
 *              -1 se o shell fechou.
 *
 * A memoria do par e' liberada quando as DUAS pontas foram fechadas.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/kmalloc.h>
#include <sys/scheduler.h>
#include <sys/critical.h>
#include <sys/linedisc.h>
#include <sys/sys_pty.h>

#define PTY_IN_SIZE   256      /* teclas: telnetd -> shell */
#define PTY_OUT_SIZE  1024     /* tela:   shell -> telnetd */

typedef struct {
    uint8_t *buf;
    uint16_t size, head, tail, count;
} ring_t;

typedef struct pty {
    ring_t in;                 /* teclas */
    ring_t out;                /* tela   */
    volatile uint8_t master_open;
    volatile uint8_t slave_open;
    uint8_t in_buf[PTY_IN_SIZE];
    uint8_t out_buf[PTY_OUT_SIZE];
} pty_t;

typedef struct {
    pty_t   *p;
    uint32_t mode;             /* TTY_* (usado so' na ponta do shell) */
    uint8_t  is_master;
} pty_end_t;

/* ---------- buffer circular (uma operacao = uma secao critica curta) ---------- */
static int ring_put(ring_t *r, uint8_t c)
{
    uint16_t sr = cs_enter();
    int ok = 0;
    if (r->count < r->size) {
        r->buf[r->head] = c;
        r->head = (uint16_t)((r->head + 1 == r->size) ? 0 : r->head + 1);
        r->count++;
        ok = 1;
    }
    cs_leave(sr);
    return ok;
}

static int ring_get(ring_t *r)
{
    uint16_t sr = cs_enter();
    int c = -1;
    if (r->count) {
        c = r->buf[r->tail];
        r->tail = (uint16_t)((r->tail + 1 == r->size) ? 0 : r->tail + 1);
        r->count--;
    }
    cs_leave(sr);
    return c;
}

/* poe um byte em p->out esperando (dormindo) se encher; desiste se o telnetd fechou */
static void out_put(pty_t *p, uint8_t c)
{
    while (!ring_put(&p->out, c)) {
        if (!p->master_open)
            return;
        OS_SleepTicks(1);
    }
}

/* ---------- ponta do shell ---------- */
static int slave_getch(void *ctx, int block)
{
    pty_end_t *e = (pty_end_t *)ctx;
    pty_t *p = e->p;

    for (;;) {
        int open = p->master_open;      /* ler a flag ANTES: se fechou, o que ele escreveu ja' esta no buffer */
        int c = ring_get(&p->in);
        if (c >= 0)
            return c;
        if (!open)
            return LD_EOF;
        if (!block)
            return LD_NONE;
        OS_SleepTicks(1);
    }
}

static void slave_putch(void *ctx, uint8_t c)
{
    out_put(((pty_end_t *)ctx)->p, c);
}

static int slave_read(File *file, void *buffer, size_t size)
{
    pty_end_t *e = (pty_end_t *)file->private_data;
    ld_io_t io;

    io.getch   = slave_getch;
    io.putch   = slave_putch;
    io.ctx     = e;
    io.bs_echo = 1;
    return linedisc_read(&io, e->mode, buffer, size);
}

static int slave_write(File *file, const void *buffer, size_t size)
{
    pty_end_t *e = (pty_end_t *)file->private_data;
    const uint8_t *b = (const uint8_t *)buffer;
    size_t i;

    if (!e->p->master_open)
        return -1;
    for (i = 0; i < size; i++)
        out_put(e->p, b[i]);
    return (int)size;
}

static int slave_ioctl(File *file, int cmd, void *arg)
{
    pty_end_t *e = (pty_end_t *)file->private_data;
    static const char clr[] = "\033[2J\033[H";
    size_t i;

    switch (cmd) {
    case TIOC_CLEAR:
        for (i = 0; i < sizeof clr - 1; i++)
            out_put(e->p, (uint8_t)clr[i]);
        return 0;
    case TIOC_SETMODE:
        e->mode = (uint32_t)(uintptr_t)arg & (TTY_RAW | TTY_NOECHO | TTY_NONBLOCK);
        return 0;
    case TIOC_GETMODE:
        return (int)e->mode;
    default:
        return -1;
    }
}

/* ---------- ponta do telnetd ---------- */
static int master_read(File *file, void *buffer, size_t size)
{
    pty_end_t *e = (pty_end_t *)file->private_data;
    pty_t *p = e->p;
    uint8_t *b = (uint8_t *)buffer;
    size_t n = 0;
    int open = p->slave_open;           /* ANTES de esvaziar (mesma razao do slave_getch) */

    while (n < size) {
        int c = ring_get(&p->out);
        if (c < 0)
            break;
        b[n++] = (uint8_t)c;
    }
    if (n == 0 && size > 0 && !open)
        return -1;                      /* acabou: o shell fechou e nao ha mais nada */
    return (int)n;
}

static int master_write(File *file, const void *buffer, size_t size)
{
    pty_end_t *e = (pty_end_t *)file->private_data;
    const uint8_t *b = (const uint8_t *)buffer;
    size_t n = 0;

    if (!e->p->slave_open)
        return -1;
    while (n < size && ring_put(&e->p->in, b[n]))
        n++;
    return (int)n;
}

static int master_ioctl(File *file, int cmd, void *arg)
{
    (void)file; (void)cmd; (void)arg;
    return -1;
}

/* ---------- comum ---------- */
static int pty_close(File *file)
{
    pty_end_t *e = (pty_end_t *)file->private_data;
    pty_t *p;
    int last;
    uint16_t sr;

    if (e == NULL)
        return 0;
    p = e->p;
    sr = cs_enter();
    if (e->is_master) p->master_open = 0; else p->slave_open = 0;
    last = !p->master_open && !p->slave_open;
    cs_leave(sr);

    kfree(e);
    file->private_data = NULL;
    if (last)
        kfree(p);
    return 0;
}

static size_t pty_lseek(File *file, size_t offset, int whence)
{
    (void)file; (void)offset; (void)whence;
    return (size_t)-1;
}

static void kfree_nn(void *p)
{
    if (p)
        kfree(p);
}

static File *new_end_file(pty_t *p, pty_end_t *e, int is_master)
{
    File *f = (File *)kmalloc(sizeof(File));

    if (f == NULL)
        return NULL;
    memset(f, 0, sizeof(File));
    e->p = p;
    e->mode = 0;
    e->is_master = (uint8_t)is_master;

    f->name         = NULL;             /* vfs_close ignora nome NULL */
    f->type         = FILE_TYPE_DEVICE;
    f->private_data = e;
    f->close        = pty_close;
    f->lseek        = pty_lseek;
    if (is_master) {
        f->read  = master_read;
        f->write = master_write;
        f->ioctl = master_ioctl;
    } else {
        f->read  = slave_read;
        f->write = slave_write;
        f->ioctl = slave_ioctl;
    }
    return f;
}

int pty_create(File **master, File **slave)
{
    pty_t *p  = (pty_t *)kmalloc(sizeof(pty_t));
    pty_end_t *em = (pty_end_t *)kmalloc(sizeof(pty_end_t));
    pty_end_t *es = (pty_end_t *)kmalloc(sizeof(pty_end_t));
    File *fm = NULL, *fs = NULL;

    if (p && em && es) {
        memset(p, 0, sizeof(pty_t));
        p->in.buf   = p->in_buf;   p->in.size  = PTY_IN_SIZE;
        p->out.buf  = p->out_buf;  p->out.size = PTY_OUT_SIZE;
        p->master_open = 1;
        p->slave_open  = 1;
        fm = new_end_file(p, em, 1);
        fs = new_end_file(p, es, 0);
    }
    //if (fm == NULL || fs == NULL) {
    //    kfree_nn(fm); kfree_nn(fs); kfree_nn(em); kfree_nn(es); kfree_nn(p);
    //    return -1;
    //}
    if (!p || !em || !es) {
        kfree_nn(fm); kfree_nn(fs); kfree_nn(em); kfree_nn(es); kfree_nn(p);
        return -11;                 /* kmalloc de pty_t ou das pontas falhou */
    }
    if (fm == NULL || fs == NULL) {
        kfree_nn(fm); kfree_nn(fs); kfree_nn(em); kfree_nn(es); kfree_nn(p);
        return -12;                 /* kmalloc do File falhou */
    }

    *master = fm;
    *slave  = fs;
    return 0;
}

int pty_open_fds(int fds[2])
{
    File *m, *s;
    int mfd, sfd;

    if (pty_create(&m, &s) != 0)
        return -2;
    mfd = allocate_fd(m);
    if (mfd < 0) {
        m->close(m); kfree(m);
        s->close(s); kfree(s);
        return -3;
    }
    sfd = allocate_fd(s);
    if (sfd < 0) {
        vfs_close(mfd);                 /* fecha o master (libera File e ponta) */
        s->close(s); kfree(s);          /* e o slave, que nunca entrou na tabela: libera o par */
        return -4;
    }
    fds[0] = mfd;
    fds[1] = sfd;
    return 0;
}
