// add by gu.fuguang 2021/2/10
#include "hal_init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "keyboard.h"
#include "console.h"
#include "tss.h"

#define HAL_MOD_NAME "[HAL] "
/*************************************************************************
 *  底层相关硬件初始化
*************************************************************************/
void hal_init(uint32_t mm_size) 
{
    put_str(HAL_MOD_NAME" board init start ...\n");

    idt_init();   // 初始化中断

    tss_init();

    tick_timer_init();

    mm_init(mm_size);

    keyboard_init();

    console_init(); //线程安全打印函数
}

