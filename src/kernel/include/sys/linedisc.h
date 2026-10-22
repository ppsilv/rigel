#ifndef LINEDISC_H
#define LINEDISC_H
/*
 * linedisc -- disciplina de linha do console (a logica que estava dentro do vgacon_read),
 * separada da origem dos bytes e do destino do eco para servir ao /dev/vcon (teclado+VGA)
 * e ao pty (telnet).
 *
 * Modos (flags TTY_* de sys/vfs.h):
 *   0           linha: espera Enter, eco, backspace edita; devolve a linha com '\n' no fim
 *   TTY_RAW     devolve assim que chega 1+ byte, sem tratar nada
 *   TTY_NOECHO  nao ecoa
 *   TTY_NONBLOCK so' com TTY_RAW: se nao ha byte, devolve 0 na hora
 */
#include <stdint.h>
#include <stddef.h>

#define LD_NONE (-1)   /* getch sem bloqueio: nao ha byte agora */
#define LD_EOF  (-2)   /* getch: a outra ponta fechou e nao ha mais bytes */

typedef struct {
    int  (*getch)(void *ctx, int block);      /* 0..255, LD_NONE ou LD_EOF */
    void (*putch)(void *ctx, uint8_t c);      /* eco */
    void  *ctx;
    int    bs_echo;                           /* 1 = o destino entende 0x08: eco do backspace apaga na tela */
} ld_io_t;

/* Mesmo contrato do read: devolve quantos bytes entregou (0 = fim, quando getch deu LD_EOF) */
int linedisc_read(const ld_io_t *io, uint32_t mode, void *buffer, size_t size);

#endif
