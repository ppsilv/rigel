#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sys_vga.h>
#include <sys/color.h>
#include <sys/kmalloc.h>

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

typedef struct Window {
    int16_t x, y;
    int16_t width, height;
    char title[32];
    uint8_t active;
    struct Window *next; // Próxima janela na pilha (abaixo desta)
} Window;

static Window *window_stack_head = NULL; // Janela no topo (foco atual)

static int16_t old_x = 0, old_y = 0;
static Window *active_drag_window = NULL;
static int16_t drag_offset_x = 0;
static int16_t drag_offset_y = 0;
static int16_t global_mouse_x = 100; // Posição inicial X na tela
static int16_t global_mouse_y = 100; // Posição inicial Y na tela


#define MOUSE_STATUS (*(volatile uint8_t *)0xFF9101)
#define MOUSE_DATA   (*(volatile uint8_t *)0xFF9103)
#define CURSOR_X (*(volatile uint8_t *)0xFF9105)
#define CURSOR_Y (*(volatile uint8_t *)0xFF9107)


#define MOUSE_QUEUE_SIZE 16

typedef struct {
    uint8_t buttons;
    int8_t dx;
    int8_t dy;
    int8_t wheel;
} MouseEvent;

static MouseEvent mouse_queue[MOUSE_QUEUE_SIZE];
static volatile uint8_t mouse_head = 0;
static volatile uint8_t mouse_tail = 0;

// Agora coerente com a chamada: enqueue_mouse_event(packet);
void enqueue_mouse_event(uint8_t *packet) {
    // packet[0] é o byte de sincronismo (0x4D)
    uint8_t next = (mouse_head + 1) % MOUSE_QUEUE_SIZE;
    if (next != mouse_tail) {
        mouse_queue[mouse_head].buttons = packet[1];
        mouse_queue[mouse_head].dx      = (int8_t)packet[2];
        mouse_queue[mouse_head].dy      = (int8_t)packet[3];
        mouse_queue[mouse_head].wheel   = (int8_t)packet[4];
        mouse_head = next;
    }
}

int get_next_mouse_event(uint8_t *buttons, int8_t *dx, int8_t *dy) {
    if (mouse_head == mouse_tail) {
        return 0; // Fila vazia
    }
    *buttons = mouse_queue[mouse_tail].buttons;
    *dx = mouse_queue[mouse_tail].dx;
    *dy = mouse_queue[mouse_tail].dy;
    mouse_tail = (mouse_tail + 1) % MOUSE_QUEUE_SIZE;
    return 1;
}

typedef struct {
    uint8_t buttons; // Bit 0: Esquerdo, Bit 1: Direito
    int16_t delta_x; // Movimento horizontal
    int16_t delta_y; // Movimento vertical
} MousePacket;

// Dentro da sua Rotina de Interrupção (ISR):
void mouse_isr(void) {
    uint8_t packet[5];
    for (int i = 0; i < 5; i++) {
        packet[i] = MOUSE_DATA; // Lê os 5 bytes em sequência
    }
    // Envia o pacote para o ring buffer do kernel
    enqueue_mouse_event(packet);
}

void update_mouse_cursor(int16_t x, int16_t y) {
    // Limitando aos limites da tela e enviando via MMIO de 8 bits
    CURSOR_X = (uint8_t)x;
    CURSOR_Y = (uint8_t)y;
}


void draw_window_outline(int16_t x, int16_t y, int16_t width, int16_t height) {
    // Desenha as quatro linhas do retângulo da janela
    sys_drawhline(x, y, width, BLACK);             // Linha superior
    sys_drawhline(x, y + height, width, BLACK);    // Linha inferior
    sys_drawvline(x, y, height, BLACK);            // Linha esquerda
    sys_drawvline(x + width, y, height, BLACK);    // Linha direita
}

void draw_window(Window *win) {
    // 1. Corpo fundo da janela (Branco ou Cinza claro)
    sys_fillrect(win->x, win->y + 16, win->width, win->height - 16, CYAN);

    // 2. Barra de Título (Escura se ativa, cinza se inativa)
    color_t title_color = win->active ? BLUE : CYAN;
    sys_fillrect(win->x, win->y + 16, win->width, win->height - 16, title_color);

    // 3. Escreve o Título na barra
    sys_gotoxy(win->x + 4, win->y + 4);
    // (Chama a rotina de texto para imprimir win->title)

    // 4. Borda externa da janela
    sys_drawhline(win->x, win->y, win->width, BLACK);                 // Topo
    sys_drawhline(win->x, win->y + win->height, win->width, BLACK);    // Base
    sys_drawhline(win->x, win->y, win->height, BLACK);                 // Esquerda
    sys_drawhline(win->x + win->width, win->y, win->height, BLACK);    // Direita
}

void draw_stack_bottom_to_top(Window *win) {
    if (win == NULL) {
        return; // Chegou ao fim da pilha (fundo)
    }
    
    // Vai primeiro para o próximo (fundo da pilha)
    draw_stack_bottom_to_top(win->next);
    
    // Desenha a janela no caminho de volta (do fundo para o topo)
    draw_window(win);
}


void handle_outline_drag(Window *win, uint8_t buttons, int16_t mouse_x, int16_t mouse_y) {
    // 1. Início do Arraste
    if ((buttons & 0x01) && !active_drag_window) {
        if (mouse_x >= win->x && mouse_x <= (win->x + win->width) &&
            mouse_y >= win->y && mouse_y <= (win->y + 16)) {
            
            active_drag_window = win;
            drag_offset_x = mouse_x - win->x;
            drag_offset_y = mouse_y - win->y;
            
            old_x = win->x;
            old_y = win->y;
            // Desenha o contorno inicial
            draw_window_outline(old_x, old_y, win->width, win->height);
        }
    }

    // 2. Durante o Arraste (Movendo o mouse)
    if ((buttons & 0x01) && active_drag_window) {
        int16_t new_x = mouse_x - drag_offset_x;
        int16_t new_y = mouse_y - drag_offset_y;

        if (new_x != old_x || new_y != old_y) {
            // Apaga o contorno antigo (desenhando por cima com a cor do fundo/desktop)
            draw_window_outline(old_x, old_y, win->width, win->height);
            
            // Atualiza para a nova posição
            old_x = new_x;
            old_y = new_y;
            
            // Desenha o novo contorno
            draw_window_outline(old_x, old_y, win->width, win->height);
        }
    }

    // 3. Fim do Arraste (Soltou o botão)
    if (!(buttons & 0x01) && active_drag_window) {
        // Apaga o último contorno
        draw_window_outline(old_x, old_y, win->width, win->height);
        
        // Move a janela de fato para a posição final e a redesenha inteira
        active_drag_window->x = old_x;
        active_drag_window->y = old_y;
        draw_window(active_drag_window);
        
        active_drag_window = NULL;
    }
}


void bring_to_front(Window *win) {
    if (window_stack_head == win) return; // Já está no topo

    // Remove da posição atual na lista
    Window *curr = window_stack_head;
    while (curr && curr->next != win) {
        curr = curr->next;
    }
    
    if (curr && curr->next == win) {
        curr->next = win->next; // Desconecta
    }

    // Coloca no topo da pilha
    win->next = window_stack_head;
    window_stack_head = win;

    // Atualiza o estado de foco visual
    // (Desativa as outras, ativa a nova)
    Window *temp = window_stack_head;
    while (temp) {
        temp->active = (temp == window_stack_head);
        draw_window(temp); // Redesenha com a barra de título correta
        temp = temp->next;
    }
}

void redraw_desktop(void) {
    // 1. Limpa a tela com a cor de fundo do Desktop (ex: cinza claro ou padrão Lisa)
    sys_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PINK);

    // 2. Função auxiliar recursiva ou iterativa para desenhar da última até a primeira
    // (Exemplo simplificado percorrendo um array ou invertendo a pilha temporariamente)
    draw_stack_bottom_to_top(window_stack_head);
}



void process_mouse_and_windows(void) {
    uint8_t buttons;
    int8_t dx, dy;
    if (!get_next_mouse_event(&buttons, &dx, &dy)) {
        return; // Sem eventos novos
    }

    // Atualiza posição do cursor no PicoVGA via MMIO
    global_mouse_x += dx;
    global_mouse_y += dy;
    update_mouse_cursor(global_mouse_x, global_mouse_y);

    int left_button_pressed = (buttons & 0x01);

    // 1. Botão Esquerdo Pressionado
    if (left_button_pressed && !active_drag_window) {
        Window *curr = window_stack_head;
        while (curr) {
            if (global_mouse_x >= curr->x && global_mouse_x <= (curr->x + curr->width) &&
                global_mouse_y >= curr->y && global_mouse_y <= (curr->y + curr->height)) {
                
                bring_to_front(curr);

                // Se clicou na barra de título (16px), inicia o arraste por contorno
                if (global_mouse_y <= (curr->y + 16)) {
                    active_drag_window = curr;
                    drag_offset_x = global_mouse_x - curr->x;
                    drag_offset_y = global_mouse_y - curr->y;
                    
                    old_x = curr->x;
                    old_y = curr->y;
                    draw_window_outline(old_x, old_y, curr->width, curr->height);
                }
                break;
            }
            curr = curr->next;
        }
    }

    // 2. Movendo com o botão pressionado (Arrastando)
    if (left_button_pressed && active_drag_window) {
        int16_t new_x = global_mouse_x - drag_offset_x;
        int16_t new_y = global_mouse_y - drag_offset_y;

        if (new_x != old_x || new_y != old_y) {
            draw_window_outline(old_x, old_y, active_drag_window->width, active_drag_window->height);
            old_x = new_x;
            old_y = new_y;
            draw_window_outline(old_x, old_y, active_drag_window->width, active_drag_window->height);
        }
    }

    // 3. Botão Solto (Fim do arraste)
    if (!left_button_pressed && active_drag_window) {
        draw_window_outline(old_x, old_y, active_drag_window->width, active_drag_window->height);
        active_drag_window->x = old_x;
        active_drag_window->y = old_y;
        redraw_desktop();
        active_drag_window = NULL;
    }
}


Window* create_window(int16_t x, int16_t y, int16_t width, int16_t height, const char *title) {
    Window *win = (Window *)kmalloc(sizeof(Window));
    if (!win) return NULL; // Falta de memória

    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
//    snprintf(win->title, sizeof(win->title), "%s", title);
    memcpy(win->title, title, sizeof(win->title));
    win->active = 0;

    // Insere no topo da pilha atual
    win->next = window_stack_head;
    window_stack_head = win;

    return win;
}

int main_window(void) {
    // Inicializações do hardware e barramento...

    // Cria a primeira janela (ex: X=50, Y=50, Largura=250, Altura=160)
    Window *my_win = create_window(50, 50, 250, 160, "rigelOS Terminal");

    // Traz para o frente, ativa e desenha
    bring_to_front(my_win);
    redraw_desktop();

    // Loop principal do sistema
//    while (1) {
//        process_mouse_and_windows();
//        // outras tarefas do kernel...
//    }
    
    return 0;
}
