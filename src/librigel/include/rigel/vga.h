/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_VGA_H
#define RIGEL_VGA_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_setcurvisibility(const char cur);
long rigel_vga_putchar(long c);
long rigel_gotoxy(long x, long y);
long rigel_gohome(void);
long rigel_cursor(long status);
long rigel_settextcolor(long color,long bgcolor);
long rigel_clrscr(void);
long rigel_drawvline(long x, long y, long h, long color);
long rigel_drawhline(long x, long y, long w, long color);
long rigel_drawline(long x0, long y0, long x1, long y1, long color);
long rigel_drawcircle(long x0, long y0, long r, long color);
long rigel_drawcirclehelper(long x0, long y0, long r, long cornername, long color);
long rigel_fillcircle(long x0, long y0, long r, long color);
long rigel_fillcirclehelper(long x0, long y0, long r, long cornername, long delta, long color);
long rigel_drawroundrect(long x, long y, long w, long h, long r, long color);
long rigel_fillroundrect(long x, long y, long w, long h, long r, long color);
long rigel_fillrect(long x, long y, long w, long h, long color);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_VGA_H */
