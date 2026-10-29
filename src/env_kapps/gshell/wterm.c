/*
 * wterm.c -- terminal em janela para o Rigel: roda o shell DENTRO de uma janela.
 *
 * Mesma ideia do telnetd, mas no lugar do socket ficam o teclado e a tela:
 *   teclado (stdin deste app) --> pty --> stdin do shell
 *   stdout do shell --> pty --> grade de caracteres na RAM --> desenhada na janela
 *
 * Para sair: digite  exit  no shell (o shell termina, o pty fecha, o wterm sai).
 *
 * Sem printf depois de iniciar (o stdout deste app e' a tela inteira).
 * Usa so' syscalls da librigel (rigel_fillrect, rigel_gotoxy, ...), sem MMIO direto.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>

/* ---- ajuste aqui ------------------------------------------------------- */
#define COLS        60          /* colunas de texto da janela            */
#define ROWS        20          /* linhas de texto da janela             */
#define WIN_COL     6           /* coluna (celula de 8 px) do texto      */
#define WIN_ROW     3           /* linha  (celula de 16 px) do texto     */
#define SHELL_PATH  "/bin/shell.elf"
#define TITLE       "rigelOS Terminal"

/* cores 0..15 da PicoVGA: confira com o seu colors.h (BLACK, BLUE, ...) */
#define COR_FUNDO     0         /* fundo do texto (BLACK) */
#define COR_TEXTO    15         /* texto (WHITE)          */
#define COR_BARRA     1         /* barra de titulo (BLUE) */
#define COR_TITULO   15         /* texto do titulo        */
#define COR_BORDA    15         /* contorno da janela     */
/* ------------------------------------------------------------------------ */

#define CHAR_W 8
#define CHAR_H 16
#define TX (WIN_COL * CHAR_W)               /* pixel x do canto do texto */
#define TY (WIN_ROW * CHAR_H)               /* pixel y do canto do texto */
#define TW (COLS * CHAR_W)
#define TH (ROWS * CHAR_H)

#define CUR_FLAG 0x100                      /* celula com cursor (cores invertidas) */

static uint8_t  grid[ROWS][COLS];           /* o que o shell escreveu             */
static uint16_t shown[ROWS][COLS];          /* o que esta' desenhado na tela      */
static int cx, cy;                          /* cursor na grade                    */

/* ------------------------------------------------------------ grade */
static void grid_clear_row(int r)
{
    memset(grid[r], ' ', COLS);
}

static void grid_init(void)
{
    int r, c;
    for (r = 0; r < ROWS; r++) {
        grid_clear_row(r);
        for (c = 0; c < COLS; c++)
            shown[r][c] = 0xFFFF;           /* forca o primeiro desenho completo */
    }
    cx = cy = 0;
}

static void grid_newline(void)
{
    cx = 0;
    if (++cy >= ROWS) {                     /* rola uma linha */
        memmove(grid[0], grid[1], (ROWS - 1) * COLS);
        grid_clear_row(ROWS - 1);
        cy = ROWS - 1;
    }
}

static void grid_put(uint8_t c)
{
    switch (c) {
    case '\n': grid_newline();              break;   /* o shell manda so' \n: vale CR+LF */
    case '\r': cx = 0;                      break;
    case '\b': if (cx > 0) cx--;            break;   /* "\b \b" apaga: o espaco sobrescreve */
    case '\t': do { grid_put(' '); } while (cx % 8); break;
    case 0x0C: {                                     /* form feed: limpa a janela */
        int r;
        for (r = 0; r < ROWS; r++) grid_clear_row(r);
        cx = cy = 0;
        break;
    }
    default:
        if (c < 0x20) break;                         /* ignora outros controles */
        if (c > 0x7E) c = '.';
        if (cx >= COLS) grid_newline();
        grid[cy][cx++] = c;
        break;
    }
}

/* ------------------------------------------------------------ desenho */
static void draw_frame(void)
{
    long x = TX - 1, y = TY - CHAR_H - 1, w = TW + 2, h = TH + CHAR_H + 2;
    int i;

    rigel_fillrect(TX, TY, TW, TH, COR_FUNDO);
    rigel_fillrect(TX, TY - CHAR_H, TW, CHAR_H, COR_BARRA);
    rigel_drawhline(x, y,         w, COR_BORDA);
    rigel_drawhline(x, y + h - 1, w, COR_BORDA);
    rigel_drawvline(x,         y, h, COR_BORDA);
    rigel_drawvline(x + w - 1, y, h, COR_BORDA);

    rigel_settextcolor(COR_TITULO, COR_BARRA);
    rigel_gotoxy(WIN_COL + 1, WIN_ROW - 1);
    for (i = 0; TITLE[i]; i++)
        rigel_vga_putchar(TITLE[i]);
}

/* desenha so' as celulas que mudaram; celulas vizinhas com a mesma cor saem numa corrida
 * (um gotoxy e varios putchar: o cursor do Pico avanca sozinho) */
static void render(void)
{
    int r, c, c2;

    for (r = 0; r < ROWS; r++) {
        c = 0;
        while (c < COLS) {
            uint16_t want = grid[r][c] | ((r == cy && c == cx) ? CUR_FLAG : 0);
            if (shown[r][c] == want) { c++; continue; }

            /* inicio de uma corrida: mesma cor (cursor ou normal) */
            if (want & CUR_FLAG) rigel_settextcolor(COR_FUNDO, COR_TEXTO);
            else                 rigel_settextcolor(COR_TEXTO, COR_FUNDO);
            rigel_gotoxy(WIN_COL + c, WIN_ROW + r);

            c2 = c;
            while (c2 < COLS) {
                uint16_t w2 = grid[r][c2] | ((r == cy && c2 == cx) ? CUR_FLAG : 0);
                if (shown[r][c2] == w2 || (w2 & CUR_FLAG) != (want & CUR_FLAG))
                    break;
                rigel_vga_putchar(grid[r][c2]);
                shown[r][c2] = w2;
                c2++;
            }
            c = c2;
        }
    }
}

/* ------------------------------------------------------------ main */
int main(void)
{
    int pty[2];
    long pid;
    char *argv[2];
    uint8_t kb[32], tx[256];

    long rc = rigel_openpty(pty);
    if (rc < 0) {
        printf("wterm: sem pty (rc=%ld)\n", rc);
        return 1;
    }

    argv[0] = (char *)SHELL_PATH;
    argv[1] = NULL;
    pid = rigel_spawn_io(SHELL_PATH, argv, pty[1], pty[1], pty[1]);
    close(pty[1]);                          /* so' o shell segura a ponta dele */
    if (pid < 0) {
        printf("wterm: nao consegui iniciar o shell\n");
        close(pty[0]);
        return 1;
    }

    /* teclado: bytes crus (setas 0x10..0x13), sem eco, sem bloquear */
    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO | TTY_NONBLOCK));
    rigel_cursor(0);                        /* esconde o cursor do Pico; a janela tem o seu */

    grid_init();
    draw_frame();
    render();

    for (;;) {
        int n, busy = 0;

        /* 1) teclado -> shell */
        n = read(0, kb, sizeof kb);
        if (n > 0) {
            int off = 0;
            busy = 1;
            while (off < n) {
                int w = write(pty[0], kb + off, n - off);
                if (w < 0)  goto fim;       /* o shell saiu */
                if (w == 0) break;          /* pty cheio: descarta o resto */
                off += w;
            }
        }

        /* 2) shell -> grade */
        for (;;) {
            int i;
            n = read(pty[0], tx, sizeof tx);
            if (n < 0)  goto fim;           /* o shell terminou */
            if (n == 0) break;
            busy = 1;
            for (i = 0; i < n; i++)
                grid_put(tx[i]);
        }

        /* 3) tela */
        render();

        if (!busy)
            rigel_yield();
    }

fim:
    close(pty[0]);
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);
    rigel_cursor(1);
    rigel_fillrect(TX - 1, TY - CHAR_H - 1, TW + 2, TH + CHAR_H + 2, COR_FUNDO);
    rigel_gotoxy(0, 0);
    return 0;
}
