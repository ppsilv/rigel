/*
 * vfs_vga.c -- dispositivo /dev/vcon: console de video + teclado.
 *
 *   write : escreve na VGA (PICO_VGA_BASE + 0x01), como o kernel_puts().
 *   read  : le' do ringbuffer (preenchido pela interrupcao do teclado).
 *
 * Modos (TIOC_SETMODE, flags TTY_* do sys/vfs.h; o modo e' por descritor):
 *   0            linha: espera Enter, eco na VGA, backspace edita; devolve a
 *                linha com '\n' no fim.                        (padrao)
 *   TTY_RAW      devolve assim que chega 1+ tecla, bytes sem tratamento
 *                (inclusive '\r', 0x08 e sequencias ESC).
 *   TTY_NOECHO   nao ecoa o que e' digitado (vale nos dois modos).
 *   TTY_NONBLOCK so' com TTY_RAW: se nao ha tecla, read devolve 0 na hora.
 *
 * O read bloqueante espera dormindo (OS_SleepTicks(1)) ate' o teclado mandar algo.
 * A logica de linha/eco/modos esta em linedisc.c (compartilhada com o pty).
 */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/vfs.h>
#include <sys/scheduler.h>   /* OS_SleepTicks */
#include <sys/linedisc.h>

extern void    kernel_puts(const char *s);

/* ringbuffer.c (a ISR do teclado faz ring_buf_put) */
extern uint8_t ring_buf_is_empty(void);
extern uint8_t ring_buf_get(void);

#define PICO_VGA_BASE     0x00FF8000UL
#ifndef VGA_WRITE_SCREEN                /* (os testes no PC redefinem) */
#define VGA_WRITE_SCREEN  (*((volatile uint8_t *)(PICO_VGA_BASE + 0x01)))
#endif

/* 1 = a VGA entende o backspace (0x08): o eco apaga o caractere na tela.
 * 0 = o caractere sai do buffer da linha, mas continua visivel na tela.   */
#define VCON_BACKSPACE_OK  1

static void vga_put(uint8_t c)
{
    VGA_WRITE_SCREEN = c;
}

int vgacon_write(File *file, const void *buffer, size_t size)
{
    const uint8_t *p = (const uint8_t *)buffer;
    (void)file;

    for (size_t i = 0; i < size; i++) {
        if (p[i] == '\r')           /* o '\n' ja' faz nova linha na VGA */
            continue;
        vga_put(p[i]);
    }
    return (int)size;
}

/* origem dos bytes e destino do eco para o linedisc.c */
static int vcon_getch(void *ctx, int block)
{
    (void)ctx;
    while (ring_buf_is_empty()) {
        if (!block)
            return LD_NONE;
        OS_SleepTicks(1);                       /* dorme 1 tick (10 ms) em vez de girar */
    }
    return ring_buf_get();
}

static void vcon_putch(void *ctx, uint8_t c)
{
    (void)ctx;
    vga_put(c);
}

int vgacon_read(File *file, void *buffer, size_t size)
{
    ld_io_t io;

    io.getch   = vcon_getch;
    io.putch   = vcon_putch;
    io.ctx     = NULL;
    io.bs_echo = VCON_BACKSPACE_OK;
    return linedisc_read(&io, (uint32_t)(uintptr_t)file->private_data, buffer, size);
}

int vgacon_close(File *file)
{
    (void)file;
    return 0;
}

int vgacon_ioctl(File *file, int cmd, void *arg)
{
    switch (cmd) {
    case TIOC_CLEAR:
        kernel_puts("\033[2J\033[H");     /* mesma sequencia do tty_ioctl */
        return 0;
    case TIOC_SETMODE:
        file->private_data = (void *)(uintptr_t)((uint32_t)(uintptr_t)arg & (TTY_RAW | TTY_NOECHO | TTY_NONBLOCK));
        return 0;
    case TIOC_GETMODE:
        return (int)(uint32_t)(uintptr_t)file->private_data;
    default:
        return -1;
    }
}

size_t vgacon_lseek(File *file, size_t offset, int whence)
{
    (void)file; (void)offset; (void)whence;
    return (size_t)-1;
}

int vgacon_open(File *file, const char *path, int flags)
{
    (void)path; (void)flags;
    file->private_data = NULL;
    file->position = 0;
    file->read  = vgacon_read;
    file->write = vgacon_write;
    file->close = vgacon_close;
    file->ioctl = vgacon_ioctl;
    file->lseek = vgacon_lseek;
    return 0;
}
