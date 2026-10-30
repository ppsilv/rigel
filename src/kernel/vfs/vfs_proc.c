#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include <sys/kmalloc.h>


extern uint32_t get_heap_size();
extern uint32_t get_heap_base();
extern uint32_t get_kernel_end();
extern uint32_t get_kernel_estack();
extern uint32_t get_kernel_heap_start();
extern uint32_t get_kernel_stack_size();
extern uint32_t get_kernel_stack_init();
extern uint32_t get_kernel_heap_end();
extern uint32_t get_kernel_stack_end();

extern uint32_t get_heap_free(void);
extern uint32_t get_heap_largest_free(void);
extern void kmalloc_init(void *base, uint32_t size);


// ============================================
// ESTRUTURA DE DADOS DO /proc
// ============================================
typedef struct {
    char *data;
    size_t size;
    size_t position;
} ProcData;

// ============================================
// FUNÇÕES AUXILIARES
// ============================================
uint32_t get_total_memory(void) { return ((get_heap_base()-524288)+get_heap_size()); }
uint32_t get_kfree_memory(void)  { return get_heap_size(); }
const char *get_cpu_name(void) { return "Motorola MC68K"; }
int get_cpu_clock(void) { return 16; }

extern uint32_t get_mem_avail();
extern uint32_t get_heap_largest_free();

// ============================================
// PROC_READ
// ============================================
int proc_read(File *file, void *buffer, size_t size) {
    ProcData *data = (ProcData*)file->private_data;
    if (!data) return -1;
    
    size_t available = data->size - data->position;
    size_t to_read = (size < available) ? size : available;
    
    if (to_read == 0) return 0;
    
    memcpy(buffer, data->data + data->position, to_read);
    data->position += to_read;
    file->position = data->position;
    
    return (int)to_read;
}

// ============================================
// PROC_CLOSE
// ============================================
int proc_close(File *file) {
    ProcData *data = (ProcData*)file->private_data;
    if (data) {
        kfree(data->data);
        kfree(data);
        file->private_data = NULL;
    }
    return 0;
}

// ============================================
// PROC_LSEEK
// ============================================
size_t proc_lseek(File *file, size_t offset, int whence) {
    ProcData *data = (ProcData*)file->private_data;
    if (!data) return (size_t)-1;
    
    size_t new_pos;
    switch (whence) {
        case 0: new_pos = offset; break;
        case 1: new_pos = data->position + offset; break;
        case 2: new_pos = data->size + offset; break;
        default: return (size_t)-1;
    }
    
    if (new_pos > data->size) new_pos = data->size;
    data->position = new_pos;
    file->position = new_pos;
    return new_pos;
}
extern void heap_dump(uint32_t max);
// ============================================
// PROC_MEMINFO_OPEN
// ============================================
int proc_meminfo_open(File *file, const char *path, int flags) {
    ProcData *data = (ProcData*)kmalloc(sizeof(ProcData));
    if (!data) return -1;
    
    char buffer[256];
    ksprintf(buffer,
        "Kernel code end: %d KB\n"
        "Heap start.....: %d KB\n"
        "Heap end.......: %d KB\n"
        "Stack init.....: %d KB\n"
        "Stack end......: %d KB\n"
        "Stack size.....: %d KB\n"
        "MemTotal.......: %d KB\n"
        "Heap mem. free.: %d KB\n"
        "Largest block..:  KB\n"
        "MemAvail: %d KB\n",
        get_kernel_end(),
        get_kernel_heap_start(),
        get_kernel_heap_end(),
        get_kernel_stack_init(),
        get_kernel_stack_end(),
        get_kernel_stack_size(),
        get_total_memory(),
        get_heap_free(),
        
        get_mem_avail()
    );
    
    heap_dump(40);

    data->data = strdup(buffer);
    data->size = strlen(buffer);
    data->position = 0;
    
    file->private_data = data;
    file->position = 0;
    file->read = proc_read;
    file->write = NULL;
    file->close = proc_close;
    file->ioctl = NULL;
    file->lseek = proc_lseek;
    
    return 0;
}

// ============================================
// PROC_CPUINFO_OPEN
// ============================================
int proc_cpuinfo_open(File *file, const char *path, int flags) {
    ProcData *data = (ProcData*)kmalloc(sizeof(ProcData));
    if (!data) return -1;
    
    char buffer[256];
    ksprintf(buffer,
        "CPU:       %s\n"
        "Clock:     %d MHz\n"
        "Features:  FPU (sim)\n"
        "Cores:     1\n"
        "Family:    m68k\n",
        get_cpu_name(),
        get_cpu_clock()
    );
    
    data->data = strdup(buffer);
    data->size = strlen(buffer);
    data->position = 0;
    
    file->private_data = data;
    file->position = 0;
    file->read = proc_read;
    file->write = NULL;
    file->close = proc_close;
    file->ioctl = NULL;
    file->lseek = proc_lseek;
    
    return 0;
}

// ============================================
// PROC_UPTIME_OPEN
// ============================================
int proc_uptime_open(File *file, const char *path, int flags) {
    ProcData *data = (ProcData*)kmalloc(sizeof(ProcData));
    if (!data) return -1;
    
    static int uptime = 0;
    uptime += 10;
    
    char buffer[64];
    ksprintf(buffer, "%d seconds\n", uptime);
    
    data->data = strdup(buffer);
    data->size = strlen(buffer);
    data->position = 0;
    
    file->private_data = data;
    file->position = 0;
    file->read = proc_read;
    file->write = NULL;
    file->close = proc_close;
    file->ioctl = NULL;
    file->lseek = proc_lseek;
    
    return 0;
}