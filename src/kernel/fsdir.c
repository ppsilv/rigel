/*
 * fsdir.c -- SYS_FOPENDIR / SYS_FREADDIR / SYS_FCLOSEDIR do kernel Rigel.
 *
 *   long sys_opendir(const char *path)              -> handle (0..MAX_DIRS-1) ou -1
 *   long sys_readdir(long h, rigel_dirent_t *ent)   -> 1 = entrada preenchida, 0 = fim, -1 = erro
 *   long sys_closedir(long h)                       -> 0 ou -1
 *
 * O app nao ve o DIR do FatFS: so' um numero (handle). O kernel guarda ate' MAX_DIRS
 * diretorios abertos ao mesmo tempo (todas as tasks juntas).
 */
#include <stdint.h>
#include <string.h>
#include <fatfs/ff.h>
#include <sys/rigel_dirent.h>

#define MAX_DIRS 4

static DIR     dirs[MAX_DIRS];
static uint8_t dir_used[MAX_DIRS];

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

static int dir_alloc(void)
{
    int h = -1;
    uint16_t sr = irq_off();
    for (int i = 0; i < MAX_DIRS; i++)
        if (!dir_used[i]) { dir_used[i] = 1; h = i; break; }
    irq_back(sr);
    return h;
}

long sys_opendir(const char *path)
{
    if (path == NULL) return -1;
    /* "." ou "" = raiz/diretorio atual do FatFS */
    if (path[0] == '.' && path[1] == '\0') path = "";

    int h = dir_alloc();
    if (h < 0) return -1;
    if (f_opendir(&dirs[h], path) != FR_OK) {
        dir_used[h] = 0;
        return -1;
    }
    return h;
}

long sys_readdir(long h, rigel_dirent_t *ent)
{
    FILINFO fno;                      /* local: cada task tem a sua (nao ha' corrida entre tasks) */

    if (h < 0 || h >= MAX_DIRS || !dir_used[h] || ent == NULL) return -1;

    if (f_readdir(&dirs[h], &fno) != FR_OK) return -1;
    if (fno.fname[0] == '\0') return 0;                  /* fim do diretorio */

    ent->size = (uint32_t)fno.fsize;
    ent->date = (uint16_t)fno.fdate;
    ent->time = (uint16_t)fno.ftime;
    ent->attr = (uint8_t)fno.fattrib;
    strncpy(ent->name, fno.fname, RIGEL_NAME_MAX);
    ent->name[RIGEL_NAME_MAX] = '\0';
    return 1;
}

long sys_closedir(long h)
{
    if (h < 0 || h >= MAX_DIRS || !dir_used[h]) return -1;
    f_closedir(&dirs[h]);
    dir_used[h] = 0;
    return 0;
}
