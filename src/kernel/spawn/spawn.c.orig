/*
 * spawn.c -- SYS_SPAWN do kernel Rigel.
 *
 *   int sys_spawn(const char *path, char **argv);
 *
 * path : arquivo ELF no SD (ex.: "/bin/ls.elf" ou "hello.elf")
 * argv : vetor terminado em NULL (argv[0] = nome); NULL => so' {path}
 * Retorna o pid (>0, = id da task) ou -1 em erro.
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

int sys_spawn(const char *path, char **argv)
{
    char *def[2];
    int argc = 0, pid = -1;

    if (path == NULL) return -1;

    if (argv == NULL) {
        def[0] = (char *)path;
        def[1] = NULL;
        argv = def;
    }
    while (argv[argc] != NULL && argc < 64) argc++;

    /* pega o "lock" (teste-e-marca com IRQ mascarada) */
    for (;;) {
        uint16_t sr = irq_off();
        int got = !spawn_busy;
        if (got) spawn_busy = 1;
        irq_back(sr);
        if (got) break;
    }

    if (f_open(&spawn_fil, path, FA_READ) == FR_OK) {
        if (load_elf_spawn(&spawn_fil, argc, argv, &pid) != 0)
            pid = -1;
        f_close(&spawn_fil);
    }

    spawn_busy = 0;
    return pid;
}
