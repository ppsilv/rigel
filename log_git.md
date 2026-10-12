## Implementações:
                /dev/vga criado driver de video
                /dev/vcon criado driver teclado

##  Pendencias
#               Pendências, sem pressa:

                Cartão FAT: confira se as execuções antigas deixaram algum 
                arquivo ou pasta dev indevido (o fat_write recebeu os bytes), 
                e me mande o fat_open para entender por que ele aceitou 
                /dev/tty0.
                Saída do app: o último texto foi T2: fim, retornando de 
                main -> exit. Me diga se depois disso o kernel voltou ao 
                shell normalmente, porque isso confirma o exit e o OS_Reap.
                Entrada: scanf, getchar e fgets passam pelo read, que cai 
                no tty_read. Vale um teste curto.
                open dos apps: fopen e open da newlib usam flags diferentes 
                das do vfs.h, e o trap0.c ainda precisa traduzi-los. Eu 
                preparo isso quando você quiser ler arquivos de dentro de 
                um app.
##              .Resolvido
                └── Vídeo: hoje a saída dos apps vai só para a DUART. Se quiser
                    └── na VGA também, me diga e eu escrevo o write do console.
                        └── duartA_open: ele reinicializa o canal A a cada abertura.
                            └── Vale deixar a inicialização só no boot.


## 2026/10/12 kernel trap #0 funcionando corretamente
                Aplicação tsttrap.c funcionou completamente.
                vet32 (end 0x80)      = 0x000800B4
                [destino+0]         = 0x4EF90008
                [destino+4]         = 0x21AC4EF9
                [destino+8]         = 0x0000068E
                disparando trap #0 (getpid)
                voltou, d0            = 0x000003E8
                T3: fim 
                Erro: app travava ao entrar no ar.
                Funcionou: o write() e o printf da newlib de um app carregado 
                por spawn saíram na DUART. O caminho completo está provado, do 
                printf do app até o terminal, passando pelo TRAP #0.

                Os dois problemas que travavam eram do kernel, não da lib nem 
                da newlib: os slots começavam em 0x92000, dentro do heap, e o 
                vfs_open consultava o FAT antes dos dispositivos.

                A “escada” no terminal. Cada linha começa mais à direita porque 
                o /dev/tty0 (duartA_write) manda só \n, sem \r. O tty_write do 
                /dev/tty faz a conversão de \n para \r\n, e o tty_read faz o eco.
                Como o vfs_open agora consulta os dispositivos primeiro, o 
                /dev/tty deve funcionar para os apps, e é o dispositivo do 
                console que o seu VFS foi desenhado para usar.


## 2026/10/12 kernel e tstraw compilados com sucesso e rodando com sucesso
              tstraw provou que a aplicação anterior não estava rodando por
              que estava sendo compilada com newlib, por algum motivo isso faz
              travar. mas tstraw sem lib alguma funcionou e mostrou mensagem
              RAW OK
              main    = 0x000920B4
              global  = 0x0009215C
              sp      = 0x000D1FE8
 