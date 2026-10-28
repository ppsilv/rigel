/*
 * cursortest.c -- descobre se o cursor da VGA apaga as letras quando se move.
 *
 *   cursortest
 *
 * Escreve ABCDEFGHIJ na linha 5 e move o cursor para a esquerda, tecla por tecla ('h'),
 * de dois jeitos:
 *   Teste A: esconde o cursor, move, mostra o cursor (o que o editor faz)
 *   Teste B: so' move com rigel_gotoxy, sem esconder nem mostrar
 * Em cada um, anote se alguma letra de ABCDEFGHIJ sumiu e qual.
 * 'q' passa para o proximo teste / sai.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <rigel/rigel.h>

#define LINHA  5
#define COL0   10
#define TXT    "ABCDEFGHIJ"

static int tecla(void)
{
    unsigned char c;
    while (read(0, &c, 1) != 1) { }
    return c;
}

static void texto(int col, int lin, const char *s)
{
    rigel_gotoxy(col, lin);
    write(1, s, strlen(s));
}

static void limpa(void)
{
    int fd = open("/dev/vcon", 0x03);
    if (fd >= 0) { rigel_ioctl(fd, TIOC_CLEAR, NULL); close(fd); }
}

static void teste(int com_hide)
{
    int col = COL0 + 9;                    /* comeca em cima da letra J */

    limpa();
    texto(COL0, LINHA, TXT);
    texto(0, 8,  com_hide ? "TESTE A: esconde, move, mostra" : "TESTE B: so' move (gotoxy)");
    texto(0, 9,  "'h' = cursor 1 coluna para a esquerda     'q' = proximo teste");
    texto(0, 10, "Anote se alguma letra de ABCDEFGHIJ (linha 5) sumiu e qual.");

    rigel_gotoxy(col, LINHA);
    if (com_hide) rigel_cursor(1);
    for (;;) {
        int c = tecla();
        if (c == 'q') break;
        if (c == 'h' && col > COL0) {
            if (com_hide) rigel_cursor(0);
            col--;
            rigel_gotoxy(col, LINHA);
            if (com_hide) rigel_cursor(1);
        }
    }
}

int main(void)
{
    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO));
    teste(1);
    teste(0);
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);
    rigel_cursor(1);
    limpa();
    return 0;
}