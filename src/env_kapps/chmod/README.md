Portei o chmod para o Rigel, com os mesmos argumentos do seu (+r -h -s, +r+h+s, e só o nome do arquivo para ver os atributos). Testei a lógica no meu computador com rigel_stat e rigel_chmod de mentira, sem rodar no Rigel.

Passos

Crie uma pasta chmod ao lado das outras, coloque lá o chmod.c e o Makefile e rode make.
Copie o chmod.elf para o disco do Rigel.
Teste: chmod arquivo.txt para ver, depois chmod +r arquivo.txt e chmod -r arquivo.txt.

O que mudou em relação ao seu

Saíram o FatFs, o ideinit, o vputs e o f_stat/f_chmod. Agora o programa usa rigel_stat e rigel_chmod.
A syscall do kernel grava os quatro atributos R, H, S e A de uma vez, em vez de ligar e desligar separadamente. Por isso o programa lê os atuais, aplica as suas mudanças (liga os do +, desliga os do -) e faz uma única chamada. O resultado é o mesmo do seu.
A tabela de letras continua r, h, s e a. Agora também aceito maiúsculas (+R).
As mensagens dizem chmod: (as suas diziam attrib:), e o texto de uso também.
O resultado é mostrado no formato do ls (DLRHSA).

Limite: o chmod do Rigel só altera R, H, S e A. A syscall do kernel ignora os bits 6 e 7, e o f_chmod do FatFS também os ignora por padrão. Então D e L aparecem na listagem, mas o chmod não os muda. Se um dia você quiser gravar D e L, é uma mudança à parte, no kernel e no f_chmod.

Para isso funcionar, o ffconf.h do kernel precisa ter FF_USE_CHMOD = 1. Se der erro ao alterar atributo(s), é provável que seja essa opção.