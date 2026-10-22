 
O primeiro teste não rodou: `/tmp/t` já existia como arquivo e a sequência parou ali. Refaço numa pasta nova.

O `mv`, o `head` e o `tail` estão prontos, com um Makefile que gera os três.

No PC, com um shim no lugar das chamadas do Rigel, `head` e `tail` deram saída
idêntica à do coreutils em 45 combinações. Foram 5 arquivos (com e sem `\n`
final, vazio e grande) e 9 valores de `-n`. O `mv` passou nos casos de arquivo,
diretório, vários arquivos, `-f` e erros. Não rodei nada no Rigel nem compilei
com `m68k-elf-gcc`, que não tenho aqui.

**Passos:**
1. Crie uma pasta de apps(por exemplo ao lado do `ls`) e copie `mv.c`, `head.c`,
    `tail.c` e `Makefile` para ela.
2. Rode `make`. Saem `mv.elf`, `head.elf` e `tail.elf`. Se o linker reclamar de
    `-lc`, use `make NEWLIB_LIB=<pasta do libc.a>`, como nos outros apps.
3. Copie os `.elf` para o cartão do mesmo jeito que fez com o `ls` e o `chmod`.

**O que cada um faz:**
- **`head` e `tail`:** aceitam `-n N` e `-N`, com padrão de 10 linhas. Com mais
    de um arquivo, imprimem `==> nome <==` antes de cada um. O `tail` lê de trás
    para frente em blocos de 512 bytes, então não percorre o arquivo inteiro.
- **`mv`:** faz `mv origem destino` e `mv a b c diretorio/`. Se o destino já
    existir, recusa e avisa; com `-f` apaga o destino antes. Também recusa mover
    um diretório para dentro dele mesmo. Usa a syscall `rename` (`f_rename`),
    então os atributos D e L são preservados.Só funciona dentro do mesmo volume.

Sobre as suas perguntas anteriores:
- O wrapper de rename é `rename` (padrão POSIX, vindo do `("FRENAME", None,
    "rename", None)`).
- O `lseek` existe, mas é o do VFS e não serve para o `tail`. No `head` e no
    `tail` usei o `rigel_ff_lseek`, que aceita offset negativo e `SEEK_END`.
- Os flags de `open` do VFS são os do Rigel (`O_RDONLY` = 0x01, não o 0 do
    newlib).Se você for escrever um app com `open()`,use as constantes do Rigel.

O `cp` está pronto. Testei no PC com um shim das chamadas do Rigel. Não rodei
no Rigel.

**Passos:**
1. Copie `cp.c` para a mesma pasta do `mv.c`, `head.c` e `tail.c`.
2. Substitua o `Makefile` pelo novo, que agora gera `cp.elf`, `mv.elf`,
    `head.elf` e `tail.elf`.
3. Rode `make`, depois copie o `cp.elf` para o cartão como os outros.

**Como usar:**
- `cp origem destino` copia um arquivo.
- `cp a b c diretorio` copia vários arquivos para dentro de um diretório.
- `cp -r pasta destino` copia a pasta inteira, com subpastas, até 4 níveis
    contando a origem. O limite existe porque o kernel só aceita 8 diretórios
    abertos ao mesmo tempo, somando todas as tasks.
- `cp -n` não sobrescreve o que já existe no destino. Sem `-n`, o destino é
    sobrescrito, como no Linux.
- Os atributos R, H, S, D e L da origem são copiados junto.

Os testes acharam um erro grave, que corrigi antes de entregar: `cp a ./a`
zerava o arquivo `a`. Agora, quando o destino já existe, a cópia vai primeiro
para um temporário `CP~TMP.TMP` na pasta do destino e só depois troca o arquivo.
Assim a origem nunca é zerada, e o destino antigo não se perde se o cartão
encher no meio.

Se uma cópia for interrompida (reset no meio), pode sobrar um `CP~TMP.TMP`
na pasta. É só apagar com `rm`.

Quando quiser, seguimos com `ps` e `kill`.


Próximos da lista: `ps` e `kill`, que precisam de syscalls novas no kernel.
Quando quiser começar, eu parto do `scheduler.c` que você já me mandou.

