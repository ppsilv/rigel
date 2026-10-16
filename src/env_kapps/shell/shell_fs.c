/*
 * shell_fs.c -- cd, pwd, mkdir, rmdir e prompt com o diretorio corrente, para o shell Rigel.
 *
 * O diretorio corrente fica em cache (cwd): o f_getcwd do FatFS varre diretorios no SD para
 * montar o nome, entao so' e' chamado no inicio e depois de cada cd bem-sucedido.
 * Precisa de FF_FS_RPATH = 2 no ffconf.h e das syscalls rigel_chdir / rigel_getcwd.
 */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>        /* mkdir */
#include <unistd.h>          /* unlink */
#include <rigel/rigel.h>     /* rigel_chdir, rigel_getcwd, rigel_stat, rigel_dirent_t, RIGEL_ATTR_DIR */
#include "shell_fs.h"

static char cwd[128] = "/";

void shell_cwd_refresh(void)
{
    char tmp[128];
    if (rigel_getcwd(tmp, sizeof tmp) == 0)
        strcpy(cwd, tmp);
}

const char *shell_cwd(void) { return cwd; }

void shell_prompt(char *buf, size_t n)
{
    snprintf(buf, n, "%s> ", cwd);
}

void do_pwd(int argc, char *argv[])
{
    (void)argc; (void)argv;
    shell_cwd_refresh();
    printf("%s\n", cwd);
}

void do_cd(int argc, char *argv[])
{
    const char *alvo = (argc >= 1) ? argv[0] : "/";
    rigel_dirent_t e;

    /* "/", "." e ".." sempre existem; o resto confere se e' diretorio (mensagem mais clara) */
    if (strcmp(alvo, "/") != 0 && strcmp(alvo, ".") != 0 && strcmp(alvo, "..") != 0) {
        if (rigel_stat((char *)alvo, &e) != 0)  { printf("cd: %s: nao existe\n", alvo); return; }
        if (!(e.attr & RIGEL_ATTR_DIR))         { printf("cd: %s: nao e' diretorio\n", alvo); return; }
    }
    if (rigel_chdir((char *)alvo) != 0) { printf("cd: %s: falhou\n", alvo); return; }
    shell_cwd_refresh();
}

void do_mkdir(int argc, char *argv[])
{
    int i;
    for (i = 0; i < argc; i++) {
        if (mkdir(argv[i], 0777) != 0)
            printf("mkdir: %s: nao foi possivel criar (ja existe ou caminho invalido)\n", argv[i]);
    }
}

void do_rmdir(int argc, char *argv[])
{
    rigel_dirent_t e;
    int i;
    for (i = 0; i < argc; i++) {
        if (rigel_stat(argv[i], &e) != 0) { printf("rmdir: %s: nao existe\n", argv[i]); continue; }
        if (!(e.attr & RIGEL_ATTR_DIR))   { printf("rmdir: %s: nao e' diretorio\n", argv[i]); continue; }
        /* o f_unlink do FatFS so' apaga diretorio VAZIO */
        if (unlink(argv[i]) != 0)
            printf("rmdir: %s: nao esta vazio ou e' somente leitura\n", argv[i]);
    }
}
