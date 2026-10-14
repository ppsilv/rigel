/*
 * exec.c -- roda programas ELF a partir do shell (via SYS_SPAWN + SYS_WAITPID).
 *
 * Regras de busca de process_command_executable (FatFS ignora maiusculas):
 *   - nome com '/'  (ex. "/bin/ls", "apps/x.elf"): usa como esta; se nao existir tenta + ".elf"
 *   - nome simples (ex. "ls", "hello.elf"): tenta, nesta ordem,
 *         /<nome>   /<nome>.elf   /bin/<nome>   /bin/<nome>.elf
 * Para mudar as pastas, edite SEARCH_DIRS abaixo.
 */
#include <stdio.h>
#include <string.h>
#include <rigel/rigel.h>
#include "exec.h"

#define SEARCH_DIRS   { "/", "/bin/" }
#define PATH_MAX_LEN  128

int run_elf(const char *path, int argc, char *argv[])
{
    char *def[2];
    (void)argc;

    if (argv == NULL) {
        def[0] = (char *)path;
        def[1] = NULL;
        argv = def;
    }

    long pid = rigel_spawn((char *)path, argv);
    if (pid <= 0)
        return -1;

    rigel_waitpid(pid);          /* primeiro plano: o shell espera o filho terminar */
    return 0;
}

/* 1 se 'path' existe e NAO e' diretorio */
static int file_exists(const char *path)
{
    rigel_dirent_t e;
    return rigel_stat((char *)path, &e) == 0 && !(e.attr & RIGEL_ATTR_DIR);
}

static int has_elf_ext(const char *s)
{
    size_t n = strlen(s);
    return n >= 4 && strcasecmp(s + n - 4, ".elf") == 0;
}

/* monta dir + nome (+ ".elf") em out; devolve 0 se coube */
static int join(char *out, const char *dir, const char *name, int add_ext)
{
    size_t need = strlen(dir) + strlen(name) + (add_ext ? 4 : 0) + 1;
    if (need > PATH_MAX_LEN) return -1;
    strcpy(out, dir);
    strcat(out, name);
    if (add_ext) strcat(out, ".elf");
    return 0;
}

int process_command_executable(int argc, char *argv[])
{
    static char path[PATH_MAX_LEN];
    static const char *const dirs[] = SEARCH_DIRS;
    const char *name;
    int found = 0;

    if (argc < 1 || argv[0] == NULL || argv[0][0] == '\0')
        return 0;
    name = argv[0];

    if (strchr(name, '/')) {
        if (join(path, "", name, 0) == 0 && file_exists(path))
            found = 1;
        else if (!has_elf_ext(name) && join(path, "", name, 1) == 0 && file_exists(path))
            found = 1;
    } else {
        for (size_t i = 0; i < sizeof dirs / sizeof dirs[0] && !found; i++) {
            if (join(path, dirs[i], name, 0) == 0 && file_exists(path)) { found = 1; break; }
            if (!has_elf_ext(name) && join(path, dirs[i], name, 1) == 0 && file_exists(path)) { found = 1; break; }
        }
    }

    if (!found)
        return 0;                                    /* o shell mostra "Unknown command" */

    if (run_elf(path, argc, argv) != 0)
        printf("%s: nao consegui executar '%s'\n", name, path);
    return 1;
}
