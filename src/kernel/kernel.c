

#include <stdio.h>
#include <string.h>
#include <sys/interrupt.h>
#include <sys/critical.h>
#include "sys/scheduler.h"
#include "./exceptions/exceptions.h"
#include <sys/vfs.h>
#include "ringbuffer.h"
#include "w5100_netif.h"

FATFS FatFs; 

uint32_t tick_uptime=0;
uint32_t day_uptime=0;


/*
 * Teste isolado: duas tarefas bobas, cada uma so incrementando um
 * contador proprio e imprimindo de vez em quando. Se elas alternarem
 * no terminal, o context switch esta funcionando.
 *
 * Cada tarefa precisa da SUA PROPRIA pilha, alocada estaticamente
 * (sem malloc). O tamanho e' um chute razoavel pra uma tarefa simples
 * -- se sua tarefa usar buffers grandes na pilha (arrays locais, etc)
 * aumente esse valor. Estourar a pilha aqui e' silencioso (sem MMU),
 * entao se algo comportar estranho, suspeite disso primeiro.
 */
#define STACK_SIZE 1024

static uint8_t stack_task_a[STACK_SIZE];
static uint8_t stack_task_b[STACK_SIZE];
static uint8_t stack_task_c[STACK_SIZE];

void kernel_puts(const char *s);
void long_to_string(long value, char *str, int base);
extern int load_elf_task(FIL *fd, int argc, char *argv[]);


static void TaskA(void)
{
    uint32_t count = 0;
    char str[32];
    for (;;) {
        count++;
        if (count >= 100000) {
            long_to_string(count, str,16);
            count = 0;
            kernel_puts("\nTask A: ");
            kernel_puts(str);
        }
    }
}

static int tty_fd = -1;

void TaskA_Entry(void)
{
    char str[16];
    long count = 0;
    long print_counter = 0;

    tty_fd = vfs_open("/dev/tty0", O_RDWR);
    if (tty_fd < 0) {
        kernel_puts("TaskC: falha ao abrir /dev/tty0\n");
        for (;;)
            ;   /* trava aqui mesmo, sem tty nao da pra nem avisar melhor */
    }

    for (;;) {
        count++;
        if (++print_counter >= 25000) {
            print_counter = 0;
            long_to_string(count, str, 10);
            vfs_write(tty_fd, "\nTask C: ", 9);
            vfs_write(tty_fd, str, strlen(str));
        }
    }
}
static void TaskB(void)
{
    uint32_t count = 0;
    char str[32];
    uint32_t roof=150000;
    for (;;) {
        count++;
        if (count >= roof) {
            roof-=1000;
            if(roof<=10000) roof = 50000;
            kernel_puts("\nTask B: ");
            long_to_string(count, str,10);
            kernel_puts(str);
            count = 0;
        }
    }
}

void run_scheduler_test(void)
{

    /* arg = NULL porque essas tarefas nao precisam de nenhum dado externo */
    OS_TaskCreate(TaskA, NULL, stack_task_a, STACK_SIZE, /*id=*/1);
    OS_TaskCreate(TaskB, NULL, stack_task_b, STACK_SIZE, /*id=*/2);
    OS_TaskCreate(TaskA_Entry, NULL, stack_task_c, STACK_SIZE, /*id=*/3);
    

    OS_Start();   /* nunca retorna -- a partir daqui quem manda e' o OS_TickISR */

    /* nunca chega aqui */
}
extern int load_pic_elf_standalone(FIL *fd, int argc, char *argv[]);
extern int sys_spawn(const char *path, char **argv);

/*
O tipo de ELF: ET_EXEC, executável estático (não é ET_DYN/PIE). 
Ele é linkado no endereço 0 e guarda as relocações (-Wl,-q). 
O loader soma a base do slot em cada R_68K_32.
*/
void run_scheduler_test2(void)
{
    if (sys_spawn("/shell.elf", NULL) <= 0)
        kprintf("Falha ao dar spawn no /shell.elf\n");
    OS_Start();   /* so' agora entrega o controle pro scheduler */
}
/*
void run_scheduler_test2(void)
{
   // OS_TaskCreate(TaskA, NULL, stack_task_a, STACK_SIZE, 1);
   // OS_TaskCreate(TaskB, NULL, stack_task_b, STACK_SIZE, 2);
   // OS_TaskCreate(TaskA_Entry, NULL, stack_task_c, STACK_SIZE, 3);

   // teste do spawn via ELF 
   
    FIL test_fd;
    if (f_open(&test_fd, "/shell.elf", FA_READ) == FR_OK) {
        if (load_elf_task(&test_fd, 0, NULL) != 0) {
            kprintf("Falha ao dar spawn no duart.elf\n");
        }
    } else {
        kprintf("Nao achei /shell.elf no disco\n");
    }

    OS_Start();   // so' agora entrega o controle pro scheduler 
}
*/

extern void run_scheduler_test(void);
extern uint32_t __kernel_end;   /* símbolo do seu linker script, fim do .bss do kernel */
extern uint32_t __estack; 
extern void kmalloc_init(void *base, uint32_t size);
extern char __heap_start[], __heap_end[];
extern void OS_TickISR(void);
extern void Int3Handler(void);
extern void sys_setramvector(uint32_t stub_addr, uint32_t handler_addr);
void kernel_puts(const char *s);
extern void vfs_init(void);
extern int vfs_open(const char *path, int flags);


uint32_t systemTick=0;
uint32_t tick_count=0;
static void do_ideinit(int argc, char *argv[])
{
    FRESULT fr;
    fr = f_mount(&FatFs, "", 0);
    if (fr != FR_OK) {
        kputs("PANIC: Erro ao montar FAT\n");
    }else{
        kputs("FAT success mounted!\n");
    }
}

extern int trap0_entry(int num, long a1, long a2, long a3, long a4, long a5, long a6, long a7);
extern int trap0_handler(int num, long a1, long a2, long a3, long a4, long a5, long a6, long a7);


int kernel_main(){
    kernel_puts("\n\n\n\n");
    short saved_sr;

    uint32_t heap_base = (uint32_t)&__kernel_end;
    uint32_t heap_size = __estack - heap_base - 0x4000;  /* 16 KB de folga pra pilha */
    //kmalloc_init((void*)0x92000, 0x8000); ERRO FEIO PORQUE TEM VALOR CHUMBADO ESTÁTICAMENTE
    kmalloc_init(__heap_start, (uint32_t)(__heap_end - __heap_start));

    kernel_puts("Rigel 0.10 kernel 0.101\n");
    do_ideinit(0,NULL);

   kernel_puts("Ring buffer init\n");
   ring_buf_init();
//    kmalloc_init(&__kernel_end, 0x10000);   /* ajuste o tamanho pro que sobrar de RAM ali */


    kernel_puts("Scheduler init\n");
    OS_Init();
    kernel_puts("Posix system init\n");
    vfs_init();

    LOCK(saved_sr);
    sys_setramvector(0x000800b4,(uint32_t)trap0_entry);
    sys_setramvector(0x00080090,(uint32_t)OS_TickISR);
    sys_setramvector(0x00080096,(uint32_t)Int3Handler);
    sys_setramvector(0x00080008, (uint32_t)exc_bus_error);
    sys_setramvector(0x0008000C, (uint32_t)exc_address_error);
    sys_setramvector(0x00080010, (uint32_t)exc_illegal);
    sys_setramvector(0x00080014, (uint32_t)exc_div0);
    sys_setramvector(0x00080020, (uint32_t)exc_priv);
    sys_setramvector(0x0008001C, (uint32_t)exc_trapv);
    sys_setramvector(0x00080028, (uint32_t)exc_linea);
    sys_setramvector(0x0008002C, (uint32_t)exc_linef);

    for(int i=0;i<0xFFFF; i++){
        volatile int j = 0; 
         (void)j;       
    }
    UNLOCK(saved_sr);

    kernel_puts("Initializing ethernet\n ");
    if( rigel_net_start() ){
        kernel_puts("Initializing ethernet FAILED...\n ");
    }else{
        kernel_puts("Initializing ethernet SUCCESS...\n ");
    }

    kernel_puts("Loading process #1 init\n ");

    run_scheduler_test2();

    /*System never turn back from scheduler*/

    return 0;
}