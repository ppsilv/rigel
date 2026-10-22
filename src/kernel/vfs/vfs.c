#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include <sys/kmalloc.h>
#include "fileio.h" 

// ============================================
// TABELA DE DESCRITORES GLOBAL
// ============================================
File *fd_table[256];
static int fd_count = 0;

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
// FUNÇÕES DE ACESSO À TABELA DE DESCRITORES
// ============================================

void vfs_set_fd(int fd, File *file) {
    if (fd >= 0 && fd < 256) {
        fd_table[fd] = file;
    }
}

File *vfs_get_fd(int fd) {
    if (fd >= 0 && fd < 256) {
        return fd_table[fd];
    }
    return NULL;
}

void vfs_restore_fd(int fd) {
    if (fd >= 0 && fd < 256) {
        // Restaura para /dev/tty
        int tty_fd = vfs_open("/dev/tty", O_RDWR);
        if (tty_fd >= 0) {
            fd_table[fd] = fd_table[tty_fd];
            fd_table[tty_fd] = NULL;
        }
    }
}

// ============================================
// ALLOCATE_FD
// ============================================
int allocate_fd(File *file) {
    for (int i = 3; i < 256; i++) {
        if (fd_table[i] == NULL) {
            fd_table[i] = file;
            fd_count++;
            return i;
        }
    }
    return -1;
}

// ============================================
// GET_FILE_FROM_FD
// ============================================
File *get_file_from_fd(int fd) {
    if (fd < 0 || fd >= 256) return NULL;
    return fd_table[fd];
}

// ============================================
// kfree_FD
// ============================================
void kfree_fd(int fd) {
    if (fd >= 0 && fd < 256) {
        fd_table[fd] = NULL;
        fd_count--;
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
    file->name = strdup(path);
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
int vfs_close(int fd) {
    File *file = get_file_from_fd(fd);
    if (!file) return -1;

    /* dup: se OUTRO fd ainda aponta para este mesmo File, so' solta este numero */
    for (int i = 0; i < 256; i++) {
        if (i != fd && fd_table[i] == file) {
            kfree_fd(fd);
            return 0;
        }
    }
    
    int result = 0;
    if (file->close) result = file->close(file);
    
    kfree(file->name);
    kfree(file);
    kfree_fd(fd);
    return result;
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
    if (de < 0 || de == para)
        return;
    fd_table[para] = fd_table[de];
    fd_table[de]   = NULL;           /* o File continua em uso: fd_count nao muda */
}

void vfs_init(void) {
    memset(fd_table, 0, sizeof(fd_table));
    fd_count = 0;

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
// Se newfd ja tinha um File e ele nao e' usado por nenhum outro fd, e' fechado aqui.
// Devolve newfd, ou -1.
// ============================================
static int vfs_fd_shared(File *f) {
    for (int i = 0; i < 256; i++) {
        if (fd_table[i] == f)
            return 1;
    }
    return 0;
}

int vfs_dup(int oldfd, int newfd) {
    File *f, *prev;

    if (oldfd < 0 || oldfd >= 256 || newfd < 0 || newfd >= 256)
        return -1;
    f = fd_table[oldfd];
    if (f == NULL)
        return -1;
    if (oldfd == newfd)
        return newfd;

    prev = fd_table[newfd];
    fd_table[newfd] = f;
    if (prev == NULL) {
        fd_count++;
    } else if (!vfs_fd_shared(prev)) {          /* era a ultima referencia: fecha de verdade */
        if (prev->close) prev->close(prev);
        kfree(prev->name);
        kfree(prev);
    }
    return newfd;
}
