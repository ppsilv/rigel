#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include <sys/kmalloc.h>
#include <sys/scheduler.h>   /* tcb_t, current_task, OS_MAX_FDS */
#include <sys/critical.h>
#include "fileio.h"

// ============================================
// TABELA DE DESCRITORES POR TASK
// ============================================
// Cada task tem a sua tabela (tcb->fds). Quando a task nasce, herda do pai SO' o
// stdin/stdout/stderr (0..2) -- vfs_task_init(). Os outros descritores nao sao herdados
// (assim um socket do telnetd nao fica preso no shell). Quando a task termina,
// vfs_task_exit() fecha tudo o que ela deixou aberto.
//
// Antes de existir task (boot) vale boot_fds; as primeiras tasks herdam dela, entao
// vfs_init() tem que rodar ANTES de criar as tasks que escrevem em stdout.
//
// O mesmo File pode estar em varias tabelas (e em varios descritores): file->ref_count
// conta quantos descritores apontam para ele; so' fecha de verdade no ultimo.
static File *boot_fds[OS_MAX_FDS];

static File **fdt(void) {
    return current_task ? current_task->fds : boot_fds;
}

#define FD_OK(fd) ((fd) >= 0 && (fd) < OS_MAX_FDS)

extern DeviceDriver drivers[];
extern void kernel_puts(const char *s);
extern void kernel_puthex(uint32_t v);

/* Depuracao do vfs_write (imprime na VGA). Para desligar: apague a linha abaixo. */
/* #define VFS_DEBUG 1 */
#ifdef VFS_DEBUG
#define VDBG_S(s) kernel_puts(s)
#define VDBG_H(v) kernel_puthex((uint32_t)(v))
#else
#define VDBG_S(s) ((void)0)
#define VDBG_H(v) ((void)0)
#endif

// ============================================
// CONTAGEM DE REFERENCIAS
// ============================================
static void file_ref(File *f) {
    uint16_t sr = cs_enter();
    f->ref_count++;
    cs_leave(sr);
}

/* solta uma referencia; na ultima fecha o File de verdade */
static int file_release(File *f) {
    int left, r = 0;
    uint16_t sr = cs_enter();
    left = --f->ref_count;
    cs_leave(sr);
    if (left > 0)
        return 0;
    if (f->close)
        r = f->close(f);
    if (f->name)
        kfree(f->name);
    kfree(f);
    return r;
}

static char *kstrdup(const char *s) {          /* kmalloc/kfree do inicio ao fim (o strdup da newlib usa outro heap) */
    size_t n = strlen(s) + 1;
    char *p = (char *)kmalloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

// ============================================
// FUNÇÕES DE ACESSO À TABELA DE DESCRITORES
// ============================================
// vfs_set_fd/vfs_restore_fd: mecanismo antigo (troca o ponteiro sem mexer em ref_count).
void vfs_set_fd(int fd, File *file) {
    if (FD_OK(fd)) {
        fdt()[fd] = file;
    }
}

File *vfs_get_fd(int fd) {
    if (FD_OK(fd)) {
        return fdt()[fd];
    }
    return NULL;
}

void vfs_restore_fd(int fd) {
    if (FD_OK(fd)) {
        File **t = fdt();
        // Restaura para /dev/tty
        int tty_fd = vfs_open("/dev/tty", O_RDWR);
        if (tty_fd >= 0) {
            t[fd] = t[tty_fd];
            t[tty_fd] = NULL;
        }
    }
}

// ============================================
// ALLOCATE_FD
// ============================================
int allocate_fd(File *file) {
    File **t = fdt();
    for (int i = 3; i < OS_MAX_FDS; i++) {
        if (t[i] == NULL) {
            file->ref_count = 1;
            t[i] = file;
            return i;
        }
    }
    return -2;
}

// ============================================
// GET_FILE_FROM_FD
// ============================================
File *get_file_from_fd(int fd) {
    if (!FD_OK(fd)) return NULL;
    return fdt()[fd];
}

// ============================================
// kfree_FD
// ============================================
void kfree_fd(int fd) {
    if (FD_OK(fd)) {
        fdt()[fd] = NULL;
    }
}

// ============================================
// VFS_OPEN
// ============================================
// Ordem: 1) tabela de dispositivos (/dev/..., /proc/...)  2) arquivos no FAT.
// Os dispositivos TEM que ser consultados primeiro: se o fat_open vier antes
// e aceitar o caminho, o fd fica preso a um arquivo do cartao (fat_write) e
// o texto nunca chega ao console.
extern int fat_open(File *file, const char *path, int flags);
int vfs_open(const char *path, int flags) {
    File *file = (File*)kmalloc(sizeof(File));
    if (!file)
        return -1;

    memset(file, 0, sizeof(File));
    file->name = kstrdup(path);
    if (!file->name) {
        kfree(file);
        return -1;
    }

    /* 1) dispositivos */
    for (int i = 0; drivers[i].path != NULL; i++) {
        if (strcmp(path, drivers[i].path) == 0) {
            if (drivers[i].open(file, path, flags) == 0) {
                int fd = allocate_fd(file);
                if (fd >= 0)
                    return fd;
                if (file->close)                /* sem descritor livre: devolve o recurso do driver */
                    file->close(file);
            }
            /* e' um dispositivo conhecido: nao tenta o FAT */
            kfree(file->name);
            kfree(file);
            return -1;
        }
    }

    /* 2) arquivo no FAT */
    if (fat_open(file, path, flags) == 0) {
        int fd = allocate_fd(file);
        if (fd >= 0)
            return fd;
        if (file->close)
            file->close(file);
    }

    kfree(file->name);
    kfree(file);
    return -1;
}

// ============================================
// VFS_READ
// ============================================
int vfs_read(int fd, void *buffer, size_t size) {
    File *file = get_file_from_fd(fd);
    if (!file) return -1;
    if (file->read) return file->read(file, buffer, size);
    return -1;
}

// ============================================
// VFS_WRITE
// ============================================
int vfs_write(int fd, const void *buffer, size_t size) {
    File *file = get_file_from_fd(fd);
    VDBG_S("[vw file="); VDBG_H((uintptr_t)file);
    if (!file) { VDBG_S(" NULL]\n"); return -1; }
    VDBG_S(" write="); VDBG_H((uintptr_t)file->write); VDBG_S("]\n");
    if (file->write) return file->write(file, buffer, size);
    return -1;
}

// ============================================
// VFS_CLOSE
// ============================================
// Solta este descritor. O File so' fecha de verdade quando nenhum outro descritor
// (desta ou de outra task) aponta para ele.
int vfs_close(int fd) {
    File *file = get_file_from_fd(fd);
    if (!file) return -1;

    kfree_fd(fd);
    return file_release(file);
}

// ============================================
// VFS_IOCTL
// ============================================
int vfs_ioctl(int fd, int cmd, void *arg) {
    File *file = get_file_from_fd(fd);
    if (!file) return -1;
    if (file->ioctl) return file->ioctl(file, cmd, arg);
    return -1;
}

// ============================================
// VFS_LSEEK
// ============================================
size_t vfs_lseek(int fd, size_t offset, int whence) {
    File *file = get_file_from_fd(fd);
    if (!file) return (size_t)-1;

    if (file->lseek) return file->lseek(file, offset, whence);

    switch (whence) {
        case 0: file->position = offset; break;
        case 1: file->position += offset; break;
        case 2: return (size_t)-1;
        default: return (size_t)-1;
    }
    return file->position;
}

// ============================================
// VFS_INIT
// ============================================
// allocate_fd() comeca em 3, entao os tres vfs_open abaixo caem em
// fd 3, 4 e 5. Depois movemos cada File para 0, 1 e 2 (stdin, stdout,
// stderr). Sem isso o write(1,...) dos apps devolve -1.
// Dispositivos ligados a stdin/stdout/stderr dos apps. Para trocar a E/S
// basta mudar estes tres nomes:
//   "/dev/vcon" -> teclado (ringbuffer) + video VGA   (vfs_vga.c)
//   "/dev/tty"  -> DUART com \n -> \r\n e eco       (tty_read/tty_write)
//   "/dev/tty0" -> DUART direto (duartA_*), sem conversao de \n
#define STDIN_DEVICE   "/dev/vcon"
#define STDOUT_DEVICE  "/dev/vcon"
#define STDERR_DEVICE  "/dev/vcon"

static void move_fd(int de, int para) {
    File **t = fdt();
    if (de < 0 || de == para)
        return;
    t[para] = t[de];
    t[de]   = NULL;                  /* o File continua em uso: ref_count nao muda */
}

void vfs_init(void) {
    memset(fdt(), 0, sizeof(File *) * OS_MAX_FDS);

    int fd0 = vfs_open(STDIN_DEVICE,  O_RDWR);
    int fd1 = vfs_open(STDOUT_DEVICE, O_RDWR);
    int fd2 = vfs_open(STDERR_DEVICE, O_RDWR);

    if (fd0 < 0 || fd1 < 0 || fd2 < 0) {
        kernel_puts("vfs_init: nao abriu stdin/stdout/stderr\n");
        return;
    }

    move_fd(fd0, 0);
    move_fd(fd1, 1);
    move_fd(fd2, 2);
}

// ============================================
// VFS_DUP - dup2: faz newfd apontar para o mesmo File de oldfd
// Se newfd ja tinha um File e este era a ultima referencia, ele e' fechado aqui.
// Devolve newfd, ou -1.
// ============================================
int vfs_dup(int oldfd, int newfd) {
    File **t = fdt();
    File *f, *prev;

    if (!FD_OK(oldfd) || !FD_OK(newfd))
        return -1;
    f = t[oldfd];
    if (f == NULL)
        return -1;
    if (oldfd == newfd)
        return newfd;

    prev = t[newfd];
    if (prev == f)
        return newfd;
    file_ref(f);
    t[newfd] = f;
    if (prev != NULL)
        file_release(prev);
    return newfd;
}

// ============================================
// DESCRITORES POR TASK
// ============================================
/* stdio explicito para o proximo filho (sys_spawn_io). So' valido entre _begin e _end,
 * que o spawn.c chama com o lock do spawn tomado. */
static File *spawn_io[3];
static int   spawn_io_on;

int vfs_spawn_io_begin(int fd_in, int fd_out, int fd_err) {
    File **t = fdt();
    int fds[3];
    int i;

    fds[0] = fd_in; fds[1] = fd_out; fds[2] = fd_err;
    for (i = 0; i < 3; i++) {
        if (!FD_OK(fds[i]) || t[fds[i]] == NULL)
            return -1;
    }
    for (i = 0; i < 3; i++)
        spawn_io[i] = t[fds[i]];
    spawn_io_on = 1;
    return 0;
}

void vfs_spawn_io_end(void) {
    spawn_io_on = 0;
    spawn_io[0] = spawn_io[1] = spawn_io[2] = NULL;
}

/* Chamada pelo OS_TaskCreate (e OS_TaskCreatePIC), com IRQ mascarada, ainda como a task PAI. */
void vfs_task_init(struct tcb *t) {
    File **src = fdt();
    int i;

    memset(t->fds, 0, sizeof(t->fds));
    for (i = 0; i < 3; i++) {
        File *f = spawn_io_on ? spawn_io[i] : src[i];
        if (f) {
            file_ref(f);
            t->fds[i] = f;
        }
    }
}

/* Chamada pelo OS_TaskExit, ainda na propria task (o close pode bloquear). */
void vfs_task_exit(struct tcb *t) {
    for (int i = 0; i < OS_MAX_FDS; i++) {
        File *f = t->fds[i];
        if (f) {
            t->fds[i] = NULL;
            file_release(f);
        }
    }
}
