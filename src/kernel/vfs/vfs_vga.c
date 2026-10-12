/*
 * vfs_vga.c -- dispositivo /dev/vcon: console de video + teclado.
 *
 *   write : escreve na VGA (PICO_VGA_BASE + 0x01), como o kernel_puts().
 *   read  : le' do ringbuffer (preenchido pela interrupcao do teclado),
 *           em modo linha com eco na VGA: devolve quando chega Enter,
 *           com o '\n' no fim. Backspace apaga o ultimo caractere.
 *
 * O read BLOQUEIA (espera ocupada) ate' o teclado mandar algo.
 */
#include <stddef.h>
#include <stdint.h>
#include <sys/vfs.h>

extern void    kernel_puts(const char *s);

/* ringbuffer.c (a ISR do teclado faz ring_buf_put) */
extern uint8_t ring_buf_is_empty(void);
extern uint8_t ring_buf_get(void);

#define PICO_VGA_BASE     0x00FF8000UL
#define VGA_WRITE_SCREEN  (*((volatile uint8_t *)(PICO_VGA_BASE + 0x01)))

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

int vgacon_read(File *file, void *buffer, size_t size)
{
    uint8_t *buf = (uint8_t *)buffer;
    size_t n = 0;
    (void)file;

    while (n < size) {
        while (ring_buf_is_empty())
            ;                                   /* espera uma tecla */
        uint8_t c = ring_buf_get();

        if (c == '\r' || c == '\n') {           /* Enter: fecha a linha */
            vga_put('\n');
            buf[n++] = '\n';
            break;
        }
        if (c == 0x08 || c == 0x7F) {           /* backspace / delete */
            if (n > 0) {
                n--;
#if VCON_BACKSPACE_OK
                vga_put(0x08); vga_put(' '); vga_put(0x08);
#endif
            }
            continue;
        }
        if (c < 0x20)                           /* demais controles: ignora */
            continue;

        buf[n++] = c;
        vga_put(c);                             /* eco */
    }
    return (int)n;
}

int vgacon_close(File *file)
{
    (void)file;
    return 0;
}

int vgacon_ioctl(File *file, int cmd, void *arg)
{
    (void)file; (void)arg;
    switch (cmd) {
    case TIOC_CLEAR:
        kernel_puts("\033[2J\033[H");     /* mesma sequencia do tty_ioctl */
        return 0;
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
