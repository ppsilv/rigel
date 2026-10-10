#include <stdio.h>
#include <stdint.h>
#include "duarts.h"
#include "bootinfo.h"
#include "ff.h"

uint32_t tick_count=0;
uint32_t systemTick=0;

uint8_t *WR_CHAR=(uint8_t *)0xFF912B;

extern int boot_load_kernel(const char *path);
extern void boot_jump(void);

#include "ff.h"

static FATFS fs;                      /* area de trabalho do volume (fica no .bss) */

/* Monta o volume 0. Retorna 0 em sucesso, ou o codigo FRESULT. */
int boot_mount(void)
{
    FRESULT r = f_mount(&fs, "", 1);  /* opt=1: monta agora, nao so' registra */

    if (r != FR_OK) {
        printf("disco: f_mount falhou (FRESULT %d)\n", (int)r);
        return (int)r;
    }
    printf("disco: FAT montado\n");
    return 0;
}

int main(void)
{
    char nome[32];
    *WR_CHAR='A';
    
    setvbuf(stdout, NULL, _IONBF, 0);   /* sem buffer: saida aparece na hora */
    printf("\nRigel bootstrap 0.1 newlib for m68k\nMinimum hardware duart-disk0\n");
/*
    printf("Qual o seu nome? ");
    if (fgets(nome, sizeof nome, stdin)){
        printf("Ola, %s", nome);
    }
    printf("malloc funcionando: ");
    void *p = __builtin_malloc(100);
    printf("%s\n", p ? "sim" : "nao");
    fprintf(stdout,"Saida stdout\n");
    fprintf(stderr,"Saida stderr\n");
*/
    while(1){
        printf("Essa area pode ser usada\n para manutencao no futuro.\nPressione k para sair\n");
        if (fgets(nome, sizeof nome, stdin)){
            printf("Digitado:[%s]\n", nome);
        }
        if( nome[0] == 'k' )
            break;
    }
    /* depois de serial, video, disco e f_mount ja estarem ok */
    if (boot_mount()){
        printf("boot: no disk present..\n");
    }else if (boot_load_kernel(KERNEL_PATH) == BK_OK) {
        printf("saltando para o kernel...\n");
        boot_jump();
    }
    printf("boot falhou; ficando no monitor\n");
    while(1){
        printf("Digite algo que ecoo de volta...\n");
        if (fgets(nome, sizeof nome, stdin)){
            printf("%s\n", nome);
        }
    }


    return 0;
}
/*
1 - Serial (DUART). Já funciona. Mantém o banner e as mensagens de progresso.
2 - Vídeo. Inicialize e limpe a tela. Se falhar, o boot segue só pela serial.
3 - Disco. Inicialize, monte o FAT com o FatFS e abra /boot/kernel.bin.
    Conferir o tamanho. Pegue o tamanho do arquivo com f_size (ou o equivalente 
    do seu fileio.h). Recuse o boot se 0x82000 + tamanho passar do limite da 
    RAM livre, que é abaixo do topo da pilha (0xFFFF0 menos a pilha). Um arquivo 
    vazio também deve ser recusado.
    Ler direto para 0x82000. Leia o arquivo inteiro para o endereço fixo, em um 
    único fread ou em blocos. Verifique que o número de bytes lidos é igual ao 
    tamanho. Não há cabeçalho para interpretar, não há p_paddr, não há zeragem 
    de .bss pelo Rigel.Validar a imagem (opcional, mas recomendo). 
    Dois cuidados baratos:
    Confira um valor mágico no início do .bin, por exemplo 4 bytes 'ORIO' ou 'KRNL' 
    no começo do .text.start, antes do _start. Ele avisa se você carregou o arquivo 
    errado. Ou some um checksum simples. O Amiga faz isso com o bootblock.
4 - Preparar a bootinfo. Preencha a struct num endereço fixo (por exemplo 0x80700, 
    em área que o Rigel não usa) com: versão do Rigel, tamanho da RAM, base do DUART, 
    dados do vídeo, dispositivo de boot, tamanho do kernel e ponteiro para a tabela 
    de saltos. Passe também o ponteiro em A0.
5 - Mascarar IRQ e saltar. move.w #0x2700,%sr e depois jmp 0x82000. O crt0 do kernel 
    assume, coloca o próprio SP e zera o .bss.

Pontos que merecem atenção

1 - Falhas de boot: decida desde já o que acontece se o disco ou o kernel faltar. 
    Eu mostraria a mensagem na serial e cairia num mini-monitor, com comandos como md, 
    load e go. Isso salva muito tempo quando você estiver testando.
2 - Mensagens de progresso: imprima uma linha por etapa (serial ok, video ok, disk ok, 
    kernel 0x82000 N bytes). Se o boot travar, você vê onde.
3 - Interrupções: o Rigel deve deixá-las mascaradas (SR = 0x2700) no salto. 
    O kernel as habilita quando tiver os seus handlers prontos.
4 - Vetores: os vetores estão na ROM e apontam para os stubs em RAM (0x80000–0x803FF). 
    O kernel pode redirecioná-los reescrevendo esses stubs. Isso é uma convenção que
    vale registrar no README, para não se perder depois.
5 - Newlib dentro do Rigel: o heap, o printf e o FatFS consomem RAM, e depois do salto 
    o kernel não pode contar com nada disso. 
    Se o Rigel ficar na ROM, o kernel pode 
    continuar chamando o printf dele via tabela de saltos, o que economiza espaço.

*/