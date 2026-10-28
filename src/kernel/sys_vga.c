#include <stdint.h>
#include <rigel/rigel.h>

#define PICO_VGA_BASE 0x00FF8000
#define WRITE_SCREEN   (*((volatile unsigned char *)(PICO_VGA_BASE + 0x01)))
#define REG_02         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x03)))
#define REG_03         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x05)))
#define REG_04         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x07)))
#define CONFIG_REG     (*((volatile unsigned char *)(PICO_VGA_BASE + 0x09)))
#define REG_06         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x0B)))
#define REG_07         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x0D)))
#define REG_08         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x0F)))
#define REG_09         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x11)))
#define REG_0A         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x13)))
#define REG_0B         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x15)))
#define REG_0C         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x17)))
#define REG_0D         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x19)))
#define REG_0E         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x1B)))
#define REG_0F         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x1D)))
#define REG_10         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x1F)))
#define REG_11         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x21)))
#define REG_12         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x23)))
#define REG_13         (*((volatile unsigned char *)(PICO_VGA_BASE + 0x25)))
#define SET_MODE       (*((volatile unsigned char *)(PICO_VGA_BASE + 0x27)))
#define SET_TXT_COLOR  (*((volatile unsigned char *)(PICO_VGA_BASE + 0x29)))
#define CHANGE_CUR_POS (*((volatile unsigned char *)(PICO_VGA_BASE + 0x2B)))
#define REG_X_HIGH     (*((volatile unsigned char *)(PICO_VGA_BASE + 0x2D)))
#define REG_X_LOW      (*((volatile unsigned char *)(PICO_VGA_BASE + 0x2F)))
#define REG_Y_HIGH     (*((volatile unsigned char *)(PICO_VGA_BASE + 0x31)))
#define REG_Y_LOW      (*((volatile unsigned char *)(PICO_VGA_BASE + 0x33)))
#define CHANGE_BUFFER  (*((volatile unsigned char *)(PICO_VGA_BASE + 0x35)))
#define SELECT_SCREEN  (*((volatile unsigned char *)(PICO_VGA_BASE + 0x37)))
#define SET_HORIZONTAL (*((volatile unsigned char *)(PICO_VGA_BASE + 0x39)))
#define SET_VERTICAL   (*((volatile unsigned char *)(PICO_VGA_BASE + 0x3B)))
#define RUN_CMD        (*((volatile unsigned char *)(PICO_VGA_BASE + 0x3D)))
#define CORINGA        (*((volatile unsigned char *)(PICO_VGA_BASE + 0x3F)))


#define CMD_SYSTEM_ENABLE   0xA5
#define CMD_CLEAR_SCREEN    0xA4
#define CMD_SET_CUR_POS     0xA3
#define CMD_SET_TXT_COLOR   0xA2
#define CMD_GO_HOME         0xA1
//Graphos commands
#define CMD_DRAW_PIXEL                  0xA0
#define CMD_DRAW_LINE                   0xBF
#define CMD_DRAW_VLINE                  0xBE
#define CMD_DRAW_HLINE                  0xBD
#define CMD_DRAW_CIRCLE                 0xBC
#define CMD_DRAW_CIRCLEHELPER           0xBB
#define CMD_DRAW_FILLCIRCLE             0xBA
#define CMD_DRAW_FILLCIRCLEHELPER       0xB9
#define CMD_DRAW_ROUNDRECT              0xB8
#define CMD_DRAW_FILLROUNDRECT          0xB7
#define CMD_DRAW_FILLRECT               0xB6
#define CMD_DRAW_FILLTRIANGLE           0xB5
#define CMD_DRAW_MULTILINE              0xB4
#define CMD_SET_CUR_ON                  0xB3
#define CMD_SET_CUR_OFF                 0xB2

static inline void run_cmd(unsigned char cmd){
    RUN_CMD = cmd;
}
void sys_setcurvisibility(const char cur){
    if(cur){
        run_cmd(CMD_SET_CUR_ON); 
    }else{
        run_cmd(CMD_SET_CUR_OFF); 
    }
}
long sys_vga_putchar(long c){
}
long sys_gotoxy(long x, long y){
    REG_Y_LOW  = y; //(unsigned char)(row & 0x00FF);
    REG_Y_HIGH = 0;   //(unsigned char)(row > 8);
    REG_X_LOW  = x; //(unsigned char)(col & 0x00FF);
    REG_X_HIGH = 0;   //(unsigned char)(col > 8);
    run_cmd(CMD_SET_CUR_POS);    
}
long sys_gohome(void){
    run_cmd(CMD_GO_HOME);    
}
void sys_settextcolor(long txtcolor,long bgcolor){
    unsigned int color;
    color = (txtcolor << 4) &0xF0;
    color |= bgcolor;
    SET_TXT_COLOR = color;
}
long sys_clrscr(void){
    run_cmd(CMD_CLEAR_SCREEN);    
}

void sys_turncursor(long status){
    
}