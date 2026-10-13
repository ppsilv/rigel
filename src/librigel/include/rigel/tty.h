/* GERADO por tools/gen_librigel.py a partir do vfs.h do kernel -- NAO EDITE. */
/* Uso:  rigel_ioctl(0, TIOC_SETMODE, (void *)(TTY_RAW | TTY_NOECHO)); */
#ifndef RIGEL_TTY_H
#define RIGEL_TTY_H

#define TIOC_CLEAR     0x01
#define TIOC_GETXY     0x02
#define TIOC_SETXY     0x03
#define TIOC_SETMODE   0x04
#define TIOC_GETMODE   0x05
#define TTY_RAW        0x01
#define TTY_NOECHO     0x02
#define TTY_NONBLOCK   0x04

#endif /* RIGEL_TTY_H */
