#ifndef __DUART_H___
#define __DUART_H___

void duart_init_canal_a(void);
void duart_putc(char c);
char duart_getc(void);
void duart_opr_init(void);
void duart_led_op3_on(void);
void duart_led_op3_off(void);
void duart_led_op3_toggle(void);

#endif