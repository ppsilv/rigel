/*
 * sys_fsfile.c -- arquivos FAT do kernel Rigel (familia SYS_F*), direto sobre o FatFS.
 *
 * ATENCAO: estes "handles" (0..MAX_FILES-1) sao DIFERENTES dos fds do VFS
 * (SYS_OPEN/READ/WRITE, fd 0/1/2 = console). Handle de rigel_ff_open so' serve
 * para rigel_ff_*, rigel_tell, rigel_size, rigel_eof. Nao misture com read()/write().
 *
 * Retorno: >= 0 sucesso; -1 erro (handle invalido, arquivo nao existe, disco...).
 * Quando uma task termina (SYS_EXIT), sys_fs_task_exit() fecha os arquivos que ela esqueceu.
 *
 *   SYS_FOPEN   sys_fopen(path, flags)           handle   (flags = RIGEL_FA_*)
 *   SYS_FCLOSE  sys_fclose(h)                    0
 *   SYS_FREAD   sys_fread(h, buf, size)          bytes lidos (0 = fim)
 *   SYS_FWRITE  sys_fwrite(h, buf, size)         bytes gravados (menor que size = disco cheio)
 *   SYS_FSYNC   sys_fsync(h)                     0
 *   SYS_FLSEEK  sys_flseek(h, off, whence)       nova posicao
 *   SYS_FIOCTL  sys_fioctl(h, cmd, arg)          0   (RIGEL_FIOC_TRUNCATE / RIGEL_FIOC_SYNC)
 *   SYS_FTELL   sys_ftell(h)                     posicao atual
 *   SYS_FSIZE   sys_fsize(h)                     tamanho em bytes
 *   SYS_FEOF    sys_feof(h)                      1 se posicao == tamanho, senao 0
 *   SYS_FUNLINK sys_funlink(path)                0   (apaga arquivo ou diretorio vazio)
 *   SYS_FRENAME sys_frename(old, new)            0
 *   SYS_FSTAT   sys_fstat(path, ent)             0   (preenche rigel_dirent_t)
 *   SYS_FMKDIR  sys_fmkdir(path)                 0
 *   SYS_CHMOD   sys_chmod(path, attrs)           0   (attrs = RIGEL_ATTR_RDO|HID|SYS|ARC; precisa FF_USE_CHMOD=1)
 *   SYS_FMOUNT  sys_fmount(path)                 0   ("0:", "1:"...; monta/remonta o volume)
 *   SYS_FUNMOUNT sys_funmount(path)              0   (o volume 0 = boot nao pode ser desmontado)
 */
#include <stdint.h>
#include <string.h>
#include <fatfs/ff.h>
#include <sys/scheduler.h>        /* current_task */
#include <sys/rigel_dirent.h>
#include <sys/rigel_fsflags.h>

/* nomes antigos do FatFS (< R0.14) */
#ifndef FF_FS_READONLY
#define FF_FS_READONLY _FS_READONLY
#endif
#ifndef FF_FS_MINIMIZE
#define FF_FS_MINIMIZE _FS_MINIMIZE
#endif
#ifndef FF_VOLUMES
#define FF_VOLUMES _VOLUMES
#endif
#ifndef FF_USE_CHMOD
#ifdef _USE_CHMOD
#define FF_USE_CHMOD _USE_CHMOD
#else
#define FF_USE_CHMOD 0
#endif
#endif

/* o layout dos flags do app tem que ser o do FatFS */
_Static_assert(RIGEL_FA_READ  == FA_READ  && RIGEL_FA_WRITE == FA_WRITE &&
               RIGEL_FA_CREATE_NEW == FA_CREATE_NEW && RIGEL_FA_CREATE_ALWAYS == FA_CREATE_ALWAYS &&
               RIGEL_FA_OPEN_ALWAYS == FA_OPEN_ALWAYS && RIGEL_FA_OPEN_APPEND == FA_OPEN_APPEND,
               "RIGEL_FA_* diferente de FA_* do FatFS");

#define MAX_FILES 8               /* arquivos abertos ao mesmo tempo (todas as tasks); ~560 bytes cada */

static FIL     files[MAX_FILES];
static uint8_t file_used[MAX_FILES];
static int     file_owner[MAX_FILES];
static FATFS   vols[FF_VOLUMES];  /* areas de trabalho para rigel_mount */

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

static int cur_owner(void) { return current_task ? (int)current_task->id : 0; }

static int file_alloc(void)
{
    int h = -1;
    uint16_t sr = irq_off();
    for (int i = 0; i < MAX_FILES; i++)
        if (!file_used[i]) { file_used[i] = 1; file_owner[i] = cur_owner(); h = i; break; }
    irq_back(sr);
    return h;
}

#define BAD(h) ((h) < 0 || (h) >= MAX_FILES || !file_used[(h)])

static void fill_ent(rigel_dirent_t *ent, const FILINFO *fno)
{
    ent->size = (uint32_t)fno->fsize;
    ent->date = (uint16_t)fno->fdate;
    ent->time = (uint16_t)fno->ftime;
    ent->attr = (uint8_t)fno->fattrib;
    strncpy(ent->name, fno->fname, RIGEL_NAME_MAX);
    ent->name[RIGEL_NAME_MAX] = '\0';
}

/* ---------------------------------------------------------------- abrir / fechar / ler / gravar */

long sys_fopen(const char *path, long flags)
{
    if (path == NULL) return -1;
    int h = file_alloc();
    if (h < 0) return -1;
    if (f_open(&files[h], path, (BYTE)(flags & 0x3F)) != FR_OK) {
        file_used[h] = 0;
        return -1;
    }
    return h;
}

long sys_fclose(long h)
{
    if (BAD(h)) return -1;
    FRESULT fr = f_close(&files[h]);
    file_used[h] = 0;
    return (fr == FR_OK) ? 0 : -1;
}

long sys_fread(long h, void *buf, unsigned long size)
{
    UINT br = 0;
    if (BAD(h) || buf == NULL) return -1;
    if (f_read(&files[h], buf, (UINT)size, &br) != FR_OK) return -1;
    return (long)br;
}

long sys_fwrite(long h, const void *buf, unsigned long size)
{
#if !FF_FS_READONLY
    UINT bw = 0;
    if (BAD(h) || buf == NULL) return -1;
    if (f_write(&files[h], buf, (UINT)size, &bw) != FR_OK) return -1;
    return (long)bw;
#else
    (void)h; (void)buf; (void)size;
    return -1;
#endif
}

long sys_fsync(long h)
{
#if !FF_FS_READONLY
    if (BAD(h)) return -1;
    return (f_sync(&files[h]) == FR_OK) ? 0 : -1;
#else
    (void)h;
    return -1;
#endif
}

long sys_flseek(long h, long off, long whence)
{
    if (BAD(h)) return -1;
    long base;
    switch (whence) {
    case RIGEL_SEEK_SET: base = 0;                          break;
    case RIGEL_SEEK_CUR: base = (long)f_tell(&files[h]);    break;
    case RIGEL_SEEK_END: base = (long)f_size(&files[h]);    break;
    default: return -1;
    }
    long np = base + off;
    if (np < 0) return -1;
    if (f_lseek(&files[h], (FSIZE_t)np) != FR_OK) return -1;
    return (long)f_tell(&files[h]);       /* arquivo aberto so' p/ leitura: posicao limitada ao tamanho */
}

long sys_fioctl(long h, long cmd, void *arg)
{
    (void)arg;
    if (BAD(h)) return -1;
    switch (cmd) {
#if !FF_FS_READONLY
    case RIGEL_FIOC_TRUNCATE: return (f_truncate(&files[h]) == FR_OK) ? 0 : -1;
    case RIGEL_FIOC_SYNC:     return (f_sync(&files[h])     == FR_OK) ? 0 : -1;
#endif
    default: return -1;
    }
}

long sys_ftell(long h) { return BAD(h) ? -1 : (long)f_tell(&files[h]); }
long sys_fsize(long h) { return BAD(h) ? -1 : (long)f_size(&files[h]); }
long sys_feof (long h) { return BAD(h) ? -1 : (f_eof(&files[h]) ? 1 : 0); }

/* ---------------------------------------------------------------- por nome */

long sys_funlink(const char *path)
{
#if !FF_FS_READONLY && FF_FS_MINIMIZE == 0
    if (path == NULL) return -1;
    return (f_unlink(path) == FR_OK) ? 0 : -1;
#else
    (void)path; return -1;
#endif
}

long sys_frename(const char *oldp, const char *newp)
{
#if !FF_FS_READONLY && FF_FS_MINIMIZE == 0
    if (oldp == NULL || newp == NULL) return -1;
    return (f_rename(oldp, newp) == FR_OK) ? 0 : -1;
#else
    (void)oldp; (void)newp; return -1;
#endif
}

long sys_fmkdir(const char *path)
{
#if !FF_FS_READONLY && FF_FS_MINIMIZE == 0
    if (path == NULL) return -1;
    return (f_mkdir(path) == FR_OK) ? 0 : -1;
#else
    (void)path; return -1;
#endif
}

long sys_fstat(const char *path, rigel_dirent_t *ent)
{
#if FF_FS_MINIMIZE <= 1
    FILINFO fno;
    if (path == NULL || ent == NULL) return -1;
    if (f_stat(path, &fno) != FR_OK) return -1;
    fill_ent(ent, &fno);
    return 0;
#else
    (void)path; (void)ent; return -1;
#endif
}

/* troca os atributos (somente-leitura, oculto, sistema, arquivo) de um arquivo/diretorio.
 * 'attrs' = os bits que o arquivo deve ficar tendo; os 4 bits sao sempre todos substituidos. */
long sys_chmod(const char *path, long attrs)
{
#if FF_USE_CHMOD && !FF_FS_READONLY && FF_FS_MINIMIZE == 0
    const BYTE all = AM_RDO | AM_HID | AM_SYS | AM_ARC | 0xC0;
    if (path == NULL) return -1;
    return (f_chmod(path, (BYTE)(attrs & all), all) == FR_OK) ? 0 : -1;
#else
    (void)path; (void)attrs;
    return -1;                       /* ligue FF_USE_CHMOD no ffconf.h */
#endif
}

/* ---------------------------------------------------------------- volumes */

/* "0:" "1:" ... ; "", "/" ou sem numero = volume 0. Devolve o numero ou -1. */
static int drive_of(const char *path)
{
    int drv = 0;
    if (path == NULL) return -1;
    if (path[0] >= '0' && path[0] <= '9' && path[1] == ':')
        drv = path[0] - '0';
    return (drv < FF_VOLUMES) ? drv : -1;
}

long sys_fmount(const char *path)
{
    int drv = drive_of(path);
    if (drv < 0) return -1;
    char p[3] = { (char)('0' + drv), ':', '\0' };
    return (f_mount(&vols[drv], p, 1) == FR_OK) ? 0 : -1;     /* 1 = monta ja' (confere o cartao) */
}

long sys_funmount(const char *path)
{
    int drv = drive_of(path);
    if (drv <= 0) return -1;                                   /* volume 0 = boot: nunca desmonta */
    char p[3] = { (char)('0' + drv), ':', '\0' };
    return (f_mount(NULL, p, 0) == FR_OK) ? 0 : -1;
}

/* ---------------------------------------------------------------- limpeza */

/* chamada no SYS_EXIT: fecha os arquivos que a task deixou abertos */
void sys_fs_task_exit(int owner)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_used[i] && file_owner[i] == owner) {
            f_close(&files[i]);
            file_used[i] = 0;
        }
    }
}
