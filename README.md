# rigel
Bootstrap and a small kernel for MC68000.

# 2026/10/10: crt0.S 
  interrupts with jumptable in sram initialized. \
  int2 systemtick, led alive initialized. \
  int3 keyboard initialized

# Macintosh (1984) e o Lisa (1983), ambos com o MC68000.

# Lisa: MC68000 a 5 MHz, 1 MB de RAM, tela de 12 polegadas. 
  Foi o primeiro computador pessoal com interface gráfica da Apple, 
  e vendeu pouco por ser caro, uns US$ 10.000.
# Macintosh 128K: MC68000 a 7,83 MHz, 128 KB de RAM, tela de 9 polegadas. 
  Seguiram o Mac 512K, o Mac Plus e o Mac SE, todos com o mesmo MC68000. 
  O Mac Plus e o SE usam 8 MHz.

O MC68010 praticamente não foi usado em máquinas da Apple. Depois do SE 
  vieram o Macintosh II e o SE/30, que já usam o MC68020 e o MC68030, e o
  68040 fechou a linha antes dos PowerPC.

O MC68010 é quase igual ao MC68000. Entre as diferenças, ele consegue 
  retomar um acesso de memória que falhou e tem um registrador de base 
  para os vetores de exceção, o VBR. Por isso funcionava melhor em sistemas 
  Unix com memória virtual. Foi usado em máquinas como o Sun-1 e o Sun-2 
  (a primeira Sun) e em alguns computadores da AT&T.

Se você for usar o seu 68010 no Rigel, ele serve no lugar do 68000, e o 
  VBR permitiria colocar a tabela de vetores fora do endereço 0, o que 
  ajuda se um dia você quiser relocar o kernel.
