/*
 * shell_history.c -- leitura de linha com historico (UP/DOWN) para o shell Rigel.
 *
 * UP (0x10) / DOWN (0x11) trocam o comando NA MESMA LINHA, sem '\n'.
 * Uso:
 *   char cmdline[LINE_MAX];
 *   for (;;) {
 *       fputs("rigel> ", stdout); fflush(stdout);
 *       readline_with_history(cmdline);
 *       history_add(cmdline);
 *       executa_comando(cmdline);
 *   }
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>      /* rigel_ioctl, TIOC_SETMODE, TTY_RAW, TTY_NOECHO */

#define HIST_SIZE   16        /* quantos comandos guardar */
#undef  LINE_MAX
#define LINE_MAX    128       /* tamanho maximo de uma linha */

#define KEY_UP     0x10
#define KEY_DOWN   0x11
#define KEY_BS     0x08       /* mude para 0x7F se seu teclado mandar DEL */
#define KEY_ENTER  '\r'

static char history[HIST_SIZE][LINE_MAX];
static int  hist_count  = 0;  /* total de comandos ja guardados */
static int  hist_cursor = -1; /* -1 = linha nova (fora do historico) */

/* Apaga 'old_len' caracteres da linha e escreve 'text' no lugar.
 * fputs (e NAO puts) para nao emitir '\n'. */
static void redraw_line(int old_len, const char *text)
{
    for (int i = 0; i < old_len; i++) putchar('\b');
    for (int i = 0; i < old_len; i++) putchar(' ');
    for (int i = 0; i < old_len; i++) putchar('\b');
    fputs(text, stdout);
    fflush(stdout);
}

void history_add(const char *cmd)
{
    if (cmd[0] == '\0')
        return;

    strncpy(history[hist_count % HIST_SIZE], cmd, LINE_MAX - 1);
    history[hist_count % HIST_SIZE][LINE_MAX - 1] = '\0';
    hist_count++;
    hist_cursor = -1;
}

/* Le uma linha (Enter termina). 'buf' precisa ter LINE_MAX bytes. */
void readline_with_history(char *buf)
{
    int len = 0;
    unsigned char c;
    buf[0] = '\0';
    hist_cursor = -1;

    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO));

    for (;;) {
        if (read(0, &c, 1) != 1)
            continue;

        if (c == KEY_ENTER || c == '\n') {
            putchar('\n');
            fflush(stdout);
            buf[len] = '\0';
            rigel_ioctl(0, TIOC_SETMODE, (void *)0);   /* restaura modo normal */
            return;
        }

        if (c == KEY_BS || c == 0x7F) {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                putchar('\b'); putchar(' '); putchar('\b');
                fflush(stdout);
            }
            continue;
        }

        if (c == KEY_UP) {
            if (hist_cursor + 1 < hist_count && hist_cursor + 1 < HIST_SIZE) {
                hist_cursor++;
                int idx = (hist_count - 1 - hist_cursor + HIST_SIZE) % HIST_SIZE;
                redraw_line(len, history[idx]);
                strcpy(buf, history[idx]);
                len = (int)strlen(buf);
            }
            continue;
        }

        if (c == KEY_DOWN) {
            if (hist_cursor > 0) {
                hist_cursor--;
                int idx = (hist_count - 1 - hist_cursor + HIST_SIZE) % HIST_SIZE;
                redraw_line(len, history[idx]);
                strcpy(buf, history[idx]);
                len = (int)strlen(buf);
            } else if (hist_cursor == 0) {
                hist_cursor = -1;
                redraw_line(len, "");
                buf[0] = '\0';
                len = 0;
            }
            continue;
        }

        /* caractere normal: ecoa so' se imprimivel e couber no buffer */
        if (c >= 0x20 && len < LINE_MAX - 1) {
            buf[len++] = (char)c;
            buf[len] = '\0';
            putchar(c);
            fflush(stdout);
        }
    }
}