//  add by gu.fuguang 2021/2/17
#include "semaphore.h"
#include "interrupt.h"
#include "debug.h"
/*************************************************************************
 *  信号量初始化
*************************************************************************/
void sem_init(SEM* sem, uint8_t value)
{
    sem->value = value;
    list_init(&sem->wait_list);
}
/*************************************************************************
 *  锁初始化
*************************************************************************/
void lock_init(THREAD_LOCK* lock)
{
    lock->owner = NULL;
    lock->owner_repeat_no = 0;
    sem_init(&lock->sem, 1);
}
/*************************************************************************
 *  获取信号量
*************************************************************************/
void sem_down(SEM* sem) 
{
    INTR_STATUS old_status = intr_disable();

    while(sem->value == 0)
    {
        ASSERT(!list_find(&sem->wait_list, &get_cur_thread_pcb()->thread_status_list));

        list_add_tail(&sem->wait_list, &get_cur_thread_pcb()->thread_status_list);
        thread_block(THREAD_BLOCKED);
    }

    sem->value--;
    ASSERT(sem->value == 0);

    intr_set_status(old_status);
}
/*************************************************************************
 *  释放信号量
*************************************************************************/
void sem_up(SEM* sem) 
{
    INTR_STATUS old_status = intr_disable();

    ASSERT(sem->value == 0);
    if(!list_empty(&sem->wait_list))
    {
        THREAD_PCB* pcb = MEMBER2ENTRY(THREAD_PCB, thread_status_list, \
                                        list_delete(&sem->wait_list));
        thread_unblock(pcb);
    }

    sem->value++;
    ASSERT(sem->value == 1);

    intr_set_status(old_status);
}
/*************************************************************************
 *  获取锁
*************************************************************************/
void lock_request(THREAD_LOCK* lock)
{
    if(lock->owner != get_cur_thread_pcb())
    {
        sem_down(&lock->sem);
        lock->owner = get_cur_thread_pcb();
        ASSERT(lock->owner_repeat_no == 0);
        lock->owner_repeat_no = 1;
    } else {
        lock->owner_repeat_no++;
    }
}
/*************************************************************************
 *  获取锁
*************************************************************************/
void lock_release(THREAD_LOCK* lock)
{
    ASSERT(lock->owner == get_cur_thread_pcb());

    if(lock->owner_repeat_no > 1)
    {
        lock->owner_repeat_no--;

        return;
    } 

    ASSERT(lock->owner_repeat_no == 1);

    lock->owner = NULL;
    lock->owner_repeat_no = 0;
    sem_up(&lock->sem);
}
