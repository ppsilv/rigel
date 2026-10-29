#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

// Definições de cores padrão
#define COLOR_BLACK         0x00
#define COLOR_WHITE         0xFF
#define COLOR_LIGHT_GRAY    0xC0
#define COLOR_GRAY          0x80
#define COLOR_DARK_BLUE     0x01
#define COLOR_DESKTOP_BG    0x7F

// Protótipos das primitivas gráficas do sistema
extern void sys_fillrect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
extern void sys_drawhline(int16_t x, int16_t y, int16_t w, uint8_t color);
extern void sys_drawvline(int16_t x, int16_t y, int16_t h, uint8_t color);
extern void sys_gotoxy(int16_t x, int16_t y);

typedef struct Window {
    int16_t x, y;
    int16_t width, height;
    char title[32];
    uint8_t active;
    struct Window *next; // Próxima janela na pilha (abaixo desta)
} Window;

static Window *window_stack_head = NULL;

static int16_t old_x = 0, old_y = 0;
static Window *active_drag_window = NULL;
static int16_t drag_offset_x = 0;
static int16_t drag_offset_y = 0;
static int16_t global_mouse_x = 100;
static int16_t global_mouse_y = 100;

// MMIO unificado
#define MOUSE_STATUS    (*(volatile uint8_t *)0xFF9101)
#define MOUSE_DATA      (*(volatile uint8_t *)0xFF9103)
#define CURSOR_X        (*(volatile uint8_t *)0xFF9105)
#define CURSOR_Y        (*(volatile uint8_t *)0xFF9107)

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

void enqueue_mouse_event(uint8_t *packet) {
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

void mouse_isr(void) {
    uint8_t packet[5];
    for (int i = 0; i < 5; i++) {
        packet[i] = MOUSE_DATA;
    }
    enqueue_mouse_event(packet);
}

void update_mouse_cursor(int16_t x, int16_t y) {
    CURSOR_X = (uint8_t)x;
    CURSOR_Y = (uint8_t)y;
}

void draw_window_outline(int16_t x, int16_t y, int16_t width, int16_t height) {
    sys_drawhline(x, y, width, COLOR_BLACK);
    sys_drawhline(x, y + height, width, COLOR_BLACK);
    sys_drawvline(x, y, height, COLOR_BLACK);
    sys_drawvline(x + width, y, height, COLOR_BLACK);
}

void draw_window(Window *win) {
    sys_fillrect(win->x, win->y + 16, win->width, win->height - 16, COLOR_LIGHT_GRAY);

    uint8_t title_color = win->active ? COLOR_DARK_BLUE : COLOR_GRAY;
    sys_fillrect(win->x, win->y, win->width, 16, title_color);

    sys_gotoxy(win->x + 4, win->y + 4);

    sys_drawhline(win->x, win->y, win->width, COLOR_BLACK);
    sys_drawhline(win->x, win->y + win->height, win->width, COLOR_BLACK);
    sys_drawvline(win->x, win->y, win->height, COLOR_BLACK);
    sys_drawvline(win->x + win->width, win->y, win->height, COLOR_BLACK);
}

void draw_stack_bottom_to_top(Window *win) {
    if (win == NULL) {
        return;
    }
    draw_stack_bottom_to_top(win->next);
    draw_window(win);
}

void bring_to_front(Window *win) {
    if (window_stack_head == win) return;

    Window *curr = window_stack_head;
    while (curr && curr->next != win) {
        curr = curr->next;
    }
    
    if (curr && curr->next == win) {
        curr->next = win->next;
    }

    win->next = window_stack_head;
    window_stack_head = win;

    Window *temp = window_stack_head;
    while (temp) {
        temp->active = (temp == window_stack_head);
        draw_window(temp);
        temp = temp->next;
    }
}

void redraw_desktop(void) {
    sys_fillrect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_DESKTOP_BG);
    draw_stack_bottom_to_top(window_stack_head);
}

void process_mouse_and_windows(void) {
    uint8_t buttons;
    int8_t dx, dy;
    if (!get_next_mouse_event(&buttons, &dx, &dy)) {
        return;
    }

    global_mouse_x += dx;
    global_mouse_y += dy;
    update_mouse_cursor(global_mouse_x, global_mouse_y);

    int left_button_pressed = (buttons & 0x01);

    if (left_button_pressed && !active_drag_window) {
        Window *curr = window_stack_head;
        while (curr) {
            if (global_mouse_x >= curr->x && global_mouse_x <= (curr->x + curr->width) &&
                global_mouse_y >= curr->y && global_mouse_y <= (curr->y + curr->height)) {
                
                bring_to_front(curr);

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

    if (!left_button_pressed && active_drag_window) {
        draw_window_outline(old_x, old_y, active_drag_window->width, active_drag_window->height);
        active_drag_window->x = old_x;
        active_drag_window->y = old_y;
        redraw_desktop();
        active_drag_window = NULL;
    }
}