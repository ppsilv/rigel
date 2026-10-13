/*
 * ex_raw.c -- exemplo: ler tecla por tecla (modo cru) no Rigel.
 *
 * Mostra o codigo de cada tecla. 'q' termina. As setas e outras teclas
 * especiais chegam como varios bytes (sequencia ESC), um por vez.
 *
 * Compile com o include da librigel (-I<librigel>/include).
 */
#include <stdio.h>
#include <unistd.h>
#include <rigel/rigel.h>      /* rigel_ioctl, TIOC_SETMODE, TTY_RAW, TTY_NOECHO */





int main(void)
{
    unsigned char c;

    printf("Modo cru: tecle algo ('q' sai)\n");

    /* liga o modo cru, sem eco */
    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO));

    for (;;) {
        if (read(0, &c, 1) != 1)       /* espera uma tecla */
            continue;
        if (c == 'q')
            break;
        printf("tecla: 0x%02X (%c)\n", c, (c >= 0x20 && c < 0x7F) ? c : '.');
        fflush(stdout);
    }

    /* IMPORTANTE: volta ao modo normal antes de sair (a tabela de fds e' global) */
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);

    printf("fim\n");
    return 0;
}



/*
 * ex_raw.c -- exemplo: ler tecla por tecla (modo cru) no Rigel.
 *
 * Mostra o codigo de cada tecla. 'q' termina. As setas e outras teclas
 * especiais chegam como varios bytes (sequencia ESC), um por vez.
 *
 * Compile com o include da librigel (-I<librigel>/include).
 */
#include <stdio.h>
#include <unistd.h>
#include <rigel/rigel.h>      /* rigel_ioctl, TIOC_SETMODE, TTY_RAW, TTY_NOECHO */

int main(void)
{
    unsigned char c;

    printf("Modo cru: tecle algo ('q' sai)\n");

    /* liga o modo cru, sem eco */
    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO));

    for (;;) {
        if (read(0, &c, 1) != 1)       /* espera uma tecla */
            continue;
        if (c == 'q')
            break;
        printf("tecla: 0x%02X (%c)\n", c, (c >= 0x20 && c < 0x7F) ? c : '.');
        fflush(stdout);
    }

    /* IMPORTANTE: volta ao modo normal antes de sair (a tabela de fds e' global) */
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);

    printf("fim\n");
    return 0;
}