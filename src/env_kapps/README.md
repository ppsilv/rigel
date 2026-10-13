O SYS_SPAWN agora recebe (path, argv) e devolve o pid, que é o id da task (1000+slot), ou -1 em erro. 
Também criei o SYS_WAITPID, para o shell esperar o filho terminar. Sem isso, shell e filho disputam o 
teclado. Não pude compilar nem rodar nada para m68k aqui, então o código está sem teste no hardware.

# Passo a passo (arquivos no spawn_new.tar.gz):

1. No syscall.h do kernel, acrescente #define SYS_WAITPID 0x004F depois de SYS_IOCTL (o ksyscall.h do pacote já traz isso).
2. Copie para o kernel estes arquivos, que substituem os atuais:
    . trap0.c
    . scheduler.c (ganhou a função OS_TaskAlive)
    . loader_elf.c (monta o bloco argc/argv no topo da pilha do filho; load_elf_task continua existindo com a mesma assinatura)
3. Adicione spawn.c ao Makefile do kernel. É um arquivo novo, que abre o ELF no SD e chama o loader.
4. Substitua a librigel pelo librigel.tar.gz. Ele traz o crt0_app.S novo, que chama main(argc, argv), 
    e os stubs rigel_spawn(path, argv) e rigel_waitpid(pid) regenerados. Depois rode make SYSCALL_H=.../syscall.h VFS_H=.../vfs.h.
5. Recompile todos os apps. O crt0 mudou, então o shell.elf antigo não recebe argumentos.