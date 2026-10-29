#include <stdint.h>
#include "vga_gfx.h"

/* Mesmos endereços usados no sys_vga_graph.S: base + (n*2)+1 */
#define PICO_VGA_BASE 0x00FF8000UL
#define VGA8(off)     (*((volatile uint8_t *)(PICO_VGA_BASE + (off))))

#define R_WRITE_SCREEN  VGA8(0x01)   /* escreve um caractere na tela */
#define R_COLOR         VGA8(0x1D)   /* REG_PRIMITIVE_COLOR */
#define R_W_HIGH        VGA8(0x1F)
#define R_W_LOW         VGA8(0x21)
#define R_H_HIGH        VGA8(0x23)
#define R_H_LOW         VGA8(0x25)
#define R_SET_TXT_COLOR VGA8(0x29)   /* nibble alto = texto, baixo = fundo */
#define R_X_HIGH        VGA8(0x2D)
#define R_X_LOW         VGA8(0x2F)
#define R_Y_HIGH        VGA8(0x31)
#define R_Y_LOW         VGA8(0x33)
#define R_RUN_CMD       VGA8(0x3D)

#define CMD_SET_CUR_POS  0xA3
#define CMD_DRAW_VLINE   0xBE
#define CMD_DRAW_HLINE   0xBD
#define CMD_DRAW_FILLRECT 0xB6

/* ------------------------------------------------------------------ */
/* ESPERAS (o Pico descarta escritas que chegam enquanto ele ainda    */
/* está processando a anterior: orion_bus.c faz pio_sm_clear_fifos).   */
/* Valores de TESTE. Depois que tudo estiver certo, diminua até o      */
/* ponto em que ainda funciona.                                        */
/*  VGA_DELAY_PARAM: entre os parâmetros e o comando                   */
/*  VGA_DELAY_SHORT: depois de linha / gotoxy / cor de texto           */
/*  VGA_DELAY_FILL : depois de preencher retângulo                     */
/*  VGA_DELAY_CHAR : depois de cada caractere                          */
/* ------------------------------------------------------------------ */
#ifndef VGA_DELAY_PARAM
#define VGA_DELAY_PARAM  5000UL
#endif
#ifndef VGA_DELAY_SHORT
#define VGA_DELAY_SHORT  20000UL
#endif
#ifndef VGA_DELAY_FILL
#define VGA_DELAY_FILL   100000UL
#endif
#ifndef VGA_DELAY_CHAR
#define VGA_DELAY_CHAR   20000UL
#endif

static void vga_wait(uint32_t n)
{
    for (volatile uint32_t i = 0; i < n; i++) {
        /* nada: só gasta tempo */
    }
}

/* X e Y: o firmware trata cursor_x/cursor_y direito, a ordem não importa. */
static void set_xy(int16_t x, int16_t y)
{
    R_X_LOW  = (uint8_t)(x & 0xFF);
    R_X_HIGH = (uint8_t)((x >> 8) & 0xFF);
    R_Y_LOW  = (uint8_t)(y & 0xFF);
    R_Y_HIGH = (uint8_t)((y >> 8) & 0xFF);
}

/* LARGURA e ALTURA: o orion_bus.c ORIGINAL tem um bug. Quando recebe o
 * byte ALTO, ele monta  (alto << 8) | (cursor_x & 0xFF)  e joga fora o
 * byte baixo que já tinha chegado. Por isso escrevemos o ALTO primeiro e o
 * BAIXO depois: o BAIXO então completa o valor certo. Funciona com o
 * firmware com bug E com o firmware corrigido (orion_bus_fixed.c). */
static void set_w(int16_t w)
{
    R_W_HIGH = (uint8_t)((w >> 8) & 0xFF);
    R_W_LOW  = (uint8_t)(w & 0xFF);
}

static void set_h(int16_t h)
{
    R_H_HIGH = (uint8_t)((h >> 8) & 0xFF);
    R_H_LOW  = (uint8_t)(h & 0xFF);
}

/* Posição do cursor de TEXTO: coluna e linha (o Pico multiplica pela
 * largura/altura da fonte, ver setTextCursor em vga_primitives.c). */
void vga_gotoxy(int16_t col, int16_t row)
{
    set_xy(col, row);
    vga_wait(VGA_DELAY_PARAM);
    R_RUN_CMD = CMD_SET_CUR_POS;
    vga_wait(VGA_DELAY_SHORT);
}

void vga_settextcolor(uint8_t fg, uint8_t bg)
{
    R_SET_TXT_COLOR = (uint8_t)(((fg & 0x0F) << 4) | (bg & 0x0F));
    vga_wait(VGA_DELAY_SHORT);
}

void vga_putc(char c)
{
    R_WRITE_SCREEN = (uint8_t)c;
    vga_wait(VGA_DELAY_CHAR);
}

void vga_puts(const char *s)
{
    while (*s) {
        vga_putc(*s++);
    }
}

void vga_hline(int16_t x, int16_t y, int16_t w, uint8_t color)
{
    R_COLOR = color;
    set_xy(x, y);
    set_w(w);
    vga_wait(VGA_DELAY_PARAM);
    R_RUN_CMD = CMD_DRAW_HLINE;
    vga_wait(VGA_DELAY_SHORT);
}

void vga_vline(int16_t x, int16_t y, int16_t h, uint8_t color)
{
    R_COLOR = color;
    set_xy(x, y);
    set_h(h);
    vga_wait(VGA_DELAY_PARAM);
    R_RUN_CMD = CMD_DRAW_VLINE;
    vga_wait(VGA_DELAY_SHORT);
}

void vga_fillrect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    R_COLOR = color;
    set_xy(x, y);
    set_w(w);
    set_h(h);
    vga_wait(VGA_DELAY_PARAM);
    R_RUN_CMD = CMD_DRAW_FILLRECT;
    vga_wait(VGA_DELAY_FILL);
}