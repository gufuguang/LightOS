#ifndef __KERNEL_SEM_H
#define __KERNEL_SEM_H

#include "list.h"
#include "stdint.h"
#include "thread.h"

typedef struct semaphore {
    uint8_t value;
    LIST    wait_list;
}SEM;

typedef struct thread_lock {
    THREAD_PCB* owner;
    SEM         sem;
    uint32_t    owner_repeat_no;
} THREAD_LOCK;

void sem_init(SEM* sem, uint8_t value);
void sem_up(SEM* sem);
void sem_down(SEM* sem);

void lock_init(THREAD_LOCK* lock);
void lock_request(THREAD_LOCK* lock);
void lock_release(THREAD_LOCK* lock);

#endif
