// add by gu.fuguang 2021/2/14
#include "timer.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "thread.h"
#include "debug.h"

#define TIMER_MOD_NAME "[TIMER] "
/*************************************************************************
 *  8253  定时器相关定义
*************************************************************************/
#define IRQ0_FREQUENCY   100          // Tick 10ms
#define INPUT_FREQUENCY  1193180
#define COUNTER0_VALUE   (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTER0_PORT    0x40
#define PIT_CONTROL_PORT 0x43
#define COUNTER0_NO      0
#define COUNTER_MODE     2
#define READ_WRITE_LATCH 3

uint32_t ticks;
/*************************************************************************
 *  设置定时器中断触发频率
*************************************************************************/
static void freq_set(uint8_t counter_port,       // 端口号
                     uint8_t counter_no,         // 计数器编号
                     uint8_t rwl,                // 读写锁存属性
                     uint8_t counter_mode,       // 定时器模式
                     uint16_t counter_value)     // 计数值
{
    out8(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    out8(counter_port, (uint8_t)counter_value);
    out8(counter_port, (uint8_t)counter_value >> 8);
}
/*************************************************************************
 *  定时器中断回调函数
*************************************************************************/
static void intr_timer_handler(void)
{
    THREAD_PCB* cur_pcb = get_cur_thread_pcb();

    ASSERT(cur_pcb->stack_magic == 0x5A5A5A5A);
    cur_pcb->elapsed_ticks++;
    ticks++;
    if(cur_pcb->ticks == 0)
    {
        schedule();
    } else {
        cur_pcb->ticks--;
    }
}
/*************************************************************************
 *  tick 定时器初始化
*************************************************************************/
void tick_timer_init(void) 
{
    put_str(TIMER_MOD_NAME"timer init start\n");

    freq_set(COUNTER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    register_handler(0x20, intr_timer_handler);

    put_str(TIMER_MOD_NAME"timer init done\n");

    return;
}
