#ifndef __KERNEL_APP_THREAD_H
#define __KERNEL_APP_THREAD_H

#include "thread.h"

#define APP_STACK3_VADDR (0xc0000000 - 0x1000)
#define APP_VADDR_START  0x8048000

void process_ctx_init(void* file_name);
uint32_t* create_page_dir(void);
void create_app_vaddr_bitmap(THREAD_PCB* pcb);
void page_dir_activate(THREAD_PCB* pcb);
void process_activate(THREAD_PCB* pcb);
void process_execute(void* filename, char* name);
#endif