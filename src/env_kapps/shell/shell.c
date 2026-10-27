/*
 * shell.c -- shell do Rigel
 *
 * Novidades desta versao (trazidas do shell v0 do Orion):
 *   - redirecionamento:   cmd > arq      grava (apaga o conteudo anterior)
 *                         cmd >> arq     acrescenta no fim
 *                         cmd < arq      o stdin do comando vem do arquivo
 *                         (valem para comandos internos E para programas .elf)
 *   - aspas:              echo "a  b"  'c d'     (agrupam palavras; > e < dentro de aspas sao texto)
 *   - comentarios:        linha que comeca com #
 *   - script de boot:     /boot.bat e' executado ao iniciar (um comando por linha, # = comentario)
 *   - comandos novos:     echo [-n] ..., clear, meminfo, exit
 *
 * O redirecionamento usa SYS_DUP (rigel_dup = dup2): ver patch_dup.py.
 * Os fds 253 e 254 sao reservados para o shell guardar o stdin/stdout originais.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <rigel/rigel.h>
#include "shell.h"


#define MAXARGS 40
//#define DEBUG_ELF 1

/* flags do open() do VFS do Rigel (NAO sao os do newlib) */
#define SH_O_RDONLY   0x01
#define SH_O_WRONLY   0x02
#define SH_O_RDWR     0x03
#define SH_O_TRUNC    0x08
#define SH_O_APPEND   0x10

/* fds reservados: copia do stdin/stdout originais enquanto ha redirecionamento */
#define SH_FD_SAVE_IN    30
#define SH_FD_SAVE_OUT   31

extern int process_command_executable( int argc, char *argv[]);

void printerro(int eno);
static void redir_restore(int saved_in, int saved_out);


extern void do_help(int argc, char *argv[]);
extern void do_load(int argc, char *argv[]);
extern void do_dump(int argc, char *argv[]);
extern void do_cat(int argc, char *argv[]);
extern void do_uptime(int argc, char *argv[]);
extern void do_ls(int argc, char *argv[]);
extern void do_cd(int argc, char *argv[]);
extern void do_mkdir(int argc, char *argv[]);
extern void do_rmdir(int argc, char *argv[]);
extern void do_pwd(int argc, char *argv[]);
extern void shell_cwd_refresh(void);
extern void shell_prompt(char *buf, size_t n);
extern void do_time(int argc, char *argv[]);
extern void do_shst(int argc, char *argv[]);
extern void do_manual(int argc, char *argv[]);

/* ------------------------------------------------------------------ comandos novos */
static int shell_quit;                       /* 1 = o comando exit foi digitado */

static void do_echo(int argc, char *argv[])
{
    int i = 0, nl = 1;

    if (argc > 0 && strcmp(argv[0], "-n") == 0) {   /* -n: sem o \n no fim */
        nl = 0;
        i = 1;
    }
    for (; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    if (nl) printf("\n");
}

static void do_clear(int argc, char *argv[])
{
    static const char *const devs[] = { "/dev/vcon", "/dev/tty" };
    int i, fd;
    (void)argc; (void)argv;

    fflush(stdout);
    for (i = 0; i < 2; i++) {               /* abre o proprio video: funciona mesmo com stdout redirecionado */
        fd = open(devs[i], SH_O_RDWR);
        if (fd >= 0) {
            rigel_ioctl(fd, TIOC_CLEAR, NULL);
            close(fd);
            return;
        }
    }
    printf("clear: nao achei o dispositivo de video\n");
}

static void do_meminfo(int argc, char *argv[])
{
    char buf[128];
    int  fd, n;
    (void)argc; (void)argv;

    fd = open("/proc/meminfo", SH_O_RDONLY);
    if (fd < 0) {
        printf("meminfo: /proc/meminfo nao disponivel\n");
        return;
    }
    while ((n = read(fd, buf, sizeof buf)) > 0)
        printf("%.*s", n, buf);
    close(fd);
}

static void do_exit(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("Saindo do shell...\n");
    shell_quit = 1;
}

const cmd_entry_t g_cmd_table[] = {
    //    {"loadmem",	    1,  2, &do_loadmem,	    "Load a file to the specified memory location <file> <mem>" },
    {"cd",          0,  1, &do_cd,          "Change directory [dir] (no argument: go to /)" },
    {"pwd",         0,  0, &do_pwd,         "Print the current directory" },
    {"mkdir",       1,  0, &do_mkdir,       "Create folder(s) <dir> [dir ...]" },
    {"rmdir",       1,  0, &do_rmdir,       "Delete empty folder(s) <dir> [dir ...]" },
    //    {"save",        3,  3, &do_save,        "Saves file from <ADDR> to disk save <filename> <filesize>" },
    {"shst",        0,  0, &do_shst,        "Show systemtick" },
    {"time",        0,  6, &do_time,        "Display or set the current time and date (time ? for help)" },
    {"man",        0,  6, &do_manual,       "Show manual for shell redirections." },
    //    {"writemem",    2,  0, &do_writemem,    "Write memory <addr> [byte ...]" },
    //    {"writemem1",   2,  0, &do_writemem1,   "Write a memory location" },

    {"ls",          0,  1, &do_ls,	        "List directory contents" },
    {"uptime",      0,  0, &do_uptime,      "Display the time the system has been running" },
    {"cat",         1,  1, &do_cat,         "Display contents of <file>"},
    {"dump",        2,  2, &do_dump,        "Dump memory <from> <count>" },
    {"load",	    0,  0, &do_load,	    "Load a fixed program hello.elf" },
    {"echo",        0,  0, &do_echo,        "Print the arguments [-n]; use > file or >> file to write a file" },
    {"clear",       0,  0, &do_clear,       "Clear the screen" },
    {"meminfo",     0,  0, &do_meminfo,     "Show memory information (/proc/meminfo)" },
    {"exit",        0,  0, &do_exit,        "Leave the shell" },
    {"help",	    0,  0, &do_help,	    "Show available commands" },

    {0, 0, 0, 0, 0 }
};

int process_command_builtin( int argc, char *argv[]){
    const cmd_entry_t *cmd;
//    FRESULT fr;
#ifdef DEBUG_ELF  
    printf("process_command_builtin\n");
#endif
//    if (argc == 1 && argv[0][1] == ':')
//    {
//        // Change current drive
//        fr = f_chdrive(argv[0]);
//        if (fr)
//        {
//        	printerro(fr);
//        	printf("Drive %s not found\n", argv[0]);
//        }
//        return 1;
//    }
//    else
    {
        // built-in commands
        for (cmd = g_cmd_table; cmd->name; cmd++)
        {
            if (!strcasecmp(argv[0], cmd->name))
            {
                if ((argc - 1) >= cmd->min_args && (cmd->max_args == 0 || (argc - 1) <= cmd->max_args))
                {
                    cmd->function(argc - 1, argv + 1);
                }
                else
                {
                    if (cmd->min_args == cmd->max_args)
                        printf("%s: requires %d argument%s\n", argv[0], cmd->min_args, cmd->min_args == 1 ? "" : "s");
                    else
                        printf("%s: requires %d to %d arguments\n", argv[0], cmd->min_args, cmd->max_args);
                }
                return 1;
            }
        }
    }
    return 0;
}

void process_command(int argc, char *argv[])
{
    if (argc > 0)
    {
        if (process_command_builtin(argc, argv)){
            return;
        }else if (process_command_executable(argc, argv)){
            return;
        }else{
            fprintf(stderr, "Unknown command: [%s] Try 'help'.\n", argv[0]);   /* stderr: aparece na tela mesmo com '>' */
        }
    }
}

/* ------------------------------------------------------------------ redirecionamento */
typedef struct {
    char *in_file;          /* cmd < arq  */
    char *out_file;         /* cmd > arq  ou  cmd >> arq */
    int   append;           /* 1 = >> */
} redir_t;

/*
 * Quebra a linha em argumentos (no proprio buffer) e separa os redirecionamentos.
 * Aceita aspas simples e duplas e os operadores colados ou separados (cmd>arq, cmd > arq).
 * Devolve argc, ou -1 se ha erro de sintaxe (ja avisou).
 */
static int parse_line(char *line, char *args[], redir_t *rd)
{
    char *r = line, *w = line, *tok = NULL;
    char  quote = 0, c;
    int   argc = 0, in_tok = 0, pending = 0;     /* pending: 1 '<', 2 '>', 3 '>>' (falta o nome do arquivo) */

    rd->in_file = NULL;
    rd->out_file = NULL;
    rd->append = 0;

    for (;; r++) {
        c = *r;

        if (quote) {                                  /* dentro de aspas: tudo e' texto */
            if (c == quote)       quote = 0;
            else if (c == '\0') { printf("shell: aspas sem fechar\n"); return -1; }
            else                  *w++ = c;
            continue;
        }

        if (c == '\0' || isspace((unsigned char)c) || c == '<' || c == '>') {
            if (in_tok) {                             /* fecha a palavra atual */
                *w++ = '\0';
                in_tok = 0;
                if (pending) {
                    if (pending == 1) rd->in_file = tok;
                    else { rd->out_file = tok; rd->append = (pending == 3); }
                    pending = 0;
                } else if (argc < MAXARGS) {
                    args[argc++] = tok;
                } else {
                    printf("Limiting to %d arguments.\n", MAXARGS);
                }
            }
            if (c == '\0') break;
            if (c == '<' || c == '>') {
                if (pending) { printf("shell: erro de sintaxe perto de '%c'\n", c); return -1; }
                if (c == '<') {
                    if (r[1] == '<') { printf("shell: heredoc (<<) nao implementado\n"); return -1; }
                    pending = 1;
                } else if (r[1] == '>') {
                    pending = 3;
                    r++;
                } else {
                    pending = 2;
                }
            }
            continue;
        }

        if (!in_tok) { tok = w; in_tok = 1; }
        if (c == '"' || c == '\'') quote = c;         /* abre aspas (a aspa em si nao vai para a palavra) */
        else                       *w++ = c;
    }

    if (pending) { printf("shell: falta o nome do arquivo depois de < ou >\n"); return -1; }
    args[argc] = 0;
    return argc;
}

static int open_out(const char *path, int append)
{
    int fd;
    rigel_dirent_t e;

    if (!append)
        return open(path, SH_O_WRONLY | SH_O_TRUNC);       /* cria ou apaga o conteudo */

    fd = open(path, SH_O_WRONLY | SH_O_APPEND);
    if (fd < 0 && rigel_stat((char *)path, &e) != 0)        /* nao existia: cria */
        fd = open(path, SH_O_WRONLY | SH_O_TRUNC);
    return fd;
}

/* liga os redirecionamentos; 0 = ok, -1 = erro (nada fica alterado) */
static int redir_apply(const redir_t *rd, int *saved_in, int *saved_out)
{
    int fin = -1, fout = -1;

    *saved_in = *saved_out = 0;
    if (!rd->in_file && !rd->out_file) return 0;

    if (rigel_dup(1, 1) < 0) {                              /* o kernel nao tem SYS_DUP? */
        printf("shell: redirecionamento precisa do SYS_DUP no kernel (aplique o patch_dup.py)\n");
        return -1;
    }
    fflush(stdout);

    if (rd->in_file) {
        fin = open(rd->in_file, SH_O_RDONLY);
        if (fin < 0) { printf("shell: nao consegui abrir a entrada '%s'\n", rd->in_file); return -1; }
    }
    if (rd->out_file) {
        fout = open_out(rd->out_file, rd->append);
        if (fout < 0) {
            printf("shell: nao consegui abrir a saida '%s'\n", rd->out_file);
            if (fin >= 0) close(fin);
            return -1;
        }
    }

    if (fin >= 0) {
        if (rigel_dup(0, SH_FD_SAVE_IN) < 0) {
            fprintf(stderr, "shell: dup do stdin falhou\n");
            close(fin);
            if (fout >= 0) close(fout);
            return -1;
        }
        rigel_dup(fin, 0);                                  /* stdin passa a ser o arquivo */
        close(fin);
        *saved_in = 1;
    }
    if (fout >= 0) {
        if (rigel_dup(1, SH_FD_SAVE_OUT) < 0) {
            fprintf(stderr, "shell: dup do stdout falhou\n");
            close(fout);
            if (*saved_in) redir_restore(1, 0);             /* desfaz o stdin que ja tinha trocado */
            return -1;
        }
        rigel_dup(fout, 1);
        close(fout);
        *saved_out = 1;
    }
    return 0;
}

/* desfaz os redirecionamentos (o arquivo de saida e' fechado/gravado aqui) */
static void redir_restore(int saved_in, int saved_out)
{
    if (saved_out) {
        fflush(stdout);                                     /* o que ficou no buffer vai para o arquivo */
        rigel_dup(SH_FD_SAVE_OUT, 1);
        close(SH_FD_SAVE_OUT);
    }
    if (saved_in) {
        rigel_dup(SH_FD_SAVE_IN, 0);
        close(SH_FD_SAVE_IN);
    }
}

/* devolve 0 so' quando a linha pede para sair do shell (linha que comeca com ESC) */
int execute_cmd(char *linebuffer){
    char *args[MAXARGS+1];
    redir_t rd;
    char *p;
    int numargs, saved_in, saved_out;

    if( linebuffer[0] == 0x1b){
        return 0;
    }
    p = linebuffer;
    while (isspace((unsigned char)*p))
        p++;
    if (*p == '#')                                  /* comentario */
        return 1;

    numargs = parse_line(linebuffer, args, &rd);
    if (numargs <= 0)                               /* linha vazia ou erro de sintaxe */
        return 1;

    if (redir_apply(&rd, &saved_in, &saved_out) != 0)
        return 1;
    process_command(numargs, args);
    redir_restore(saved_in, saved_out);
    return 1;
}

/* ------------------------------------------------------------------ script de boot */
static void boot_line(char *line)
{
    char *p = line;

    while (isspace((unsigned char)*p))
        p++;
    if (*p == '\0' || *p == '#')
        return;
    printf("[boot] %s\n", p);
    execute_cmd(p);
}

/* executa /boot.bat: um comando por linha; linhas vazias e que comecam com # sao ignoradas */
static void run_boot_script(void)
{
    char line[128], chunk[64];
    int  fd, n, i, pos = 0;

    fd = open("/boot.bat", SH_O_RDONLY);
    if (fd < 0)
        return;                                     /* nao existe: tudo bem */

    printf("\n=== Executando /boot.bat ===\n");
    while (!shell_quit && (n = read(fd, chunk, sizeof chunk)) > 0) {
        for (i = 0; i < n && !shell_quit; i++) {
            if (chunk[i] == '\n' || chunk[i] == '\r') {
                line[pos] = '\0';
                pos = 0;
                boot_line(line);
            } else if (pos < (int)sizeof line - 1) {
                line[pos++] = chunk[i];
            }
        }
    }
    if (pos > 0 && !shell_quit) {                   /* ultima linha sem \n */
        line[pos] = '\0';
        boot_line(line);
    }
    close(fd);
    printf("=== Script de boot finalizado ===\n\n");
}
 
extern void history_add(const char *cmd);
extern void readline_with_history(char *buf);

int main(int argc, char *argv[]){
    char buffer[128];
    char prompt[160];
    printf("\nSHELL V2 october/18 2026\n");
    int ret;
    shell_cwd_refresh();
    run_boot_script();
    while(!shell_quit){    
        shell_prompt(prompt, sizeof prompt);   /* "/dir/sub> " */
        printf("%s", prompt);
        fflush(stdout);
        readline_with_history(buffer);
        history_add(buffer); 
        ret = execute_cmd(buffer);
        if(ret == 0){
            return 0;
        }
    }
    printf("Shell finalizado.\n");
    return 0;
}
