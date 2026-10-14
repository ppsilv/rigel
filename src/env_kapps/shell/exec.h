#ifndef EXEC_H
#define EXEC_H

/*
 * Roda um programa ELF e ESPERA ele terminar (primeiro plano).
 *   path : caminho exato do arquivo, ex. "/bin/ls.elf"
 *   argv : vetor terminado em NULL (argv[0] = nome do programa); pode ser NULL
 * Retorna 0 se o programa rodou ate' o fim, -1 se nao conseguiu iniciar.
 */
int run_elf(const char *path, int argc, char *argv[]);

/*
 * Para o shell: procura um programa com o nome digitado e roda.
 * Chamada com os mesmos argc/argv do process_command_builtin.
 * Retorna 1 se achou o arquivo (rodou ou tentou rodar), 0 se nao existe nenhum programa com esse nome.
 */
int process_command_executable(int argc, char *argv[]);

#endif
