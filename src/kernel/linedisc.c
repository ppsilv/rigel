#include <sys/vfs.h>          /* TTY_RAW, TTY_NOECHO, TTY_NONBLOCK */
#include <sys/linedisc.h>

int linedisc_read(const ld_io_t *io, uint32_t mode, void *buffer, size_t size)
{
    uint8_t *buf = (uint8_t *)buffer;
    size_t   n = 0;
    int raw      = (mode & TTY_RAW) != 0;
    int echo     = (mode & TTY_NOECHO) == 0;
    int nonblock = raw && (mode & TTY_NONBLOCK) != 0;

    while (n < size) {
        int c;

        /* modo cru: depois do 1o byte (ou sempre, se NONBLOCK) so' pega o que ja esta la */
        if (raw && (n > 0 || nonblock))
            c = io->getch(io->ctx, 0);
        else
            c = io->getch(io->ctx, 1);
        if (c < 0)
            break;                              /* LD_NONE: devolve o que ha'; LD_EOF: acabou */

        if (raw) {
            buf[n++] = (uint8_t)c;
            if (echo && c != '\r')
                io->putch(io->ctx, (uint8_t)c);
            continue;
        }

        if (c == '\r' || c == '\n') {           /* Enter: fecha a linha */
            if (echo)
                io->putch(io->ctx, '\n');
            buf[n++] = '\n';
            break;
        }
        if (c == 0x08 || c == 0x7F) {           /* backspace / delete */
            if (n > 0) {
                n--;
                if (io->bs_echo && echo) {
                    io->putch(io->ctx, 0x08);
                    io->putch(io->ctx, ' ');
                    io->putch(io->ctx, 0x08);
                }
            }
            continue;
        }
        if (c < 0x20)                           /* demais controles: ignora */
            continue;

        buf[n++] = (uint8_t)c;
        if (echo)
            io->putch(io->ctx, (uint8_t)c);
    }
    return (int)n;
}
