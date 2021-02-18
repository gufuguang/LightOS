// add by gu.fuguang 2021/2/17
#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "io_queue.h"
#include "stdint.h"

#define KBD_BUF_PORT 0x60    // 键盘buf寄存器端口号
#define IDT_MOD_NAME "[KBD] "

/*************************************************************************
 * 转义字符
*************************************************************************/
#define ESC        '\033'
#define BACKSPACE  '\b'
#define TAB        '\t'
#define ENTER      '\r'
#define DELETE     '\177'

#define CHAR_INVISIBLE 0
#define CTRL_L     CHAR_INVISIBLE
#define CTRL_R     CHAR_INVISIBLE
#define SHIFT_L    CHAR_INVISIBLE
#define SHIFT_R    CHAR_INVISIBLE
#define ALT_L      CHAR_INVISIBLE
#define ALT_R      CHAR_INVISIBLE
#define CAPS_LOCK  CHAR_INVISIBLE
// 按键通码和断码定义
#define SHIFT_L_MAKE   0x2a
#define SHIFT_R_MAKE   0x36
#define ALT_L_MAKE     0x38
#define ALT_R_MAKE     0xe038
#define ALT_R_BREAK    0xe0b8
#define CTRL_L_MAKE    0x1d
#define CTRL_R_MAKE    0xe01d
#define CTRL_R_BREAK   0xe09d
#define CAPS_LOCK_MAKE 0x3a

// 键盘接收缓冲区
IO_QUEUE kbd_buff; 

// 记录控制按键是否被按下
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_make_code;
static char key_map[][2] = {
// 未按shift 按下shift
    {0,                   0},
    {ESC,               ESC},
    {'1',               '!'},
    {'2',               '@'},
    {'2',               '#'},
    {'4',               '$'},
    {'5',               '%'},
    {'6',               '^'},
    {'7',               '&'},
    {'8',               '*'},
    {'9',               '('},
    {'0',               ')'},
    {'-',               '_'},
    {'=',               '+'},
    {BACKSPACE,   BACKSPACE},
    {TAB,               TAB},
    {'q',               'Q'},
    {'w',               'W'},
    {'e',               'E'},
    {'r',               'R'},
    {'t',               'T'},
    {'y',               'Y'},
    {'u',               'U'},
    {'i',               'I'},
    {'o',               'O'},
    {'p',               'P'},
    {'[',               '{'},
    {']',               '}'},
    {ENTER,           ENTER},
    {CTRL_L,         CTRL_L},
    {'a',               'A'},
    {'s',               'S'},
    {'d',               'D'},
    {'f',               'F'},
    {'g',               'G'},
    {'h',               'H'},
    {'j',               'J'},
    {'k',               'K'},
    {'l',               'L'},
    {';',               ':'},
    {'\'',              '"'},
    {'`',               '~'},
    {SHIFT_L,        SHIFT_L},
    {'\\',              '|'},
    {'z',               'Z'},
    {'x',               'X'},
    {'c',               'C'},
    {'v',               'V'},
    {'b',               'B'},
    {'n',               'N'},
    {'m',               'M'},
    {',',               '<'},
    {'.',               '>'},
    {'/',               '?'},
    {SHIFT_R,       SHIFT_R},
    {'*',               '*'},
    {ALT_L,          ALT_L},
    {' ',              ' '},
    {CAPS_LOCK,  CAPS_LOCK},
};
/*************************************************************************
 * 键盘中断处理函数
*************************************************************************/
static void intr_keyboard_handler(void)
{
    //bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;

    bool    break_code;
    uint8_t scan_code = in8(KBD_BUF_PORT);

    if(scan_code == 0xe0)   // 按键按下产生多个扫描码退出中断，等待下一个中断进来
    {
        ext_make_code = true;
        return;
    }

    if(ext_make_code == true)
    {
        scan_code = 0xe000 | scan_code;
        ext_make_code = false;    // 读完一次扫描码 
    }

    break_code = ((scan_code & 0x80) != 0);
    if(break_code)
    {
        uint16_t make_code = (scan_code &= 0xff7f);
        // 处理控制键抬起
        if(make_code == CTRL_L_MAKE || make_code == CTRL_R_MAKE)
        {
            ctrl_status = false;
        } 
        else if(make_code == SHIFT_L_MAKE || make_code == SHIFT_R_MAKE)
        {
            shift_status = false;
        } 
        else if(make_code == ALT_L_MAKE || make_code == ALT_R_MAKE)
        {
            alt_status = false;
        }
        else
        {

        }

        return;
    }
    else if((scan_code > (uint8_t)0 && scan_code < (uint8_t)0x3b) \
            || (scan_code == (uint8_t)ALT_R_MAKE) \
            || (scan_code == (uint8_t)CTRL_R_MAKE))
    {
        bool shift = false;     // shift 按键是否按下

        if((scan_code < (uint8_t)0xe) || (scan_code == (uint8_t)0x29) \
           || (scan_code == (uint8_t)0x1a) || (scan_code == (uint8_t)0x1b) \
           || (scan_code == (uint8_t)0x2b) || (scan_code == (uint8_t)0x27) \
           || (scan_code == (uint8_t)0x28) || (scan_code == (uint8_t)0x33) \
           || (scan_code == (uint8_t)0x34) || (scan_code == (uint8_t)0x35))
        { // 一个键上有两个字母
            if(shift_down_last)
            {
                shift = true;
            }
        }
        else
        {
            if(shift_down_last && caps_lock_last) 
            {
                shift = false;
            }
            else if(shift_down_last || caps_lock_last)
            {
                shift = true;
            }
            else
            {
                shift = false;
            }
        }

        uint8_t index = (scan_code &= 0x00ff);
        char key_char = key_map[index][shift];

        if(key_char)
        {
            
            if(!is_queue_fill(&kbd_buff)) // 缓冲未满就加入字符，中断不能阻塞
            {
                io_queue_put_char(&kbd_buff, key_char);
            }
            return;
        }

        if(scan_code == (uint8_t)CTRL_L_MAKE || scan_code == (uint8_t)CTRL_R_MAKE)
        {
            ctrl_status = true;
        } 
        else if(scan_code == (uint8_t)SHIFT_L_MAKE || scan_code == (uint8_t)SHIFT_R_MAKE)
        {
            shift_status = true;
        } 
        else if(scan_code == (uint8_t)ALT_L_MAKE || scan_code == (uint8_t)ALT_R_MAKE)
        {
            alt_status = true;
        }
        else if(scan_code == (uint8_t)CAPS_LOCK_MAKE)
        {
            caps_lock_status = !caps_lock_status;
        }
    }
    else
    {
        put_str("unknown key\n");
    }


    return;
}
/*************************************************************************
 * 键盘中断处理函数
*************************************************************************/
void keyboard_init(void)
{
    put_str(IDT_MOD_NAME"keyboard init start\n");

    io_queue_init(&kbd_buff);
    register_handler(0x21, intr_keyboard_handler);

    put_str(IDT_MOD_NAME"keyboard init done\n");
}
