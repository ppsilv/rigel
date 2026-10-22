#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <fatfs/ff.h>

// ============================================
// TIPOS DE ARQUIVO
// ============================================
#define FILE_TYPE_REGULAR   0
#define FILE_TYPE_DEVICE    1
#define FILE_TYPE_PROC      2

// ============================================
// FLAGS PARA OPEN
// ============================================
#define O_RDONLY    0x01
#define O_WRONLY    0x02
#define O_RDWR      0x03
#define O_CREAT     0x04
#define O_TRUNC     0x08
#define O_APPEND    0x10

// ============================================
// COMANDOS IOCTL PARA TTY
// ============================================
#define TIOC_CLEAR  0x01
#define TIOC_GETXY  0x02
#define TIOC_SETXY  0x03
#define TIOC_SETMODE 0x04   /* arg = (void *)flags TTY_*  : define o modo do /dev/vcon */
#define TIOC_GETMODE 0x05   /* devolve os flags TTY_* atuais */

// ============================================
// FLAGS DE MODO DO /dev/vcon (TIOC_SETMODE)
// Modo padrao = 0: leitura em linha, com eco e edicao (backspace).
// ============================================
#define TTY_RAW       0x01  /* read devolve assim que chega 1+ tecla, sem editar a linha */
#define TTY_NOECHO    0x02  /* nao ecoa na tela o que e' digitado */
#define TTY_NONBLOCK  0x04  /* so' com TTY_RAW: read devolve 0 se nao ha tecla, sem esperar */

// ============================================
// ESTRUTURA FILE
// ============================================
typedef struct File {
    char *name;
    int type;
    size_t position;
    int ref_count;
    void *private_data;

    int (*open)(struct File *file, const char *path, int flags);
    int (*read)(struct File *file, void *buffer, size_t size);
    int (*write)(struct File *file, const void *buffer, size_t size);
    int (*close)(struct File *file);
    int (*ioctl)(struct File *file, int cmd, void *arg);
    size_t (*lseek)(struct File *file, size_t offset, int whence);
} File;

// ============================================
// ESTRUTURA DEVICE DRIVER
// ============================================
typedef struct {
    const char *path;
    int (*open)(File *file, const char *path, int flags);
    int (*read)(File *file, void *buffer, size_t size);
    int (*write)(File *file, const void *buffer, size_t size);
    int (*close)(File *file);
    int (*ioctl)(File *file, int cmd, void *arg);
    size_t (*lseek)(File *file, size_t offset, int whence);

} DeviceDriver;

// ============================================
// FUNÇÕES DO VFS
// ============================================
int vfs_open(const char *path, int flags);
int vfs_read(int fd, void *buffer, size_t size);
int vfs_write(int fd, const void *buffer, size_t size);
int vfs_close(int fd);
int vfs_ioctl(int fd, int cmd, void *arg);
size_t vfs_lseek(int fd, size_t offset, int whence);

// ============================================
// FUNÇÕES DE GERÊNCIA DE DESCRITORES
// ============================================
int allocate_fd(File *file);
File *get_file_from_fd(int fd);
void free_fd(int fd);

// ============================================
// FUNÇÕES PARA MANIPULAR DESCRITORES
// ============================================
void vfs_set_fd(int fd, File *file);
File *vfs_get_fd(int fd);
void vfs_restore_fd(int fd);
int  vfs_dup(int oldfd, int newfd);                 /* dup2 */

// ============================================
// DESCRITORES POR TASK (ver vfs.c)
// ============================================
struct tcb;
void vfs_task_init(struct tcb *t);                  /* OS_TaskCreate: herda stdin/stdout/stderr do pai */
void vfs_task_exit(struct tcb *t);                  /* OS_TaskExit: fecha todos os descritores da task */
int  vfs_spawn_io_begin(int fd_in, int fd_out, int fd_err);  /* so' spawn.c: stdio explicito p/ o filho */
void vfs_spawn_io_end(void);

// ============================================
// INICIALIZAÇÃO
// ============================================
void vfs_init(void);

#endif /* VFS_H */
