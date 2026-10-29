Os headers que você mandou era o que faltava, e o `wterm.c` está pronto.
Ele passou na checagem de sintaxe no PC, mas não rodei no Rigel.

**Como funciona**
- É um aplicativo separado, `wterm.elf`, então o kernel e o `main_window`
    não são tocados.
- Ele copia o laço do `telnetd`: `rigel_openpty` e `rigel_spawn_io` iniciam
    o `/bin/shell.elf` com o pty. O teclado vai para o pty, e a saída do shell
    vai para uma grade de 60x20 caracteres na RAM.
- A cada volta ele redesenha só as células que mudaram. O cursor é uma célula
    com as cores invertidas.
- A janela é desenhada só com syscalls da librigel (`rigel_fillrect`,
    `rigel_gotoxy`, `rigel_vga_putchar`), sem acesso direto ao hardware.
- O teclado é lido com `TTY_RAW | TTY_NOECHO | TTY_NONBLOCK`. Por isso as
    setas ↑ e ↓ chegam ao `readline_with_history` do shell.
- Para sair, digite `exit` no shell. O pty fecha, o `wterm` limpa a janela e
    termina.

**Para testar**
1. Compile como os outros apps e copie para `/bin/wterm.elf`.
2. Rode `wterm` no shell normal.
3. Teste `ls`, `help`, a seta ↑ para o histórico e o backspace.
4. Digite `exit`.

**Pontos a conferir**
- **Cores:** os números no topo do arquivo (`COR_FUNDO` 0, `COR_TEXTO` 15,
    `COR_BARRA` 1) são palpite, porque não tenho o `colors.h`. Como o seu CYAN
    apareceu amarelo, a paleta pode estar diferente. Se a cor sair errada, é só
    trocar esses números.
- **Posição do cursor:** o texto assume que `rigel_gotoxy` usa coluna e linha
    em células de texto, e que o cursor do Pico avança sozinho a cada
    `rigel_vga_putchar`. Se aparecer texto torto ou fora do lugar, me diga como
    ficou.
- **Teclado sem bloquear:** assumi que `read(0)` com `TTY_NONBLOCK` devolve 0
    ou -1 quando não há tecla. Se o teclado travar o laço, esse é o ponto a
    ajustar.
- **Programas que não passam pelo pty:** `clear` e `dump` continuam falando com
    a tela e o teclado reais (limpam a tela toda e leem tecla direto), então não
    funcionam bem dentro da janela. O `clear` eu trato na próxima etapa,
    enviando o comando para a janela.

Rode e me conte o que aparece. Se funcionar, o próximo passo é ligar isso ao
    `main_window` para a janela poder ser arrastada.
