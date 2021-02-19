#include "print.h"
#include "hal_init.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "app_thread.h"
#include "console.h"

void thread_a(void*);
void app_thread_a(void);
static int app_var = 0;
int main(int mem_size, char** argv) 
{
    asm volatile("cli" : : :"memory");      // 关中断
    put_int((int)argv);
    put_char('\n');
    put_str("\nwelcome to C world, computer mem size 0x");
    put_int(mem_size);
    put_char('\n');

    hal_init(mem_size);

    main_thread_init();

    thread_start("thread_a", 32, thread_a, "app thread ");
    process_execute(app_thread_a, "app_thread_a");

    console_str("---------------------------------------------------------\n");
    console_str("                     hello world                         \n");
    console_str("---------------------------------------------------------\n");
    intr_enable();

    int i = 3;
    while(i--)
    {
        put_str("idle thread\n");
    }
    while(1);

    return 0;
}

void thread_a(void* arg)
{
    char* para = arg;
    int i = 3;
    while(i--)
    {
        console_str(para);
        console_int(app_var);
        console_str("\n");

    }

    while(1);
}

void app_thread_a(void)
{
    while(1)
    {
        app_var++;
    }
}