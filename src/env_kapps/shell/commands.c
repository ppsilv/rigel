#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"
#include <rigel/rigel.h>
#include <sys/vfs.h>

extern const cmd_entry_t g_cmd_table[];

void do_help(int argc, char *argv[])
{
	int i = 0;

	printf("\nAvailable commands:\n");

	while (g_cmd_table[i].name)
    {
		printf("%12s - %s\n", g_cmd_table[i].name, g_cmd_table[i].helpme);
		++i;
	}
}

//__attribute__((used, retain)) 
__attribute__((used)) void do_load(int argc, char *argv[]){
//No shell, use assim:
    char *args[] = { "hello.elf", "ola", "mundo", NULL };
    long pid = rigel_spawn("hello.elf", args);
    printf("spawn -> pid=%ld\n", pid);
    if (pid > 0) {
       rigel_waitpid(pid);
       printf("filho %ld terminou\n", pid);
    }
    
/*
O teste_spawn.c do pacote faz isso, lançando um hello.elf e esperando por ele.

Limites:

No máximo 16 argumentos e 1 KB de texto; passando disso o spawn devolve -1.
Só um spawn roda por vez (há um flag de bloqueio).
O waitpid espera em laço ocupado e devolve sempre 0, sem o código de saída do filho.
O resto do kernel continua sem proteção contra duas tasks usando o FatFS ao mesmo tempo. Se o filho abrir arquivos enquanto o shell também lê o SD, pode dar problema.
*/

}

static void dump_memory(long addr,int size){
    unsigned char * pcharhex;
    unsigned char * pcharasc;
    unsigned char ch;

    pcharhex = (unsigned char *)addr;
    while(1){
        // Inicializa os ponteiros baseado no 'addr' ATUAL da página
        
        //clrscr();
        int total_linhas = size /16;   
        
        printf("----------------------------------------------------------------------------\n");
        printf("Dumping %03x bytes from addr %08X total_linhas = [%d]\n",size, pcharhex,total_linhas);
        printf("Address   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F -             ASCII\n");
        printf("----------------------------------------------------------------------------\n");
        
        for(int j=0; j<total_linhas; j++){
            // Imprime o endereço real dessa linha específica
            printf("%08X  ", (unsigned int)pcharhex);
            // Imprime os 16 bytes em Hexa
            for(int i = 0; i<16; i++){
                printf("%02x ", *pcharhex++);            
            }
            printf("- ");
            pcharasc = pcharhex - 16;
            for(int i = 0; i<16; i++){
                ch = *pcharasc++;
                if( ch >= 0x20 && ch < 0x80 ) // Mudado para >= 0x20 para incluir o caractere de Espaço
                    printf("%c", ch);            
                else    
                    printf(".");            
            }
            printf("\n");
        }
        //dump_registradores();
        printf("Hit any <ENTER> to continue <ESC> to terminate: \n");
        ch = rigel_getkbdkey();
        if( ch == 0x1B ){ // ESC termina
            printf("\n");
            return;
        }
    }
}
void do_dump(int argc, char *argv[]){
    unsigned long start, count;

    //printf("numargs=%d args[0]=%s,args[1]=%s,args[2]=%s\n",argc,argv[0],argv[1],argv[2]);

    start = strtoul((const char *)argv[0], NULL, 16);
    count = strtoul((const char *)argv[1], NULL, 16);

    //printf("do_dump [%lu] [%lu]",start,count);

    dump_memory(start, count);
}
/*
— SEEK_SET: Set the offset to the provided value.
— SEEK_CUR: Move the offset from the current position.
— SEEK_END: Set the offset from the end of the file.
*/
void do_cat(int argc, char *argv[])
{
    int fd;
    int offset=0;
    int whence=SEEK_SET;
    char arquivo[16];
    char buffer_leitura[512];

    sprintf(arquivo,"%s",argv[0]);

    fd = open( arquivo, O_RDWR );
    if (fd < 1) {
        printf("Erro ao abrir %s \n",argv[0]);
        return;
    }    
    printf("--------------------------------------------------\n");
    lseek( fd, offset, whence);
    int bytes_lidos=0;
    do {
        bytes_lidos = read(fd, buffer_leitura, 512 - 1);
        if (bytes_lidos < 0) {
            printf("\n[Erro durante a leitura do arquivo: %d]\n", bytes_lidos);
            break;
        }else if (bytes_lidos == 0){
            break;    
        }else {
            buffer_leitura[bytes_lidos] = '\0';            
            printf("%s", buffer_leitura);
        }
    } while (bytes_lidos > 0); // Enquanto ler mais que 0 bytes, o arquivo não acabou
    printf("--------------------------------------------------\n");

    close(fd);
}
void do_uptime(int argc, char *argv[]){
    printf("uptime for %d day(s)\n",rigel_getuptime());
}

void do_ls(int argc, char *argv[]){
    const char *path = (argc > 1) ? argv[1] : "/";

    long d = rigel_opendir((char *)path);
    if (d < 0) {
        printf("ls: nao abriu '%s'\n", path);
        return ;;
    }

    rigel_dirent_t e;
    int files = 0, dirs = 0;
    unsigned long total = 0;

    while (rigel_readdir(d, &e) == 1) {
        int ano = 1980 + (e.date >> 9);
        int mes = (e.date >> 5) & 15;
        int dia = e.date & 31;
        int hor = e.time >> 11;
        int min = (e.time >> 5) & 63;

        if (e.attr & RIGEL_ATTR_DIR) {
            printf("%04d-%02d-%02d %02d:%02d  <DIR>        %s\n", ano, mes, dia, hor, min, e.name);
            dirs++;
        } else {
            printf("%04d-%02d-%02d %02d:%02d  %10lu  %s\n", ano, mes, dia, hor, min,
                   (unsigned long)e.size, e.name);
            files++;
            total += e.size;
        }
    }
    rigel_closedir(d);

    printf("%d arquivo(s), %lu bytes, %d diretorio(s)\n", files, total, dirs);    
}