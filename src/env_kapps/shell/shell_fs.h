#ifndef SHELL_FS_H
#define SHELL_FS_H
#include <stddef.h>

/*
 * Comandos do shell (mesma assinatura da g_cmd_table). O shell chama function(argc - 1, argv + 1),
 * entao argv[0] e' o 1o ARGUMENTO (nao o nome do comando) e argc e' quantos argumentos vieram.
 */
void do_cd(int argc, char *argv[]);      /* cd [dir]   (sem argumento: vai para /)       */
void do_pwd(int argc, char *argv[]);     /* pwd                                           */
void do_mkdir(int argc, char *argv[]);   /* mkdir dir [dir ...]                           */
void do_rmdir(int argc, char *argv[]);   /* rmdir dir [dir ...]   (so' diretorio vazio)   */

void shell_cwd_refresh(void);            /* le o diretorio corrente do kernel (1x no inicio) */
const char *shell_cwd(void);             /* "/dir/sub" (em cache, nao vai ao SD)          */
void shell_prompt(char *buf, size_t n);  /* monta "/dir/sub> "                            */
#endif
