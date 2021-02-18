// add by gu.fuguang 2021/2/16
#include "thread.h"
#include "string.h"
#include "global.h"
#include "interrupt.h"
#include "debug.h"
#include "print.h"
#include "app_thread.h"

#define THREAD_MOD_NAME "[THRAED] "
/*************************************************************************
 *  
*************************************************************************/
#define PG_SIZE  4096
THREAD_PCB* main_thread_pcb;
LIST thread_ready_list;
LIST thread_all_list;
static LIST_NODE* thread_status_node;

extern void thread_switch(THREAD_PCB* cur_pcb, THREAD_PCB* next_pcb);
/*************************************************************************
 *  获取当前运行任务的pcb
*************************************************************************/
THREAD_PCB* get_cur_thread_pcb(void)
{
    uint32_t esp;
    asm ("mov %%esp, %0": "=g"(esp));

    return (THREAD_PCB*)(esp & 0xfffff000);
}
/*************************************************************************
 *  线程挂起自己，调度其他任务到处理器运行
*************************************************************************/
void thread_block(THREAD_STATUS status)
{
    ASSERT(((status == THREAD_BLOCKED) || (status == THREAD_WAITING) \
          || (status == THREAD_HANGING)));
        
    INTR_STATUS old_status = intr_disable();

    THREAD_PCB* cur_pcb = get_cur_thread_pcb();

    cur_pcb->status = status;
    schedule();

    intr_set_status(old_status);
}
/*************************************************************************
 *  把线程从阻塞队列取出加到就绪队列
*************************************************************************/
void thread_unblock(THREAD_PCB *pcb)
{
    INTR_STATUS old_status = intr_disable();
    ASSERT(((pcb->status == THREAD_BLOCKED) || (pcb->status == THREAD_WAITING) \
          || (pcb->status == THREAD_HANGING)));

    if(pcb->status != THREAD_READY)
    {
        ASSERT(!list_find(&thread_ready_list, &pcb->thread_status_list));

        list_add_tail(&thread_ready_list, &pcb->thread_status_list);
        pcb->status = THREAD_READY;
    }

    intr_set_status(old_status);
}
/*************************************************************************
 *  线程运行模板
*************************************************************************/
void kernel_thread(thread_func* func, void* arg)
{
    intr_enable();
    func(arg);
}
/*************************************************************************
 *  创建线程栈结构，为线程运行做准备
*************************************************************************/
void thread_create(THREAD_PCB* pcb, thread_func func, void* arg)
{
    pcb->self_kstack -= sizeof(INTR_STACK);   //预留中断cpu压的栈
    pcb->self_kstack -= sizeof(THREAD_STACK); // 预留中断处理函数压得栈

    THREAD_STACK* k_thread_stack = (THREAD_STACK*)pcb->self_kstack;
    k_thread_stack->eip = kernel_thread;
    k_thread_stack->func = func;
    k_thread_stack->arg = arg;
    k_thread_stack->ebp = 0;
    k_thread_stack->ebx = 0;
    k_thread_stack->esi = 0;
    k_thread_stack->edi = 0;
}
/*************************************************************************
 *  初始化线程PCB的属性信息
*************************************************************************/
void thread_init(THREAD_PCB* pcb, char* name, int prio)
{
    memset(pcb, 0, sizeof(*pcb));
    strcpy(pcb->name, name);

    if(pcb == main_thread_pcb)
    {
        pcb->status = THREAD_RUNNING;
    } else {
        pcb->status = THREAD_READY;
    }

    pcb->self_kstack = (uint32_t*)((uint32_t)pcb + PG_SIZE);
    pcb->priority = prio;
    pcb->ticks = prio;
    pcb->elapsed_ticks = 0;
    pcb->pg_dir = NULL;
    pcb->stack_magic = 0x5A5A5A5A;
}
/*************************************************************************
 *  启动线程，加入调度对列
*************************************************************************/
THREAD_PCB* thread_start(char* name, int prio, thread_func func, void* arg)
{
    THREAD_PCB* thread_pcb = get_kernel_pages(1);

    thread_init(thread_pcb, name, prio);

    thread_create(thread_pcb, func, arg);

    ASSERT(!list_find(&thread_ready_list, &thread_pcb->thread_status_list));
    list_add_tail(&thread_ready_list, &thread_pcb->thread_status_list);

    ASSERT(!list_find(&thread_all_list, &thread_pcb->thread_all_list));
    list_add_tail(&thread_all_list, &thread_pcb->thread_all_list);

    //asm volatile("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop  %%esi; ret"::"g" (thread->self_kstack): "memory");

    return thread_pcb;
}
/*************************************************************************
 *  任务调度函数
*************************************************************************/
void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);

    THREAD_PCB* cur_pcb = get_cur_thread_pcb();
    if(cur_pcb->status == THREAD_RUNNING)
    {
        ASSERT(!list_find(&thread_ready_list, &cur_pcb->thread_status_list));
        list_add_tail(&thread_ready_list, &cur_pcb->thread_status_list);
        cur_pcb->ticks = cur_pcb->priority;
        cur_pcb->status = THREAD_READY;
    } else {

    }

    ASSERT(!list_empty(&thread_ready_list));
    thread_status_node = NULL;

    thread_status_node = list_delete(&thread_ready_list);

    THREAD_PCB* next_pcb = MEMBER2ENTRY(THREAD_PCB, thread_status_list, thread_status_node);

    next_pcb->status = THREAD_RUNNING;

    process_activate(next_pcb);
    // 汇编实现任务切换，主要是保存4个参数切换栈地址，任务下次还由此处返回
    thread_switch(cur_pcb, next_pcb);
}
/*************************************************************************
 *  初始化0号任务控制块，0号任务已经运行起来了，这里只是初始化PCB
*************************************************************************/
static void main_thread_create(void)
{
    // 主线程pcb
    main_thread_pcb = get_cur_thread_pcb();
    thread_init(main_thread_pcb, "idle", 255);

    ASSERT(!list_find(&thread_all_list, &main_thread_pcb->thread_all_list));
    list_add_tail(&thread_all_list, &main_thread_pcb->thread_all_list);
}
/*************************************************************************
 *  初始化0号任务控制块，0号任务已经运行起来了，这里只是初始化PCB
*************************************************************************/
void main_thread_init(void)
{
    put_str(THREAD_MOD_NAME" main thread init start \n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);

    main_thread_create();

    put_str(THREAD_MOD_NAME" main thread init done \n");
}