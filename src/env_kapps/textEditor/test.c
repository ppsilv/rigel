/*
   te: text editor for rigelOS
                               clear;clang -pthread te.c -o t
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// Definições de códigos para as teclas de seta e ESC
#define KEY_UP    0x80
#define KEY_DOWN  0x81
#define KEY_RIGHT 0x82
#define KEY_LEFT  0x83
#define KEY_DEL   0x84
#define KEY_INS   0x85
#define KEY_HOME  0x86
#define KEY_END   0x87
#define KEY_PUP   0x88
#define KEY_PDW   0x89
#define KEY_ESC   0x1B
#define KEY_BS    0x8A
#define KEY_TABS  0x09
#define KEY_ENTER 0x0A
#define KEY_CTRLW 0x11
#define KEY_CTRLL 0x0C

#define TAB_VALUE 4

//#define screen_size 80*30
#define buffer_size 64*1024

static char buffer_editor[buffer_size];
static const int width = 80;
static const int height= 30;
static const int screen_size = width * height;
static int cursor_x=0;
static int cursor_y=0,line=0;
static int janela=0; //endereço do primeiro byte dentro do buffer_editor para a janela 
static volatile unsigned char text_changed=0;
static int  max_size=0;

// Sincronização para a thread de vídeo não gastar CPU à toa
static mtx_t mutex_tela;
static cnd_t cond_tela;

static char filename[64]={0};

int getkey(void);

static int bkpcursor_x=0;
static int bkpcursor_y=0;

static void mover_cursor(int x, int y);
static void ler_nome_arquivo(char *destino, size_t tamanho_max);


// ---- Modelo: buffer = texto sequencial; 'janela' = offset (bytes) da 1a linha de TELA visivel ----
#define LINHAS_TELA (height-2)

// Monta uma linha de tela (80 colunas) a partir de 'off'. Trata \n, \r, TAB e quebra em 80 colunas.
// Retorna o offset do inicio da proxima linha de tela.
// tx >= 0: em *toff devolve o offset do byte que ocupa a coluna tx (-1 se a linha nao chega la).
static int ml_fim;   // preenchido por monta_linha: offset onde o texto da linha termina (no '\n' ou no corte)
static int monta_linha(int off, char *linha, int *ncols, int tx, int *toff){
    int n=0, p=off, fim=-1;
    if(toff) *toff=-1;
    while(n<width && p<buffer_size){
        unsigned char c=buffer_editor[p++];
        if(c=='\n'){ fim=p-1; break; }
        if(c=='\r') continue;
        int w=1;
        if(c=='\t') w=TAB_VALUE-(n%TAB_VALUE);
        if(w>width-n) w=width-n;
        if(toff && tx>=n && tx<n+w) *toff=p-1;
        if(c=='\t') memset(linha+n,' ',w);
        else linha[n]=(c<0x20)?'.':c;
        n+=w;
    }
    ml_fim=(fim<0)?p:fim;
    if(n==width && p<buffer_size && buffer_editor[p]=='\n') p++;
    if(ncols) *ncols=n;
    memset(linha+n,' ',width-n);
    return p;
}

// Offset do inicio da linha de tela numero y (0 = primeira visivel)
static int inicio_linha_tela(int y){
    char tmp[80];
    int off=janela;
    while(y-- > 0){
        int nx=monta_linha(off,tmp,NULL,-1,NULL);
        if(nx<=off) break;
        off=nx;
    }
    return off;
}

// Offset do inicio da linha de tela imediatamente antes de 'off'
static int linha_anterior(int off){
    char tmp[80];
    if(off<=0) return 0;
    int ls=off-1;
    while(ls>0 && buffer_editor[ls-1]!='\n') ls--;   // inicio da linha logica
    int r=ls;
    for(;;){
        int nx=monta_linha(r,tmp,NULL,-1,NULL);
        if(nx>=off || nx<=r) return r;
        r=nx;
    }
}

static void rolar_para(int novo){
    mtx_lock(&mutex_tela);
    janela=novo;
    text_changed=1;
    mtx_unlock(&mutex_tela);
    cnd_signal(&cond_tela);
}
static int rolar_baixo_f(int forca){
    char tmp[80];
    int nx=monta_linha(janela,tmp,NULL,-1,NULL);
    if(nx<=janela || (!forca && nx>=max_size)) return 0;   // fim do arquivo
    rolar_para(nx);
    return 1;
}
static int rolar_baixo(void){ return rolar_baixo_f(0); }
static int rolar_cima(void){
    if(janela<=0) return 0;                    // inicio do arquivo
    rolar_para(linha_anterior(janela));
    return 1;
}

// Insere 'len' bytes no offset 'off' do buffer (chamar com mutex_tela travado)
static int inserir_bytes(int off, const char *src, int len){
    if(off<0 || len<=0 || off+len>buffer_size) return 0;
    if(off>=max_size){                       // alem do texto: o buffer ja e' so espacos
        memcpy(buffer_editor+off,src,len);
        max_size=off+len;
        return 1;
    }
    if(max_size+len>buffer_size) return 0;   // buffer cheio
    memmove(buffer_editor+off+len,buffer_editor+off,max_size-off);
    memcpy(buffer_editor+off,src,len);
    max_size+=len;
    return 1;
}
// Apaga 1 byte no offset 'off' (chamar com mutex_tela travado)
static void apaga_byte(int off){
    if(off<0 || off>=max_size) return;
    memmove(buffer_editor+off,buffer_editor+off+1,max_size-off-1);
    buffer_editor[max_size-1]=0x20;
    max_size--;
}
static void fim_edicao(void){               // destrava e manda redesenhar
    text_changed=1;
    mtx_unlock(&mutex_tela);
    cnd_signal(&cond_tela);
}

// Insere um caractere na posicao do cursor
static void printchar(char ch){
    char tmp[80], buf[80];
    int off, n;
    mtx_lock(&mutex_tela);
    monta_linha(inicio_linha_tela(cursor_y),tmp,&n,cursor_x,&off);
    if(off>=0){
        inserir_bytes(off,&ch,1);
    }else{                                   // cursor alem do fim da linha: completa com espacos
        int pad=cursor_x-n;
        if(pad<0) pad=0;
        memset(buf,0x20,pad);
        buf[pad]=ch;
        inserir_bytes(ml_fim,buf,pad+1);
    }
    fim_edicao();
}

void printcursores(){
    bkpcursor_x = cursor_x;
    bkpcursor_y = cursor_y;

    mover_cursor(0,30);
    printf ("col: %02d line: %02d ",cursor_x,line);
    mover_cursor(cursor_x,cursor_y);    
}
void esconder_cursor(void) {
    printf("\033[?25l");
    fflush(stdout);
}

void mostrar_cursor(void) {
    printf("\033[?25h");
    fflush(stdout);
}
// Função para mover o cursor do terminal
static void mover_cursor(int x, int y) {
    // Atenção: o padrão ANSI geralmente espera (Linha, Coluna), 
    // ou seja, (y + 1, x + 1) se você usa coordenadas a partir de 0.
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout); // Garante que o comando seja enviado imediatamente para a tela
}
// Posiciona o cursor do terminal (travando para nao brigar com a thread de desenho)
static void atualiza_cursor(void){
    mtx_lock(&mutex_tela);
    printcursores();
    mover_cursor(cursor_x,cursor_y);
    mtx_unlock(&mutex_tela);
}
static void move_cursor_down_f(int forca){
    if( cursor_y < LINHAS_TELA-1 ) {
        cursor_y++;
    }
    else rolar_baixo_f(forca);          // forca=1: acompanha a digitacao alem do fim do texto
    atualiza_cursor();
    line++; 
    printcursores();
}
static void move_cursor_down(){ move_cursor_down_f(0); }
static void move_cursor_up(){
    if( cursor_y > 0 ) {
        cursor_y--;
    }
    else rolar_cima();
    atualiza_cursor();
    if(line > 0)
        line--;
    printcursores();
}
void move_cursor_right(){
    cursor_x++;
    if( cursor_x >= width ){
        cursor_x=0;
        move_cursor_down_f(1);
        return;
    }
    atualiza_cursor();
}
void move_cursor_left(){
    if( cursor_x > 0 ) cursor_x--;
    atualiza_cursor();
}
void move_cursor_tabs(){
    cursor_x+=TAB_VALUE;
    if( cursor_x >= width ){
        cursor_x=0;
        move_cursor_down_f(1);
        return;
    }
    atualiza_cursor();
}
// Enter: quebra a linha na posicao do cursor
static void inserir_enter(void){
    char tmp[80];
    int off, n;
    mtx_lock(&mutex_tela);
    monta_linha(inicio_linha_tela(cursor_y),tmp,&n,cursor_x,&off);
    if(off<0) off=ml_fim;                   // cursor no fim da linha
    inserir_bytes(off,"\n",1);
    fim_edicao();
    cursor_x=0;
    move_cursor_down_f(1);
}
// Backspace: apaga o caractere antes do cursor; na coluna 0 junta com a linha de cima
void move_cursor_bs(){
    char tmp[80];
    int off, n;
    if( cursor_x > 0 ){
        cursor_x--;
        mtx_lock(&mutex_tela);
        monta_linha(inicio_linha_tela(cursor_y),tmp,&n,cursor_x,&off);
        if(off>=0) apaga_byte(off);
        fim_edicao();
        atualiza_cursor();
        return;
    }
    int cur=inicio_linha_tela(cursor_y);
    if( cur <= 0 ) return;                  // inicio do arquivo
    if( cursor_y > 0 ) {
        cursor_y--; line--;
    }
    else rolar_cima();
    mtx_lock(&mutex_tela);
    monta_linha(inicio_linha_tela(cursor_y),tmp,&n,-1,NULL);
    cursor_x = (n<width)?n:width-1;         // fim da linha de cima
    apaga_byte(cur-1);                      // o '\n' (ou o ultimo caractere, se a linha quebrou sozinha)
    fim_edicao();
    atualiza_cursor();
}
// Delete: apaga o caractere sob o cursor; no fim da linha junta com a de baixo
static void delete_key(void){
    char tmp[80];
    int off, n;
    mtx_lock(&mutex_tela);
    monta_linha(inicio_linha_tela(cursor_y),tmp,&n,cursor_x,&off);
    if(off>=0) apaga_byte(off);
    else if(ml_fim<max_size && buffer_editor[ml_fim]=='\n') apaga_byte(ml_fim);
    fim_edicao();
}
static void page_down(){
    for(int i=0;i<LINHAS_TELA-1;i++) if(!rolar_baixo()) break;
    atualiza_cursor();
}
static void page_up(){
    for(int i=0;i<LINHAS_TELA-1;i++) if(!rolar_cima()) break;
    atualiza_cursor();
}
static void cursor_home(){
    cursor_x=0;
    atualiza_cursor();
}
static void cursor_end(){
    char tmp[80]; int n=0;
    monta_linha(inicio_linha_tela(cursor_y),tmp,&n,-1,NULL);
    cursor_x=(n<width)?n:width-1;
    atualiza_cursor();
}

static void clrscr(){
    printf("\033[2J\033[H");
    fflush(stdout);    
}
// Desenha a janela atual: 28 linhas de tela a partir do offset 'janela'
static void desenhar(void){
    char linha[80];
    int off=janela;
    for(int y=0;y<LINHAS_TELA;y++){
        int nx=monta_linha(off,linha,NULL,-1,NULL);
        mover_cursor(0,y);
        write(STDOUT_FILENO,linha,width);
        if(nx>off) off=nx;
    }
}

// Thread dedicada apenas a desenhar na tela quando houver mudança
int thread_renderizacao(void *arg) {
    memset(buffer_editor,0x20,buffer_size);
    while (1) {
        mtx_lock(&mutex_tela);
        // Dorme eficientemente até que o main avise que o texto mudou
        while (!text_changed) {
            cnd_wait(&cond_tela, &mutex_tela);
        }
        // --- HORA DE DESENHAR NO SEU FRAMEBUFFER/TELA ---
        // Aqui você varre o buffer_editor e joga para o vídeo do 68000
        // (ex: driver de vídeo mapeado em memória)
        // desenha_texto_na_tela(buffer_editor, cursor_x, cursor_y);
//        esconder_cursor();
        esconder_cursor();
        desenhar();
        mover_cursor(cursor_x, cursor_y);
        mostrar_cursor();
        // Reseta a flag indicando que a tela foi atualizada
        text_changed = 0;
       // move_cursor_right();
        mtx_unlock(&mutex_tela);
    }
    return 0;
}

#include <stdio.h>

int salvar_buffer(const char *nome_arquivo, const char *buffer, size_t tamanho) {
    char nomearquivo[65]={0};
    if( nome_arquivo == NULL ){
        ler_nome_arquivo(&nomearquivo[0], 64);
        if(strlen(nomearquivo) == 0)
            return -1;
    }else{
        strncpy(nomearquivo, nome_arquivo, sizeof(nomearquivo) - 1);
    }
    FILE *arquivo = fopen(nomearquivo, "w");
    if (arquivo == NULL) {
        return -1; // Erro ao abrir ou criar o arquivo
    }

    size_t escritos = fwrite(buffer, 1, tamanho, arquivo);
    fclose(arquivo);

    return (escritos == tamanho) ? 0 : -1;
}


// Carrega o arquivo para o buffer usando POSIX I/O
// Retorna a quantidade de bytes lidos ou -1 em caso de erro.
ssize_t carregar_arquivo(const char *nome_arquivo, char *buffer, size_t tamanho_maximo) {
    char nomearquivo[65]={0};
    if( nome_arquivo == NULL ){
        ler_nome_arquivo(&nomearquivo[0], 64);
        if(strlen(nomearquivo) == 0)
            return -1;
        nome_arquivo=&nomearquivo[0];
    }

    int fd = open(nome_arquivo, O_RDONLY);
    if (fd < 0) {
        return -1; // Erro ao abrir o arquivo
    }

    ssize_t total_lido = 0;
    ssize_t lidos_agora;

    while (total_lido < (ssize_t)tamanho_maximo) {
        lidos_agora = read(fd, buffer + total_lido, tamanho_maximo - total_lido);
        if (lidos_agora < 0) {
            close(fd);
            return -1; // Erro de leitura
        }
        if (lidos_agora == 0) {
            break; // Fim do arquivo (EOF)
        }
        total_lido += lidos_agora;
    }

    close(fd);
    return total_lido;
}
// No seu switch/tratamento:
// Configura o terminal para leitura caractere a caractere
static void configurar_terminal(struct termios *origem) {
    struct termios novo;
    tcgetattr(STDIN_FILENO, origem);
    novo = *origem;
    
    // Desliga ECHO e modo CANÔNICO
    novo.c_lflag &= ~(ECHO | ICANON);
    
    // Leitura bloqueante de pelo menos 1 byte
    novo.c_cc[VMIN] = 1;
    novo.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &novo);
}

// Restaura as configurações originais do terminal
static void restaurar_terminal(struct termios *origem) {
    tcsetattr(STDIN_FILENO, TCSANOW, origem);
}
int main(void) {
    thrd_t t_render;
    unsigned char btecla[2];
    struct termios terminal_original;

    configurar_terminal(&terminal_original);

    mtx_init(&mutex_tela, mtx_plain);
    cnd_init(&cond_tela);

    thrd_create(&t_render, thread_renderizacao, NULL);
    thrd_detach(t_render);

    printf("[OS-68k] Editor iniciado. Digite algo (ESC para sair):\n");
    clrscr();
    // Loop real de leitura de teclado
    while (1) {
        int tecla = getkey(); // Lê a tecla do terminal (bloqueia até você digitar algo)
        btecla[0] = tecla;
        if( tecla == KEY_CTRLL ){
            mover_cursor(0,30);
            printf ("Digite nome: ");
            mtx_lock(&mutex_tela);
            ssize_t n = carregar_arquivo(NULL,buffer_editor,buffer_size);
            if( n >= 0 ){
                memset(buffer_editor+n,0x20,buffer_size-n);   // limpa sobras do arquivo anterior
                max_size=n; janela=0; cursor_x=0; cursor_y=0;
            }
            text_changed = 1;
            mtx_unlock(&mutex_tela);
            cnd_signal(&cond_tela); 
        }
        if( tecla == KEY_CTRLW ){
            mover_cursor(0,30);
            printf ("Salvando o program:%d",max_size);
            salvar_buffer(NULL,buffer_editor,max_size);
            break;
        }    
        if( tecla == KEY_BS ){
            move_cursor_bs();
        }else if( tecla == KEY_RIGHT){
            move_cursor_right();
        }else if( tecla == KEY_LEFT){
            move_cursor_left();
        }else if( tecla == KEY_UP){
            move_cursor_up();
        }else if( tecla == KEY_DOWN){
            move_cursor_down();
        }else if( tecla == KEY_PUP){
            page_up();
        }else if( tecla == KEY_PDW){
            page_down();
        }else if( tecla == KEY_HOME){
            cursor_home();
        }else if( tecla == KEY_END){
            cursor_end();
        }else if( tecla == KEY_DEL){
            delete_key();
        }else if( tecla == KEY_ENTER){
            inserir_enter();
        }else if ( tecla == KEY_TABS){
            move_cursor_tabs();
        }else if( tecla >= 0x20 && tecla < 0x80){
            printchar(tecla);
            move_cursor_right();
        }
        if (tecla == 0x1B){
            break; // ESC para sair
        }
    }

    restaurar_terminal(&terminal_original);
    return 0;
}

// Lê uma tecla e interpreta sequências de escape
int getkey(void) {

    unsigned char c;
//    struct termios terminal_original;
//    configurar_terminal(&terminal_original);
    
    if (read(STDIN_FILENO, &c, 1) <= 0) {
         
        return -1;
    }
 //   restaurar_terminal(&terminal_original);

   // printf("|%02x|",c);
   // fflush(stdout);

    // O Backspace chega como byte único (0x7F ou 0x08)
    if (c == 0x7F || c == 0x08) {
        return KEY_BS;
    }
    if (c == 0x0D || c == 0x0A) {
        return KEY_ENTER;           
    }
    if (c == 0x09 ) {
        return KEY_TABS;           
    }
    if (c == 0x17 ) {
        return KEY_CTRLW;           
    }

    // Se não for ESC, retorna o caractere lido
    if (c != 0x1B) {
        return c;
    }
    //printf("0x1b ");
    //fflush(stdout);
   
    // Altera temporariamente VMIN/VTIME para verificar se há mais bytes (timeout de 100ms)
    struct termios atual, temp;
    tcgetattr(STDIN_FILENO, &atual);
    temp = atual;
    temp.c_cc[VMIN] = 0;
    temp.c_cc[VTIME] = 1; // 1 * 100ms
    tcsetattr(STDIN_FILENO, TCSANOW, &temp);

    unsigned char seq[2];
    int n1 = read(STDIN_FILENO, &seq[0], 1);
    int n2 = 0;
    //printf("|%02x|",n1);
    //fflush(stdout);

    if (n1 > 0) {
        n2 = read(STDIN_FILENO, &seq[1], 1);
        if (n2 > 0 && seq[0] == '[' && seq[1] >= '1' && seq[1] <= '8') {
            unsigned char til;
            read(STDIN_FILENO, &til, 1);   // consome o '~' final
        }
    }

    // Restaura o modo de leitura padrão
    tcsetattr(STDIN_FILENO, TCSANOW, &atual);

    // Se não houver bytes adicionais, foi pressionado apenas ESC
    if (n1 <= 0) {
        return KEY_ESC;
    }
    // Interpreta a sequência de escape "ESC [ X"
    if (seq[0] == '[') {
//        write(STDOUT_FILENO, &seq[1], 1);
        switch (seq[1]) {
            case 0x7F:
            case 0x08:
                      return KEY_BS;  
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            case '1': return KEY_HOME;
            case '4': return KEY_END;
            case '2': return KEY_INS;
            case '3': return KEY_DEL;
            case '5': return KEY_PUP;
            case '6': return KEY_PDW;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
        }
    
    }
    //printf("ESC");
    //fflush(stdout);
    return KEY_ESC;
}

void ler_nome_arquivo(char *destino, size_t tamanho_max) {
    size_t i = 0;
    printf("\nNome do arquivo: ");
    fflush(stdout);

    while (i < tamanho_max - 1) {
        int tecla = getkey();

        if (tecla == KEY_ENTER || tecla == 0x0A || tecla == 0x0D) {
            break;
        } 
        else if (tecla == KEY_BS) {
            if (i > 0) {
                i--;
                printf("\b \b"); // Apaga visualmente no terminal
                fflush(stdout);
            }
        } 
        else if (tecla >= 32 && tecla <= 126) { // Caracteres imprimíveis
            destino[i++] = (char)tecla;
            putchar(tecla); // Ecoa o caractere na tela
            fflush(stdout);
        }
    }
    destino[i] = '\0'; // Fecha a string
    //printf("arq:%s",destino);
    printf("\n");
}