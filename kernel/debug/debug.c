// add by gu.fuguang 2021/2//14
#include "debug.h"
#include "print.h"
#include "interrupt.h"
/*************************************************************************
 * 断言出错，挂起CPU
*************************************************************************/
void panic_spin(char* file_name, int line, const char* func, const char* condition) 
{
    intr_disable();         // 关中断CPU，CPU不会被打断

    put_str("\n***************** ERROR *****************\n");
    put_str("filename: ");
    put_str(file_name);
    put_str("\n");

    put_str("line: ");
    put_int(line);
    put_str("\n");

    put_str("function: ");
    put_str((char *)func);
    put_str("\n");

    put_str("condition: ");
    put_str((char *)condition);
    put_str("\n");

    while(1);
}