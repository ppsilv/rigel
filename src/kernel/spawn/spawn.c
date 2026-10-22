/*
 * spawn.c -- SYS_SPAWN do kernel Rigel.
 *
 *   int sys_spawn(const char *path, char **argv);
 *
 * path : arquivo ELF no SD (ex.: "/bin/ls.elf" ou "hello.elf")
 * argv : vetor terminado em NULL (argv[0] = nome); NULL => so' {path}
 * Retorna o pid (>0, = id da task) ou -1 em erro.
 *
 * sys_spawn_io: igual, mas com stdin/stdout/stderr escolhidos (ver mais abaixo).
 *
 * O FIL e' estatico e protegido por um flag (so' um spawn por vez),
 * porque o FatFS nao e' reentrante.
 */
#include <stddef.h>
#include <stdint.h>
#include <fileio.h>
#include <fatfs/ff.h>

extern int load_elf_spawn(FIL *fd, int argc, char *argv[], int *pid);

#ifdef __m68k__
static inline uint16_t irq_off(void)
{
    uint16_t sr;
    __asm__ volatile ("move.w %%sr,%0\n\tor.w #0x0700,%%sr" : "=d"(sr) : : "memory", "cc");
    return sr;
}
static inline void irq_back(uint16_t sr) { __asm__ volatile ("move.w %0,%%sr" : : "d"(sr) : "memory", "cc"); }
#else
static inline uint16_t irq_off(void) { return 0; }
static inline void irq_back(uint16_t sr) { (void)sr; }
#endif

static volatile int spawn_busy;
static FIL spawn_fil;

extern int  vfs_spawn_io_begin(int fd_in, int fd_out, int fd_err);   /* vfs.c */
extern void vfs_spawn_io_end(void);

static void spawn_lock(void)
{
    /* pega o "lock" (teste-e-marca com IRQ mascarada) */
    for (;;) {
        uint16_t sr = irq_off();
        int got = !spawn_busy;
        if (got) spawn_busy = 1;
        irq_back(sr);
        if (got) break;
    }
}

/* com o lock tomado: abre o ELF e cria a task */
static int spawn_load(const char *path, char **argv)
{
    char *def[2];
    int argc = 0, pid = -1;

    if (argv == NULL) {
        def[0] = (char *)path;
        def[1] = NULL;
        argv = def;
    }
    while (argv[argc] != NULL && argc < 64) argc++;

    if (f_open(&spawn_fil, path, FA_READ) == FR_OK) {
        if (load_elf_spawn(&spawn_fil, argc, argv, &pid) != 0)
            pid = -1;
        f_close(&spawn_fil);
    }
    return pid;
}

int sys_spawn(const char *path, char **argv)
{
    int pid;

    if (path == NULL) return -1;
    spawn_lock();
    pid = spawn_load(path, argv);
    spawn_busy = 0;
    return pid;
}

/*
 *   int sys_spawn_io(const char *path, char **argv, int fd_in, int fd_out, int fd_err);
 *
 * Igual ao sys_spawn, mas o stdin/stdout/stderr do filho sao os descritores fd_in/fd_out/fd_err
 * da task que chama (em vez de herdar os dela). E' o que o telnetd usa para ligar o shell ao pty.
 * O filho herda os File na hora em que a task e' criada (OS_TaskCreate -> vfs_task_init), por isso
 * o lock do spawn cobre todo o intervalo.
 */
int sys_spawn_io(const char *path, char **argv, int fd_in, int fd_out, int fd_err)
{
    int pid;

    if (path == NULL) return -1;
    spawn_lock();
    if (vfs_spawn_io_begin(fd_in, fd_out, fd_err) != 0) {
        spawn_busy = 0;
        return -1;
    }
    pid = spawn_load(path, argv);
    vfs_spawn_io_end();
    spawn_busy = 0;
    return pid;
}
