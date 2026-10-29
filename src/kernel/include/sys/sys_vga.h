#ifndef __SYS_VGA_H__
#define __SYS_VGA_H__
#include <sys/color.h>


void sys_drawvline(uint16_t x, uint16_t y, uint16_t h, char color);
void sys_drawhline(int x, int y, int w, color_t color);
void sys_drawline(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, char color);
void sys_drawcircle(uint16_t x0, uint16_t y0, uint16_t r, char color);
void sys_drawcirclehelper(uint16_t x0, uint16_t y0, uint16_t r, unsigned char cornername, char color);
void sys_fillcircle(uint16_t x0, uint16_t y0, uint16_t r, char color);
void sys_fillcirclehelper(uint16_t x0, uint16_t y0, uint16_t r, unsigned char cornername, uint16_t delta, char color);
void sys_drawroundrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, char color);
void sys_fillroundrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, char color);
void sys_fillrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, color_t color);


void sys_run_cmd(uint8_t cmd);
void sys_gotoxy(uint16_t col,uint16_t row);
void sys_gohome();
void sys_setcolor(uint8_t txtcolor,uint8_t bgcolor);
void sys_clrscr();

#endif