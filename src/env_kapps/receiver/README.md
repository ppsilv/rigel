 
Estou verificando qual API de arquivo do Rigel o app deve usar (open/write/close do newlib ou rigel_ff_*), para escrever o receptor de arquivos.

Erro só no meu shim de teste (tipo duplicado). Corrijo o shim e rodo de novo.

Fiz o `rxfile`, a aplicação que espera numa porta, recebe o arquivo e grava no disco. Testei a lógica no meu computador, com o mesmo código e um remetente em Python no PC. Não compilei para o 68000 nem rodei no hardware.

**Como usar**
1. Crie uma pasta `rxfile` ao lado do `nettest`, coloque lá o `rxfile.c` e o `Makefile` e rode `make`.
2. Copie o `rxfile.elf` para o disco do Rigel.
3. No Rigel, rode `rxfile 7001 /dados` (porta e pasta de destino). Sem a pasta, ele grava na pasta atual, como o `wfile`. A opção `-1` atende um arquivo e termina.
4. No PC, rode `python3 sendfile.py 192.168.1.50 7001 foto.jpg FOTO.JPG`. O nome no Rigel é opcional e deve ter no máximo 12 caracteres (8.3).

**O que ele espera receber**
- 21 bytes de cabeçalho: `tamanho` (4 bytes), `crc32` (4 bytes) e o nome (13 bytes), e logo depois os dados.
- O ponteiro `dados` da sua struct não vai pela rede.
- O cabeçalho é montado byte a byte, então não importa o alinhamento do compilador.

**Ordem dos bytes (decisão sua)**
- Por padrão ele lê `tamanho` e `crc32` em big-endian (ordem de rede, a mesma do 68000).
- Se o seu remetente no PC manda a struct crua em x86, os campos chegam em little-endian. Nesse caso use `rxfile -l`.
- Se a ordem estiver errada, ele avisa na tela e sugere a opção `-l`.

**CRC**
- Se o campo vier zerado, não confere nada.
- Se vier preenchido, ele confere com o CRC-32 padrão (o mesmo do `zlib.crc32`).
- Quando o CRC não bate, o arquivo é apagado.

**Segurança**
- O nome é validado: sem `/`, `\`, `:`, `*` e outros caracteres, e sem `.` ou `..`.
- Se a conexão cair no meio, o disco encher ou o CRC falhar, o arquivo parcial é apagado.
- Sem receber nada por 15 s, ele desiste da conexão.

**Velocidade**
Espere algo na faixa de 50 a 100 KB/s. A janela TCP atual é de 2920 bytes e cada ida e volta leva uns 40 ms. Se ficar lento para o que você quer enviar, o ajuste é aumentar a janela TCP no `lwipopts.h`, e eu faço quando você pedir.

Rode primeiro com um arquivo pequeno (alguns KB) e depois com um de uns 100 KB, e confira o conteúdo no PC e no Rigel.
