/*
 * sys_fscwd.c -- SYS_CHDIR / SYS_GETCWD do kernel Rigel (diretorio corrente do FatFS).
 *
 *   long sys_chdir(const char *path)           0 ou -1   (muda o diretorio corrente)
 *   long sys_getcwd(char *buf, long size)      0 ou -1   (devolve "/dir/sub", sem "0:")
 *
 * Precisa de  #define FF_FS_RPATH 2  no ffconf.h (1 basta para chdir, 2 e' preciso para getcwd).
 * O diretorio corrente e' UM so' para o sistema todo (e' do FatFS, nao da tarefa): o que o
 * shell fizer com cd vale tambem para os programas que ele lancar.
 */
#include <stdint.h>
#include <string.h>
#include <fatfs/ff.h>

#ifndef FF_FS_RPATH
#define FF_FS_RPATH _FS_RPATH
#endif

long sys_chdir(const char *path)
{
#if FF_FS_RPATH >= 1
    if (path == NULL) return -1;
    return (f_chdir(path) == FR_OK) ? 0 : -1;
#else
    (void)path; return -1;
#endif
}

long sys_getcwd(char *buf, long size)
{
#if FF_FS_RPATH >= 2
    if (buf == NULL || size < 3) return -1;
    if (f_getcwd(buf, (UINT)size) != FR_OK) return -1;
    /* o FatFS devolve "0:/dir": tira o prefixo do volume */
    if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == ':')
        memmove(buf, buf + 2, strlen(buf + 2) + 1);
    if (buf[0] == '\0') { buf[0] = '/'; buf[1] = '\0'; }
    return 0;
#else
    (void)buf; (void)size; return -1;
#endif
}
