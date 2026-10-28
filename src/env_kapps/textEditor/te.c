/*
 * te.c -- editor de texto para o Rigel (porta do te.c do Linux).
 *
 *   te [arquivo]
 *
 *   setas, Home, End, PgUp, PgDn   movem o cursor
 *   Del / Backspace                apagam
 *   Enter, Tab                     quebra a linha / avanca
 *   Ctrl-W                         salva e sai
 *   Ctrl-L                         pede o nome e abre um arquivo
 *   ESC                            sai SEM salvar
 *
 * Diferencas para a versao do Linux:
 *   - sem thread, mutex e cond: o proprio laco principal redesenha depois de cada tecla;
 *   - sem termios nem sequencias ESC: o teclado do Rigel manda 1 byte por tecla (modo TTY_RAW);
 *   - sem ANSI: a tela e' posicionada com rigel_gotoxy / rigel_cursor;
 *   - so' redesenha as linhas de tela que mudaram (copia "shadow");
 *   - arquivos com open/read/write/close e as flags do VFS do Rigel (nao as da newlib).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <rigel/rigel.h>

/* flags do open() do VFS do Rigel (iguais as do shell) */
#define SH_O_RDONLY   0x01
#define SH_O_WRONLY   0x02
#define SH_O_RDWR     0x03
#define SH_O_TRUNC    0x08

/* codigos que o teclado do Rigel manda (medidos com rigeltest k) */
#define KEY_UP     0x10
#define KEY_DOWN   0x11
#define KEY_LEFT   0x12
#define KEY_RIGHT  0x13
#define KEY_HOME   0x14
#define KEY_PUP    0x15
#define KEY_PDW    0x16
#define KEY_END    0x17
#define KEY_INS    0x18
#define KEY_DEL    0x7F
#define KEY_BS     0x08
#define KEY_ENTER  0x0D
#define KEY_TABS   0x09
#define KEY_ESC    0x1B
#define KEY_CTRLC  0x03
#define KEY_CTRLW  0xB7
#define KEY_CTRLL  0xAC

/* rigel_gotoxy(coluna, linha): se a primeira posicao da tela for 1 em vez de 0, troque para 1 */
#define SCR_BASE   0

#define TAB_VALUE  4
#define WIDTH      80
#define HEIGHT     30
#define LINHAS_TELA   (HEIGHT - 2)      /* linhas 0..27 = texto; 28 = ajuda; 29 = status */
#define LINHA_AJUDA   (HEIGHT - 2)
#define LINHA_STATUS  (HEIGHT - 1)

#define BUFFER_SIZE   (64 * 1024)

static char buffer_editor[BUFFER_SIZE];     /* texto + espacos ate' o fim do buffer */
static char nomearquivo[65];
static int  cursor_x = 0, cursor_y = 0, line = 0;
static int  janela = 0;                      /* offset da 1a linha de TELA visivel */
static int  max_size = 0;                    /* tamanho do texto */
static int  text_changed = 1;
static char shadow[LINHAS_TELA][WIDTH];      /* o que ja' esta na tela */

static const int width = WIDTH;

/* ------------------------------------------------------------------ tela */
static void goto_xy(int x, int y)
{
    rigel_gotoxy(x + SCR_BASE, y + SCR_BASE);
}

static void tela_escreve(int x, int y, const char *s, int n)
{
    goto_xy(x, y);
    write(STDOUT_FILENO, s, (size_t)n);
}

/* escreve uma linha inteira de texto (preenche com espacos) numa linha de tela;
 * as linhas de tela de baixo usam so' 79 colunas para nao rolar a tela */
static void tela_linha(int y, const char *txt)
{
    char s[WIDTH];
    int n = (int)strlen(txt);
    int max = (y == HEIGHT - 1) ? WIDTH - 1 : WIDTH;
    if (n > max) n = max;
    memset(s, ' ', (size_t)max);
    memcpy(s, txt, (size_t)n);
    tela_escreve(0, y, s, max);
}

static void limpa_tela_vcon(void)           /* igual ao comando clear do shell */
{
    int fd = open("/dev/vcon", SH_O_RDWR);
    if (fd >= 0) {
        rigel_ioctl(fd, TIOC_CLEAR, NULL);
        close(fd);
    }
}

/* ------------------------------------------------------------------ modelo do buffer */
static int ml_fim;   /* offset onde o texto da linha termina (no '\n' ou no corte) */

/* Monta uma linha de tela (80 colunas) a partir de 'off'. Trata \n, \r, TAB e quebra em 80 colunas.
 * Devolve o offset do inicio da proxima linha de tela.
 * tx >= 0: em *toff devolve o offset do byte que ocupa a coluna tx (-1 se a linha nao chega la). */
static int monta_linha(int off, char *linha, int *ncols, int tx, int *toff)
{
    int n = 0, p = off, fim = -1;
    if (toff) *toff = -1;
    while (n < width && p < BUFFER_SIZE) {
        unsigned char c = (unsigned char)buffer_editor[p++];
        if (c == '\n') { fim = p - 1; break; }
        if (c == '\r') continue;
        int w = 1;
        if (c == '\t') w = TAB_VALUE - (n % TAB_VALUE);
        if (w > width - n) w = width - n;
        if (toff && tx >= n && tx < n + w) *toff = p - 1;
        if (c == '\t') memset(linha + n, ' ', (size_t)w);
        else linha[n] = (c < 0x20 || c >= 0x7F) ? '.' : (char)c;
        n += w;
    }
    ml_fim = (fim < 0) ? p : fim;
    if (n == width && p < BUFFER_SIZE && buffer_editor[p] == '\n') p++;
    if (ncols) *ncols = n;
    memset(linha + n, ' ', (size_t)(width - n));
    return p;
}

/* Offset do inicio da linha de tela numero y (0 = primeira visivel) */
static int inicio_linha_tela(int y)
{
    char tmp[WIDTH];
    int off = janela;
    while (y-- > 0) {
        int nx = monta_linha(off, tmp, NULL, -1, NULL);
        if (nx <= off) break;
        off = nx;
    }
    return off;
}

/* Offset do inicio da linha de tela imediatamente antes de 'off' */
static int linha_anterior(int off)
{
    char tmp[WIDTH];
    if (off <= 0) return 0;
    int ls = off - 1;
    while (ls > 0 && buffer_editor[ls - 1] != '\n') ls--;   /* inicio da linha logica */
    int r = ls;
    for (;;) {
        int nx = monta_linha(r, tmp, NULL, -1, NULL);
        if (nx >= off || nx <= r) return r;
        r = nx;
    }
}

static void rolar_para(int novo)
{
    janela = novo;
    text_changed = 1;
}

static int rolar_baixo_f(int forca)
{
    char tmp[WIDTH];
    int nx = monta_linha(janela, tmp, NULL, -1, NULL);
    if (nx <= janela || (!forca && nx >= max_size)) return 0;   /* fim do arquivo */
    rolar_para(nx);
    return 1;
}
static int rolar_baixo(void) { return rolar_baixo_f(0); }
static int rolar_cima(void)
{
    if (janela <= 0) return 0;                  /* inicio do arquivo */
    rolar_para(linha_anterior(janela));
    return 1;
}

/* Insere 'len' bytes no offset 'off' do buffer */
static int inserir_bytes(int off, const char *src, int len)
{
    if (off < 0 || len <= 0 || off + len > BUFFER_SIZE) return 0;
    if (off >= max_size) {                      /* alem do texto: o buffer ja e' so espacos */
        memcpy(buffer_editor + off, src, (size_t)len);
        max_size = off + len;
        return 1;
    }
    if (max_size + len > BUFFER_SIZE) return 0; /* buffer cheio */
    memmove(buffer_editor + off + len, buffer_editor + off, (size_t)(max_size - off));
    memcpy(buffer_editor + off, src, (size_t)len);
    max_size += len;
    return 1;
}

/* Apaga 1 byte no offset 'off' */
static void apaga_byte(int off)
{
    if (off < 0 || off >= max_size) return;
    memmove(buffer_editor + off, buffer_editor + off + 1, (size_t)(max_size - off - 1));
    buffer_editor[max_size - 1] = 0x20;
    max_size--;
}

/* ------------------------------------------------------------------ edicao */
static void printchar(char ch)                  /* insere um caractere na posicao do cursor */
{
    char tmp[WIDTH], buf[WIDTH];
    int off, n;
    monta_linha(inicio_linha_tela(cursor_y), tmp, &n, cursor_x, &off);
    if (off >= 0) {
        inserir_bytes(off, &ch, 1);
    } else {                                    /* cursor alem do fim da linha: completa com espacos */
        int pad = cursor_x - n;
        if (pad < 0) pad = 0;
        memset(buf, 0x20, (size_t)pad);
        buf[pad] = ch;
        inserir_bytes(ml_fim, buf, pad + 1);
    }
    text_changed = 1;
}

static void move_cursor_down_f(int forca)
{
    if (cursor_y < LINHAS_TELA - 1) cursor_y++;
    else rolar_baixo_f(forca);                  /* forca=1: acompanha a digitacao alem do fim do texto */
    line++;
}
static void move_cursor_down(void) { move_cursor_down_f(0); }
static void move_cursor_up(void)
{
    if (cursor_y > 0) cursor_y--;
    else rolar_cima();
    if (line > 0) line--;
}
static void move_cursor_right(void)
{
    cursor_x++;
    if (cursor_x >= width) {
        cursor_x = 0;
        move_cursor_down_f(1);
    }
}
static void move_cursor_left(void)
{
    if (cursor_x > 0) cursor_x--;
}
static void move_cursor_tabs(void)
{
    cursor_x += TAB_VALUE;
    if (cursor_x >= width) {
        cursor_x = 0;
        move_cursor_down_f(1);
    }
}

static void inserir_enter(void)                 /* Enter: quebra a linha na posicao do cursor */
{
    char tmp[WIDTH];
    int off, n;
    monta_linha(inicio_linha_tela(cursor_y), tmp, &n, cursor_x, &off);
    if (off < 0) off = ml_fim;                  /* cursor no fim da linha */
    inserir_bytes(off, "\n", 1);
    text_changed = 1;
    cursor_x = 0;
    move_cursor_down_f(1);
}

/* Backspace: apaga o caractere antes do cursor; na coluna 0 junta com a linha de cima */
static void move_cursor_bs(void)
{
    char tmp[WIDTH];
    int off, n;
    if (cursor_x > 0) {
        cursor_x--;
        monta_linha(inicio_linha_tela(cursor_y), tmp, &n, cursor_x, &off);
        if (off >= 0) apaga_byte(off);
        text_changed = 1;
        return;
    }
    int cur = inicio_linha_tela(cursor_y);
    if (cur <= 0) return;                       /* inicio do arquivo */
    if (cursor_y > 0) {
        cursor_y--;
        if (line > 0) line--;
    } else {
        rolar_cima();
    }
    monta_linha(inicio_linha_tela(cursor_y), tmp, &n, -1, NULL);
    cursor_x = (n < width) ? n : width - 1;     /* fim da linha de cima */
    apaga_byte(cur - 1);                        /* o '\n' (ou o ultimo caractere, se a linha quebrou sozinha) */
    text_changed = 1;
}

/* Delete: apaga o caractere sob o cursor; no fim da linha junta com a de baixo */
static void delete_key(void)
{
    char tmp[WIDTH];
    int off, n;
    monta_linha(inicio_linha_tela(cursor_y), tmp, &n, cursor_x, &off);
    if (off >= 0) apaga_byte(off);
    else if (ml_fim < max_size && buffer_editor[ml_fim] == '\n') apaga_byte(ml_fim);
    text_changed = 1;
}

static void page_down(void)
{
    for (int i = 0; i < LINHAS_TELA - 1; i++) if (!rolar_baixo()) break;
}
static void page_up(void)
{
    for (int i = 0; i < LINHAS_TELA - 1; i++) if (!rolar_cima()) break;
}
static void cursor_home(void)
{
    cursor_x = 0;
}
static void cursor_end(void)
{
    char tmp[WIDTH];
    int n = 0;
    monta_linha(inicio_linha_tela(cursor_y), tmp, &n, -1, NULL);
    cursor_x = (n < width) ? n : width - 1;
}

/* ------------------------------------------------------------------ desenho */
/* Desenha a janela atual, so' as linhas de tela que mudaram */
static void desenhar(void)
{
    char linha[WIDTH];
    int off = janela;
    for (int y = 0; y < LINHAS_TELA; y++) {
        int nx = monta_linha(off, linha, NULL, -1, NULL);
        if (memcmp(linha, shadow[y], WIDTH) != 0) {
            memcpy(shadow[y], linha, WIDTH);
            tela_escreve(0, y, linha, WIDTH);
        }
        if (nx > off) off = nx;
    }
}

static void desenha_ajuda(void)
{
    tela_linha(LINHA_AJUDA, "ESC sai sem salvar   Ctrl-W salva e sai   Ctrl-L abre arquivo");
}

static void desenha_status(void)
{
    char s[128];
    snprintf(s, sizeof s, "col: %02d line: %02d  %s", cursor_x, line,
             nomearquivo[0] ? nomearquivo : "(sem nome)");
    tela_linha(LINHA_STATUS, s);
}

static void invalida_tela(void)                 /* forca o redesenho de tudo */
{
    memset(shadow, 0, sizeof shadow);
    text_changed = 1;
    desenha_ajuda();
}

static int ult_x = -1, ult_y = -1;              /* onde o cursor estava no ultimo redesenho */

static void redesenhar(void)
{
    rigel_cursor(0);
    /* A VGA apaga a letra da celula onde o cursor passa; ao sair dela, redesenha a linha antiga */
    if (ult_y >= 0 && ult_y < LINHAS_TELA && (ult_x != cursor_x || ult_y != cursor_y)) {
        memset(shadow[ult_y], 0, WIDTH);
        text_changed = 1;
    }
    if (text_changed) {
        desenhar();
        text_changed = 0;
    }
    desenha_status();
    goto_xy(cursor_x, cursor_y);
    rigel_cursor(1);
    ult_x = cursor_x;
    ult_y = cursor_y;
}

/* ------------------------------------------------------------------ teclado */
static int getkey(void)
{
    unsigned char c;
    for (;;) {
        if (read(STDIN_FILENO, &c, 1) == 1) break;
    }
    if (c == 0x0A) return KEY_ENTER;
    return c;
}

/* pede um nome na linha de status; ESC cancela (devolve string vazia) */
static void ler_nome_arquivo(char *destino, size_t tamanho_max)
{
    size_t i = 0;
    tela_linha(LINHA_STATUS, "Nome do arquivo: ");
    goto_xy(17, LINHA_STATUS);
    rigel_cursor(1);

    while (i < tamanho_max - 1) {
        int tecla = getkey();
        if (tecla == KEY_ENTER) {
            break;
        } else if (tecla == KEY_ESC) {
            i = 0;
            break;
        } else if (tecla == KEY_BS || tecla == KEY_DEL) {
            if (i > 0) {
                i--;
                tela_escreve((int)(17 + i), LINHA_STATUS, " ", 1);
                goto_xy((int)(17 + i), LINHA_STATUS);
            }
        } else if (tecla >= 32 && tecla <= 126 && 17 + i < WIDTH - 2) {
            char ch = (char)tecla;
            destino[i] = ch;
            tela_escreve((int)(17 + i), LINHA_STATUS, &ch, 1);
            i++;
        }
    }
    destino[i] = '\0';
    invalida_tela();
}

/* ------------------------------------------------------------------ arquivos */
/* devolve 0 se gravou tudo, -1 em erro */
static int salvar_buffer(const char *buffer, size_t tamanho)
{
    int fd;
    size_t feito = 0;

    if (nomearquivo[0] == '\0') {
        ler_nome_arquivo(nomearquivo, sizeof nomearquivo);
        if (nomearquivo[0] == '\0') return -1;
    }
    fd = open(nomearquivo, SH_O_WRONLY | SH_O_TRUNC);    /* cria ou apaga o conteudo */
    if (fd < 0) return -1;
    while (feito < tamanho) {
        size_t n = tamanho - feito;
        long r;
        if (n > 4096) n = 4096;
        r = write(fd, buffer + feito, n);
        if (r <= 0) break;
        feito += (size_t)r;
    }
    close(fd);
    return (feito == tamanho) ? 0 : -1;
}

/* devolve quantos bytes leu, ou -1 em erro. nome == NULL: pergunta o nome. */
static long carregar_arquivo(const char *nome, char *buffer, size_t maximo)
{
    int fd;
    size_t total = 0;

    if (nome == NULL) {
        char novo[65];
        ler_nome_arquivo(novo, sizeof novo);
        if (novo[0] == '\0') return -1;
        strcpy(nomearquivo, novo);
        nome = nomearquivo;
    }
    fd = open(nome, SH_O_RDONLY);
    if (fd < 0) return -1;
    while (total < maximo) {
        long r = read(fd, buffer + total, maximo - total);
        if (r < 0) { close(fd); return -1; }
        if (r == 0) break;
        total += (size_t)r;
    }
    close(fd);
    return (long)total;
}

static void abrir(const char *nome)
{
    long n = carregar_arquivo(nome, buffer_editor, BUFFER_SIZE);
    if (n >= 0) {
        memset(buffer_editor + n, 0x20, (size_t)(BUFFER_SIZE - n));   /* limpa sobras */
        max_size = (int)n;
        janela = 0; cursor_x = 0; cursor_y = 0; line = 0;
    }
    invalida_tela();
}

/* ------------------------------------------------------------------ main */
int main(int argc, char *argv[])
{
    int saiu_salvando = 0;

    memset(buffer_editor, 0x20, sizeof buffer_editor);

    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO));
    limpa_tela_vcon();

    if (argc > 1) {
        strncpy(nomearquivo, argv[1], sizeof nomearquivo - 1);
        abrir(nomearquivo);
    }
    invalida_tela();
    redesenhar();

    for (;;) {
        int tecla = getkey();

        if (tecla == KEY_CTRLW) {
            if (salvar_buffer(buffer_editor, (size_t)max_size) == 0) {
                saiu_salvando = 1;
                break;
            }
            tela_linha(LINHA_STATUS, "ERRO ao salvar o arquivo (aperte uma tecla)");
            getkey();
            invalida_tela();
        } else if (tecla == KEY_CTRLL) {
            abrir(NULL);
        } else if (tecla == KEY_ESC) {
            break;                              /* sai sem salvar */
        } else if (tecla == KEY_BS) {
            move_cursor_bs();
        } else if (tecla == KEY_RIGHT) {
            move_cursor_right();
        } else if (tecla == KEY_LEFT) {
            move_cursor_left();
        } else if (tecla == KEY_UP) {
            move_cursor_up();
        } else if (tecla == KEY_DOWN) {
            move_cursor_down();
        } else if (tecla == KEY_PUP) {
            page_up();
        } else if (tecla == KEY_PDW) {
            page_down();
        } else if (tecla == KEY_HOME) {
            cursor_home();
        } else if (tecla == KEY_END) {
            cursor_end();
        } else if (tecla == KEY_DEL) {
            delete_key();
        } else if (tecla == KEY_ENTER) {
            inserir_enter();
        } else if (tecla == KEY_TABS) {
            move_cursor_tabs();
        } else if (tecla >= 0x20 && tecla < 0x7F) {
            printchar((char)tecla);
            move_cursor_right();
        }
        /* KEY_INS, KEY_CTRLC e as teclas desconhecidas sao ignoradas */
        redesenhar();
    }

    rigel_ioctl(0, TIOC_SETMODE, (void *)0);    /* devolve o teclado ao modo normal */
    limpa_tela_vcon();
    rigel_cursor(1);
    if (saiu_salvando)
        printf("Salvo %s (%d bytes)\n", nomearquivo, max_size);
    return 0;
}