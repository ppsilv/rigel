#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sys_vga.h>
#include <sys/color.h>
#include <sys/kmalloc.h>
#include "vga_gfx.h"

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define TITLE_HEIGHT    16

/* Fonte do Pico: 8x16. O texto usa coluna/linha, então a janela deve
 * ficar em x múltiplo de 8 e y múltiplo de 16 para a barra de título
 * coincidir com uma linha de texto. */
#define CHAR_W          8
#define CHAR_H          16
/* Cor do texto do título (0..15). Troque se ficar ruim de ler. */
#define TITLE_TEXT_COLOR BLACK

typedef struct Window {
    int16_t x, y;
    int16_t width, height;
    char title[32];
    uint8_t active;
    struct Window *next; // Próxima janela na pilha (abaixo desta)
} Window;

/* ------------------------------------------------------------------ */
/* Estado global                                                       */
/* Tudo é (re)inicializado em window_init(), então não depende do      */
/* startup ter zerado o .bss ou copiado o .data.                       */
/* ------------------------------------------------------------------ */
static Window *window_stack_head = NULL;  // Janela no topo (foco atual)
static Window *active_drag_window = NULL;
static int16_t drag_offset_x = 0;
static int16_t drag_offset_y = 0;
static int16_t drag_x = 0;                // posição atual do contorno
static int16_t drag_y = 0;
static int16_t global_mouse_x = 100;
static int16_t global_mouse_y = 100;

#define MOUSE_STATUS (*(volatile uint8_t *)0xFF9101)
#define MOUSE_DATA   (*(volatile uint8_t *)0xFF9103)
#define CURSOR_X     (*(volatile uint8_t *)0xFF9105)
#define CURSOR_Y     (*(volatile uint8_t *)0xFF9107)

#define MOUSE_QUEUE_SIZE 16

typedef struct {
    uint8_t buttons;
    int8_t dx;
    int8_t dy;
    int8_t wheel;
} MouseEvent;

/* A fila é escrita na ISR e lida no laço principal: tudo volatile. */
static volatile MouseEvent mouse_queue[MOUSE_QUEUE_SIZE];
static volatile uint8_t mouse_head = 0;
static volatile uint8_t mouse_tail = 0;

/* ------------------------------------------------------------------ */
/* Fila de eventos do mouse                                            */
/* ------------------------------------------------------------------ */
void enqueue_mouse_event(const uint8_t *packet) {
    // packet[0] é o byte de sincronismo (0x4D)
    uint8_t next = (uint8_t)((mouse_head + 1) % MOUSE_QUEUE_SIZE);
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
    *dx      = mouse_queue[mouse_tail].dx;
    *dy      = mouse_queue[mouse_tail].dy;
    mouse_tail = (uint8_t)((mouse_tail + 1) % MOUSE_QUEUE_SIZE);
    return 1;
}

/* Rotina de interrupção. O atributo faz o GCC m68k salvar os registradores
 * e terminar com RTE em vez de RTS. Se o seu vetor de interrupção já aponta
 * para um wrapper em assembly que faz o RTE, remova o atributo. */
__attribute__((interrupt))
void mouse_isr(void) {
    uint8_t packet[5];
    for (int i = 0; i < 5; i++) {
        packet[i] = MOUSE_DATA; // Lê os 5 bytes em sequência
    }
    enqueue_mouse_event(packet);
}

void update_mouse_cursor(int16_t x, int16_t y) {
    // Envia via MMIO de 8 bits
    CURSOR_X = (uint8_t)x;
    CURSOR_Y = (uint8_t)y;
}

/* ------------------------------------------------------------------ */
/* Desenho                                                             */
/* ------------------------------------------------------------------ */
void draw_window_outline(int16_t x, int16_t y, int16_t width, int16_t height) {
    vga_hline(x, y, width, BLACK);             // Linha superior
    vga_hline(x, y + height, width, BLACK);    // Linha inferior
    vga_vline(x, y, height, BLACK);            // Linha esquerda
    vga_vline(x + width, y, height, BLACK);    // Linha direita
}

void draw_window(Window *win) {
    // 1. Corpo da janela (abaixo da barra de título)
    vga_fillrect(win->x, win->y + TITLE_HEIGHT,
                 win->width, win->height - TITLE_HEIGHT, CYAN);

    // 2. Barra de título (só a faixa de cima)
    color_t title_color = win->active ? BLUE : CYAN;
    vga_fillrect(win->x, win->y, win->width, TITLE_HEIGHT, title_color);

    // 3. Texto do título
    // (texto usa coluna/linha; o fundo do texto fica da cor da barra)
    vga_settextcolor(TITLE_TEXT_COLOR, title_color);
    vga_gotoxy(win->x / CHAR_W + 1, win->y / CHAR_H);
    vga_puts(win->title);

    // 4. Borda externa
    vga_hline(win->x, win->y, win->width, BLACK);                  // Topo
    vga_hline(win->x, win->y + win->height, win->width, BLACK);    // Base
    vga_vline(win->x, win->y, win->height, BLACK);                 // Esquerda
    vga_vline(win->x + win->width, win->y, win->height, BLACK);    // Direita
}

void draw_stack_bottom_to_top(Window *win) {
    if (win == NULL) {
        return; // Chegou ao fim da pilha (fundo)
    }
    draw_stack_bottom_to_top(win->next); // primeiro o fundo
    draw_window(win);                    // depois volta desenhando até o topo
}

void redraw_desktop(void) {
    vga_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PINK);
    draw_stack_bottom_to_top(window_stack_head);
}

/* ------------------------------------------------------------------ */
/* Pilha de janelas                                                    */
/* ------------------------------------------------------------------ */

/* Coloca a janela no topo e marca só ela como ativa.
 * Funciona tanto para janela já na lista quanto para janela nova.
 * NÃO desenha nada: quem chama decide quando chamar redraw_desktop(). */
void bring_to_front(Window *win) {
    if (win == NULL) return;

    if (window_stack_head != win) {
        // Desconecta da posição atual (se estiver na lista)
        Window **pp = &window_stack_head;
        while (*pp != NULL && *pp != win) {
            pp = &(*pp)->next;
        }
        if (*pp == win) {
            *pp = win->next;
        }
        // Insere no topo
        win->next = window_stack_head;
        window_stack_head = win;
    }

    for (Window *w = window_stack_head; w != NULL; w = w->next) {
        w->active = (w == window_stack_head);
    }
}

Window *create_window(int16_t x, int16_t y, int16_t width, int16_t height,
                      const char *title) {
    Window *win = (Window *)kmalloc(sizeof(Window));
    if (win == NULL) return NULL; // Falta de memória

    memset(win, 0, sizeof(Window));
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;

    // Cópia segura: nunca lê além do texto, sempre termina com '\0'
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = '\0';

    win->active = 0;
    win->next = NULL;

    bring_to_front(win); // insere no topo e ativa
    return win;
}

/* ------------------------------------------------------------------ */
/* Mouse + arraste por contorno                                        */
/* ------------------------------------------------------------------ */
static int16_t clamp16(int16_t v, int16_t lo, int16_t hi) {
    if (hi < lo) hi = lo;
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void handle_mouse_event(uint8_t buttons, int8_t dx, int8_t dy) {
    // Atualiza e limita a posição do cursor
    global_mouse_x = clamp16(global_mouse_x + dx, 0, SCREEN_WIDTH - 1);
    global_mouse_y = clamp16(global_mouse_y + dy, 0, SCREEN_HEIGHT - 1);
    update_mouse_cursor(global_mouse_x, global_mouse_y);

    int left = (buttons & 0x01);

    if (left && !active_drag_window) {
        // 1. Botão pressionado: procura a janela clicada, do topo para o fundo
        for (Window *curr = window_stack_head; curr != NULL; curr = curr->next) {
            if (global_mouse_x >= curr->x && global_mouse_x <= (curr->x + curr->width) &&
                global_mouse_y >= curr->y && global_mouse_y <= (curr->y + curr->height)) {

                if (curr != window_stack_head) {
                    bring_to_front(curr);
                    redraw_desktop();
                }

                // Clicou na barra de título: começa o arraste por contorno
                if (global_mouse_y <= (curr->y + TITLE_HEIGHT)) {
                    active_drag_window = curr;
                    drag_offset_x = global_mouse_x - curr->x;
                    drag_offset_y = global_mouse_y - curr->y;
                    drag_x = curr->x;
                    drag_y = curr->y;
                    draw_window_outline(drag_x, drag_y, curr->width, curr->height);
                }
                break;
            }
        }
    } else if (left && active_drag_window) {
        // 2. Arrastando
        Window *w = active_drag_window;
        int16_t new_x = clamp16(global_mouse_x - drag_offset_x, 0, SCREEN_WIDTH  - w->width);
        int16_t new_y = clamp16(global_mouse_y - drag_offset_y, 0, SCREEN_HEIGHT - w->height);

        if (new_x != drag_x || new_y != drag_y) {
            drag_x = new_x;
            drag_y = new_y;
            // Apagar o contorno antigo = redesenhar a tela (desenhar por cima
            // em preto não apaga nada). Se ficar lento, troque por XOR no sys_vga.
            redraw_desktop();
            draw_window_outline(drag_x, drag_y, w->width, w->height);
        }
    } else if (!left && active_drag_window) {
        // 3. Soltou o botão: move de verdade e redesenha
        active_drag_window->x = drag_x;
        active_drag_window->y = drag_y;
        active_drag_window = NULL;
        redraw_desktop();
    }
}

void process_mouse_and_windows(void) {
    uint8_t buttons;
    int8_t dx, dy;
    // Esvazia a fila inteira a cada chamada
    while (get_next_mouse_event(&buttons, &dx, &dy)) {
        handle_mouse_event(buttons, dx, dy);
    }
}

/* ------------------------------------------------------------------ */
/* Inicialização e ponto de entrada                                    */
/* ------------------------------------------------------------------ */
static void window_init(void) {
    window_stack_head = NULL;
    active_drag_window = NULL;
    drag_offset_x = 0;
    drag_offset_y = 0;
    drag_x = 0;
    drag_y = 0;
    global_mouse_x = 100;
    global_mouse_y = 100;
    mouse_head = 0;
    mouse_tail = 0;
}

int main_window(void) {
    window_init(); // não confia no .bss/.data

    Window *my_win = create_window(48, 48, 248, 160, "rigelOS Terminal");
    if (my_win == NULL) {
        return -1; // kmalloc falhou
    }

    redraw_desktop();

    // Loop principal do sistema
    // while (1) {
    //     process_mouse_and_windows();
    //     // outras tarefas do kernel...
    // }

    return 0;
}