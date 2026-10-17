Agora o ls do Rigel tem as 6 colunas DLRHSA, como o seu. Testei a saída no meu computador, com atributos de mentira. Não rodei no Rigel.

O que o seu código mostrou

Os 2 atributos a mais são os bits 6 e 7 do byte de atributo. Interpretei assim: bit 6 (0x40) é D (dispositivo) e bit 7 (0x80) é L (link). A ordem DLRHSA veio do seu cabeçalho. Se for o contrário, troque os dois ? na função fmt_attr_string do ls.c.
Os bits que estão desligados aparecem como -.

Passos

Substitua o ls.c pelo novo (o Makefile não muda) e rode make.
Copie o ls.elf para o disco do Rigel e confira a saída.

Um ponto importante: o FatFS, por padrão, apaga os bits 6 e 7 ao ler o diretório, com a máscara AM_MASK (0x3F) em get_fileinfo, no ff.c. Na sua versão do Orion isso já devia estar ajustado. Se no Rigel as colunas D e L ficarem sempre -, mesmo em arquivos que você sabe que têm esses bits ligados, o ff.c do kernel do Rigel ainda tem a máscara original. Nesse caso me mande o trecho de get_fileinfo do ff.c e eu digo exatamente a linha a mudar.