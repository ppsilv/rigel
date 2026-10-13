/*
 * ex_input.c -- exemplo de app Rigel que le' linhas do teclado e escreve na tela.
 *
 * Nao precisa abrir /dev/vcon: o kernel ja' liga os fds 0, 1 e 2 a ele.
 * Basta usar as funcoes normais da newlib (stdin/stdout).

Na aplicação você não precisa usar o /dev/vcon nem fazer syscall 
nenhuma à mão. No boot, o vfs_init do kernel já abre o /dev/vcon 
como fd 0 (entrada), fd 1 (saída) e fd 2 (erro). Para o app, o 
teclado é o stdin e a tela é o stdout. Enviei um exemplo, o ex_input.c, 
que lê uma linha do teclado e escreve na tela. Conferi a sintaxe 
no PC, mas não rodei no 68000.

Para ler do teclado, use qualquer uma destas, todas da newlib:

fgets(buf, tamanho, stdin) é a mais indicada para um shell: devolve 
a linha digitada, com o \n no fim.
scanf e getchar também funcionam.
read(0, buf, tamanho) é a chamada direta. Ela cai no SYS_READ, no 
seu trap0.c, e depois no vgacon_read.

Para escrever na tela: printf, puts, putchar ou write(1, ...). 
Se o texto não termina em \n, como em um prompt "> ", chame 
fflush(stdout) antes de ler. O exemplo faz isso.

Compile igual ao shell.elf, com o mesmo comando de link que já 
funciona para você (crt0_app.o, app.ld, -Wl,-q e o grupo -lrigel -lc -lgcc).

Como o read se comporta: ele funciona em modo linha, como um terminal 
comum. O programa fica parado no fgets até você apertar Enter. Enquanto 
digita, o kernel faz o eco na tela, e Backspace apaga o último caractere 
antes de a linha ser entregue.

O que ainda não existe:

Ler uma tecla só, sem esperar Enter (por exemplo, um jogo ou um editor). 
O read atual sempre espera a linha inteira. Para isso o /dev/vcon precisa 
de um modo “cru”. Se for útil para o shell, eu acrescento um ioctl que 
liga e desliga esse modo.
Teclas especiais (setas, histórico de comandos): ignoradas por enquanto, 
como avisei antes.

Quando testar o teclado de verdade, me diga o que aconteceu. Os pontos 
que mais provavelmente precisam de ajuste são os do ringbuffer.h 
(se RING_BUFFER_SIZE é 256 e se head e tail são volatile) e o backspace na VGA.


 */
#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[128];

    printf("Rigel: digite uma linha. 'sair' termina.\n");

    for (;;) {
        printf("> ");
        fflush(stdout);                      /* o prompt nao tem \n */

        if (fgets(line, sizeof line, stdin) == NULL)
            break;

        line[strcspn(line, "\n")] = '\0';    /* tira o \n do fim */

        if (strcmp(line, "sair") == 0)
            break;

        printf("voce digitou: [%s] (%d bytes)\n", line, (int)strlen(line));
    }

    printf("fim\n");
    return 0;
}