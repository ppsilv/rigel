 
Dá para fazer, e o que você descreveu é melhor do que abrir dois programas separados. Dois `gshell` rodando juntos disputariam o teclado, e as teclas iriam ora para um, ora para outro. Por isso fiz **um só programa com duas janelas**, cada uma com o seu pty e o seu shell. O teclado vai só para a janela ativa, e uma tecla troca a ativa.

Fiz o `gshell.c`, que substitui o seu `wterm`/`gshell` atual. Ele passou na checagem de sintaxe no PC, mas não rodei no Rigel.

**Como fica**
- Duas janelas empilhadas, "gshell 1" e "gshell 2", de 76x12 caracteres cada. Cada uma tem o seu pty e o seu shell.
- Só a janela ativa tem barra azul e cursor, e só ela recebe o teclado.
- **Ctrl-T** troca a janela ativa.
- **Ctrl-X** fecha tudo, mata os shells e limpa a tela.
- `exit` num shell inicia outro na mesma janela.

**Como usar o que você pediu**
1. Na janela 1, rode `telnetd`. As mensagens dele aparecem ali, e o shell dessa janela fica ocupado com ele.
2. Aperte Ctrl-T e trabalhe na janela 2.

**O que pode precisar de ajuste**
- **Teclas:** escolhi Ctrl-T e Ctrl-X porque 0x10 a 0x13 são as setas. Não sei se o seu teclado entrega esses códigos. Se não trocar de janela, mude `KEY_SWITCH` e `KEY_QUIT` no topo do arquivo.
- **Cor da barra inativa:** `COR_BARRA_OFF` é 8, um palpite, porque não tenho o `colors.h`. Troque se ficar estranha.
- **Slots:** cada janela ocupa um slot de aplicativo. Com o `gshell`, 2 shells e o `telnetd`, são 4 slots, e uma sessão telnet usa mais 2. Se a janela mostrar "sem slot livre?", falta slot, e eu preciso do `spawn.c` para ver o limite.
- **Kill:** no Ctrl-X uso `rigel_kill(pid, 9)`. Não sei se o seu kernel trata esse sinal. Se o shell ficar vivo, me diga.

Teste e me conte o que aparecer.
