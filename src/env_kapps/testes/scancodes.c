/*
 * rigeltest.c -- descobre o que o teclado e a VGA do Rigel entendem (para portar o editor).
 *
 *   rigeltest k     mostra em hex cada byte que o teclado manda (aperte as teclas; 'q' sai)
 *   rigeltest v     testa a tela: tamanho, posicionamento do cursor e esconder cursor
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>      /* rigel_ioctl, TIOC_SETMODE, TTY_RAW, TTY_NOECHO */

static void teste_teclas(void)
{
    unsigned char c;

    printf("Aperte as teclas (setas, Del, Home, End, PgUp, PgDn, Ins, Backspace, Enter, Tab,\n");
    printf("Esc, Ctrl-C, Ctrl-W, Ctrl-L). Cada tecla mostra os bytes em hex. 'q' sai.\n\n");
    fflush(stdout);

    rigel_ioctl(0, TIOC_SETMODE, (void *)(long)(TTY_RAW | TTY_NOECHO));
    for (;;) {
        if (read(0, &c, 1) != 1)
            continue;
        if (c == 'q')
            break;
        printf("%02X ", (unsigned)c);
        fflush(stdout);
    }
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);
    printf("\n");
}

static void teste_video(void)
{
    int i;

    /* 1) quantas linhas e colunas: escreve 60 linhas numeradas, cada uma com regua de colunas */
    printf("\033[2J\033[H");
    for (i = 1; i <= 60; i++)
        printf("L%02d 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n", i);
    printf("\n--- fim. Anote: 1o numero Lnn visivel no topo, e ate' que coluna a regua aparece na linha ---\n");
    printf("Aperte Enter para o proximo teste\n");
    fflush(stdout);
    { unsigned char c; while (read(0, &c, 1) != 1 || (c != '\n' && c != '\r')) { } }

    /* 2) posicionamento do cursor com ESC[linha;colunaH */
    printf("\033[2J\033[H");
    printf("\033[5;10HA");              /* A na linha 5, coluna 10 */
    printf("\033[10;20HB");             /* B na linha 10, coluna 20 */
    printf("\033[1;1HC");               /* C no canto de cima a esquerda */
    printf("\033[12;1H");
    printf("Esperado: C no canto (1,1), A na linha 5 coluna 10, B na linha 10 coluna 20.\n");
    printf("Se aparecerem como texto (ex.: '[5;10H') ou tudo na mesma linha, a VGA nao entende ESC[;H.\n");
    fflush(stdout);

    /* 3) esconder e mostrar o cursor */
    printf("\033[?25l");
    printf("Cursor deve ter SUMIDO (se ainda aparece, a VGA nao entende ESC[?25l). Aperte Enter\n");
    fflush(stdout);
    { unsigned char c; while (read(0, &c, 1) != 1 || (c != '\n' && c != '\r')) { } }
    printf("\033[?25h");
    printf("Cursor deve ter VOLTADO.\n");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "k") == 0) { teste_teclas(); return 0; }
    if (argc > 1 && strcmp(argv[1], "v") == 0) { teste_video();  return 0; }
    printf("uso: rigeltest k   (teclado)\n     rigeltest v   (video)\n");
    return 1;
}