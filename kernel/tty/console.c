// add by gu.fuguang 2021/2/17
#include "console.h"
#include "print.h"
#include "semaphore.h"
#include "thread.h"

static THREAD_LOCK console_lock;
/*************************************************************************
 *  终端锁初始化
*************************************************************************/
void console_init(void)
{
    lock_init(&console_lock);
}
/*************************************************************************
 *  终端输出字符串线程安全
*************************************************************************/
void console_str(char* str)
{
    lock_request(&console_lock);

    put_str(str);

    lock_release(&console_lock);
}
/*************************************************************************
 *  终端输出整数，线程安全
*************************************************************************/
void console_int(uint32_t num)
{
    lock_request(&console_lock);

    put_int(num);

    lock_release(&console_lock);
}