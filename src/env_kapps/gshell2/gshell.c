/*
 * gshell.c -- varias janelas de terminal (cada uma com o seu shell) num so' programa.
 *
 *   Ctrl-T (0xB4)  troca a janela ativa (so' ela recebe o teclado)
 *   Ctrl-X (0xB8)  fecha o gshell (mata os shells das janelas)
 *   exit           num shell: a janela inicia um shell novo
 *
 * Exemplo de uso: na janela 1 rode  telnetd  (as mensagens dele aparecem ali),
 * Ctrl-T, e continue trabalhando na janela 2.
 *
 * Cada janela: pty + shell (rigel_openpty / rigel_spawn_io, como o telnetd) e uma grade de
 * caracteres na RAM; so' as celulas que mudam sao redesenhadas.
 * Sem printf depois de iniciar (o stdout deste app e' a tela inteira).
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>

/* ---- ajuste aqui ------------------------------------------------------- */
#define NTERM        2          /* numero de janelas (empilhadas na tela) */
#define COLS         76         /* colunas de texto por janela            */
#define ROWS         12         /* linhas de texto por janela             */
#define WIN_COL      2          /* coluna (celula de 8 px) do texto       */
#define FIRST_ROW    2          /* linha (celula de 16 px) do 1o texto; a barra fica 1 acima */
#define ROW_STEP     14         /* distancia em linhas entre uma janela e a proxima */
#define SHELL_PATH   "/bin/shell.elf"

#define KEY_SWITCH   0xB4       /* Ctrl-T (neste teclado: Ctrl+letra = letra minuscula + 0x40) */
#define KEY_QUIT     0xB8       /* Ctrl-X (0x78 + 0x40) */
#define KEY_DEBUG    0          /* 1 = mostra na ultima linha da tela o codigo de cada tecla */

/* cores 0..15 da PicoVGA: confira com o seu colors.h */
#define COR_FUNDO       0
#define COR_TEXTO      15
#define COR_BARRA       1       /* barra da janela ativa (BLUE) */
#define COR_BARRA_OFF   8       /* barra da janela inativa (cinza?) */
#define COR_TITULO     15
#define COR_BORDA      15
/* ------------------------------------------------------------------------ */

#define CHAR_W 8
#define CHAR_H 16
#define TW (COLS * CHAR_W)
#define TH (ROWS * CHAR_H)
#define CUR_FLAG 0x100          /* celula com cursor (cores invertidas) */

typedef struct {
    int      row;               /* linha da primeira linha de texto */
    char     title[16];
    uint8_t  grid[ROWS][COLS];
    uint16_t shown[ROWS][COLS];
    int      cx, cy;
    int      pty;               /* master */
    long     pid;
    int      alive;
} term_t;

static term_t T[NTERM];
static int    cur;              /* janela ativa */

/* ------------------------------------------------------------ debug de teclas */
#if KEY_DEBUG
static void show_key(unsigned k)
{
    static const char hx[] = "0123456789ABCDEF";
    const char *m = "tecla 0x";
    int i;

    rigel_settextcolor(COR_TEXTO, COR_FUNDO);
    rigel_gotoxy(0, 29);                /* ultima linha de texto da tela (480 / 16 = 30 linhas) */
    for (i = 0; m[i]; i++)
        rigel_vga_putchar(m[i]);
    rigel_vga_putchar(hx[(k >> 4) & 15]);
    rigel_vga_putchar(hx[k & 15]);
    rigel_vga_putchar(' ');
    rigel_vga_putchar(' ');
}
#endif

/* ------------------------------------------------------------ grade */
static void clear_row(term_t *t, int r) { memset(t->grid[r], ' ', COLS); }

static void newline(term_t *t)
{
    t->cx = 0;
    if (++t->cy >= ROWS) {
        memmove(t->grid[0], t->grid[1], (ROWS - 1) * COLS);
        clear_row(t, ROWS - 1);
        t->cy = ROWS - 1;
    }
}

static void put(term_t *t, uint8_t c)
{
    switch (c) {
    case '\n': newline(t);                       break;  /* o shell manda so' \n: CR+LF */
    case '\r': t->cx = 0;                        break;
    case '\b': if (t->cx > 0) t->cx--;           break;
    case '\t': do { put(t, ' '); } while (t->cx % 8); break;
    case 0x0C: {
        int r;
        for (r = 0; r < ROWS; r++) clear_row(t, r);
        t->cx = t->cy = 0;
        break;
    }
    default:
        if (c < 0x20) break;
        if (c > 0x7E) c = '.';
        if (t->cx >= COLS) newline(t);
        t->grid[t->cy][t->cx++] = c;
        break;
    }
}

static void puts_grid(term_t *t, const char *s) { while (*s) put(t, (uint8_t)*s++); }

/* ------------------------------------------------------------ desenho */
static void draw_title(const term_t *t, int active)
{
    long tx = WIN_COL * CHAR_W, ty = t->row * CHAR_H;
    uint8_t bar = active ? COR_BARRA : COR_BARRA_OFF;
    int i, n = 0;

    rigel_fillrect(tx, ty - CHAR_H, TW, CHAR_H, bar);
    rigel_settextcolor(COR_TITULO, bar);
    rigel_gotoxy(WIN_COL + 1, t->row - 1);
    for (i = 0; t->title[i]; i++, n++)
        rigel_vga_putchar(t->title[i]);
    (void)n;
}

static void draw_frame(const term_t *t, int active)
{
    long tx = WIN_COL * CHAR_W, ty = t->row * CHAR_H;
    long x = tx - 1, y = ty - CHAR_H - 1, w = TW + 2, h = TH + CHAR_H + 2;

    rigel_fillrect(tx, ty, TW, TH, COR_FUNDO);
    rigel_drawhline(x, y,         w, COR_BORDA);
    rigel_drawhline(x, y + h - 1, w, COR_BORDA);
    rigel_drawvline(x,         y, h, COR_BORDA);
    rigel_drawvline(x + w - 1, y, h, COR_BORDA);
    draw_title(t, active);
}

/* redesenha so' as celulas que mudaram (corridas de mesma cor: um gotoxy + varios putchar) */
static void render(term_t *t, int active)
{
    int r, c, c2;

    for (r = 0; r < ROWS; r++) {
        c = 0;
        while (c < COLS) {
            uint16_t want = t->grid[r][c] | ((active && r == t->cy && c == t->cx) ? CUR_FLAG : 0);
            if (t->shown[r][c] == want) { c++; continue; }

            if (want & CUR_FLAG) rigel_settextcolor(COR_FUNDO, COR_TEXTO);
            else                 rigel_settextcolor(COR_TEXTO, COR_FUNDO);
            rigel_gotoxy(WIN_COL + c, t->row + r);

            c2 = c;
            while (c2 < COLS) {
                uint16_t w2 = t->grid[r][c2] |
                              ((active && r == t->cy && c2 == t->cx) ? CUR_FLAG : 0);
                if (t->shown[r][c2] == w2 || (w2 & CUR_FLAG) != (want & CUR_FLAG))
                    break;
                rigel_vga_putchar(t->grid[r][c2]);
                t->shown[r][c2] = w2;
                c2++;
            }
            c = c2;
        }
    }
}

/* ------------------------------------------------------------ shell da janela */
static int start_shell(term_t *t)
{
    int pty[2];
    char *argv[2];
    long pid;

    t->alive = 0;
    if (rigel_openpty(pty) < 0) {
        puts_grid(t, "\n[gshell: sem pty]\n");
        return -1;
    }
    argv[0] = (char *)SHELL_PATH;
    argv[1] = NULL;
    pid = rigel_spawn_io(SHELL_PATH, argv, pty[1], pty[1], pty[1]);
    close(pty[1]);                      /* so' o shell segura a ponta dele */
    if (pid < 0) {
        close(pty[0]);
        puts_grid(t, "\n[gshell: nao consegui iniciar o shell (sem slot livre?)]\n");
        return -1;
    }
    t->pty = pty[0];
    t->pid = pid;
    t->alive = 1;
    return 0;
}

static void term_init(term_t *t, int idx)
{
    int r, c;

    memset(t, 0, sizeof *t);
    t->row = FIRST_ROW + idx * ROW_STEP;
    t->title[0] = 'g'; t->title[1] = 's'; t->title[2] = 'h'; t->title[3] = 'e';
    t->title[4] = 'l'; t->title[5] = 'l'; t->title[6] = ' ';
    t->title[7] = (char)('1' + idx);
    t->title[8] = 0;
    for (r = 0; r < ROWS; r++) {
        clear_row(t, r);
        for (c = 0; c < COLS; c++)
            t->shown[r][c] = 0xFFFF;    /* forca o primeiro desenho completo */
    }
}

/* le a saida do shell da janela; devolve 1 se algo mudou */
static int pump(term_t *t)
{
    uint8_t tx[256];
    int n, busy = 0;

    if (!t->alive)
        return 0;
    for (;;) {
        n = read(t->pty, tx, sizeof tx);
        if (n < 0) {                    /* o shell terminou (exit): solta e inicia outro */
            close(t->pty);
            puts_grid(t, "\n[shell encerrado; iniciando outro]\n");
            start_shell(t);
            return 1;
        }
        if (n == 0)
            break;
        busy = 1;
        {
            int i;
            for (i = 0; i < n; i++)
                put(t, tx[i]);
        }
    }
    return busy;
}

/* ------------------------------------------------------------ main */
int main(void)
{
    uint8_t kb[32], out[32];
    int i;

    for (i = 0; i < NTERM; i++)
        term_init(&T[i], i);
    for (i = 0; i < NTERM; i++)
        start_shell(&T[i]);

    /* teclado: bytes crus (setas 0x10..0x13), sem eco, sem bloquear */
    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO | TTY_NONBLOCK));
    rigel_setcurvisibility(2);                  /* esconde o cursor do Pico (travado); cada janela tem o seu */

    for (i = 0; i < NTERM; i++)
        draw_frame(&T[i], i == cur);

    for (;;) {
        int n, busy = 0;

        /* 1) teclado -> janela ativa */
        n = read(0, kb, sizeof kb);
        if (n > 0) {
            int k, no = 0;
            busy = 1;
            for (k = 0; k < n; k++) {
#if KEY_DEBUG
                show_key(kb[k]);
#endif
                if (kb[k] == KEY_QUIT)
                    goto fim;
                if (kb[k] == KEY_SWITCH) {
                    cur = (cur + 1) % NTERM;
                    for (i = 0; i < NTERM; i++)
                        draw_title(&T[i], i == cur);
                    continue;
                }
                out[no++] = kb[k];
            }
            if (no > 0 && T[cur].alive) {
                int off = 0;
                while (off < no) {
                    int w = write(T[cur].pty, out + off, no - off);
                    if (w <= 0) break;      /* pty cheio ou shell saindo: descarta o resto */
                    off += w;
                }
            }
        }

        /* 2) saida de todos os shells -> grades; 3) tela */
        for (i = 0; i < NTERM; i++) {
            if (pump(&T[i]))
                busy = 1;
            render(&T[i], i == cur);
        }

        if (!busy)
            rigel_yield();
    }

fim:
    for (i = 0; i < NTERM; i++) {
        if (T[i].alive) {
            rigel_kill(T[i].pid, 9);        /* ex.: shell parado dentro do telnetd nao leria 'exit' */
            close(T[i].pty);
        }
    }
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);
    rigel_setcurvisibility(3);                  /* destrava e devolve o cursor do console */
    rigel_fillrect(0, 0, 640, 480, COR_FUNDO);
    rigel_gotoxy(0, 0);
    return 0;
}
