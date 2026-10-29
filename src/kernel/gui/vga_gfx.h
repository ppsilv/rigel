#ifndef VGA_GFX_H
#define VGA_GFX_H

#include <stdint.h>

/* Funções do PicoVGA chamáveis direto do C (jsr/rts normal).
 * Não passam por TRAP #0. */

/* Texto: posição em COLUNA/LINHA (não em pixels). Com a fonte 8x16:
 * coluna = x / 8, linha = y / 16. */
void vga_gotoxy(int16_t col, int16_t row);
void vga_settextcolor(uint8_t fg, uint8_t bg);   /* cores 0..15 */
void vga_putc(char c);
void vga_puts(const char *s);

/* Gráficos: posições em PIXELS. */
void vga_hline(int16_t x, int16_t y, int16_t w, uint8_t color);
void vga_vline(int16_t x, int16_t y, int16_t h, uint8_t color);
void vga_fillrect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

#endif