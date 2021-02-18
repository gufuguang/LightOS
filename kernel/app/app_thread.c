// add by gu.fuguang 2021/2/18
#include "app_thread.h"
#include "global.h"
#include "interrupt.h"
#include "debug.h"
#include "tss.h"
#include "string.h"
#include "console.h"

#define PROCESS_MOD_NAME "[PROCESS] "
extern void intr_exi(void);
extern LIST thread_ready_list;
extern LIST thread_all_list;
/*************************************************************************
 *  填充返回3特权级上下文
*************************************************************************/
void process_ctx_init(void* file_name)
{
    void* function = file_name;
    THREAD_PCB* pcb = get_cur_thread_pcb();

    pcb->self_kstack += sizeof(THREAD_STACK);
    INTR_STACK* proc_stack = (INTR_STACK*)pcb->self_kstack;
    proc_stack->edi = 0;
    proc_stack->esi = 0;
    proc_stack->ebp = 0;
    proc_stack->esp_dummy = 0;
    proc_stack->ebx = 0;
    proc_stack->edx = 0;
    proc_stack->ecx = 0;
    proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->fs = SELECTOR_APP_DATA;
    proc_stack->es = SELECTOR_APP_DATA;
    proc_stack->ds = SELECTOR_APP_DATA;         // 以上是进入中断后，中断中压入的栈

    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_APP_CODE;
    proc_stack->eflags = (EFLAGS_IOPL0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void*)((uint32_t)get_one_page(PF_APP,APP_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_APP_DATA;        // 以上是由低特权级进入高特权级cpu压入的栈

    asm volatile("movl %0, %%esp; jmp intrExit": :"g"(proc_stack): "memory");
}
/*************************************************************************
 *  激活页表
*************************************************************************/
void page_dir_activate(THREAD_PCB* pcb)
{
    uint32_t pg_dir_phy_addr = 0x1000000;  // loader 中内核页表在16M

    if(pcb->pg_dir != NULL) // 用户进程
    {
        pg_dir_phy_addr = addr_v2p((uint32_t)pcb->pg_dir);
    }

    asm volatile("movl %0, %%cr3":: "r"(pg_dir_phy_addr):"memory");
}
/*************************************************************************
 *  更新页表和 0特权级栈
*************************************************************************/
void process_activate(THREAD_PCB* pcb)
{
    ASSERT(pcb != NULL);

    page_dir_activate(pcb);

    if(pcb->pg_dir)
    {
        update_tss_esp0(pcb);
    }
}
/*************************************************************************
 *  创建页目录表
*************************************************************************/
uint32_t* create_page_dir(void)
{
    uint32_t* pg_dir_vaddr = get_kernel_pages(1);   // 申请一页做页目录表

    if(pg_dir_vaddr == NULL)
    {
        console_str(PROCESS_MOD_NAME"create_page_dir get_kernel_pages error\n");

        return NULL;
    }

    // 拷贝内核页目录表
    memcpy((uint32_t*)((uint32_t)pg_dir_vaddr + 0x300*4), \
            (uint32_t*)(0xfffff000 + 0x300*4), 1024);

    uint32_t pg_dir_phy_addr = addr_v2p((uint32_t)pg_dir_vaddr); 
    pg_dir_vaddr[1023] = pg_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;

    return pg_dir_vaddr;
}
/*************************************************************************
 *  创建APP程序虚拟地址管理位图
*************************************************************************/
void create_app_vaddr_bitmap(THREAD_PCB* pcb)
{
    pcb->app_mm_pool.vaddr_start = APP_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - APP_VADDR_START) / PG_SIZE / 8, \
                                            PG_SIZE);
    pcb->app_mm_pool.vaddr_bitmap.bits= get_kernel_pages(bitmap_pg_cnt);
    pcb->app_mm_pool.vaddr_bitmap.bitmap_bytes_len = (0xc0000000 - APP_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&pcb->app_mm_pool.vaddr_bitmap);
}
/*************************************************************************
 *  创建用户进程
*************************************************************************/
void process_execute(void* filename, char* name)
{
    THREAD_PCB* pcb = get_kernel_pages(1);

    thread_init(pcb, name, 200);

    create_app_vaddr_bitmap(pcb);

    thread_create(pcb, process_ctx_init, filename);
    pcb->pg_dir = create_page_dir();

    INTR_STATUS old_status = intr_disable();
    ASSERT(!list_find(&thread_ready_list, &pcb->thread_status_list));
    list_add_tail(&thread_ready_list, &pcb->thread_status_list);

    ASSERT(!list_find(&thread_all_list, &pcb->thread_all_list));
    list_add_tail(&thread_all_list, &pcb->thread_all_list);

    intr_set_status(old_status);
}