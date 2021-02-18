#ifndef __KERNEL_THREAD_H
#define __KERNEL_THREAD_H

#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "memory.h"

typedef void thread_func(void *);

typedef enum thread_status {
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_BLOCKED,
    THREAD_WAITING,
    THREAD_HANGING,
    THREAD_DIED,
}THREAD_STATUS;

typedef struct intr_stack {
    uint32_t vector_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;         // 以上是进入中断后，中断中压入的栈

    uint32_t err_code;
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void*    esp;
    uint32_t ss;        // 以上是由低特权级进入高特权级cpu压入的栈
}INTR_STACK;

typedef struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    void (*eip)(thread_func* func, void* arg);

    void (*unused_retaddr);

    thread_func *func;
    void* arg;
}THREAD_STACK;        // 以上是进行任务切换时，保存切换时候的状态，后面切回本任务时使用    

typedef struct thread_pcb {
    uint32_t* self_kstack;
    THREAD_STATUS status;
    char name[16];
    uint8_t priority;
    uint8_t ticks;
    uint32_t elapsed_ticks;

    LIST_NODE thread_status_list;
    LIST_NODE thread_all_list;
    uint32_t* pg_dir;             // 记录进程页目录表地址
    VIRTUAL_MM_POOL app_mm_pool;  // 记录管理应用空间虚拟地址
    uint32_t  stack_magic;
}THREAD_PCB;

void kernel_thread(thread_func* func, void* arg);
void thread_init(THREAD_PCB* pcb, char* name, int prio);
THREAD_PCB* thread_start(char* name, int prio, thread_func func, void* arg);
void main_thread_init(void);

void thread_create(THREAD_PCB* pcb, thread_func func, void* arg);

void schedule(void);
THREAD_PCB* get_cur_thread_pcb(void);

void thread_block(THREAD_STATUS status);
void thread_unblock(THREAD_PCB *pcb);
#endif