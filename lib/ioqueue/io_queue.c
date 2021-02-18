// add by gu.fuguang 2021/2/17
#include "io_queue.h"
#include "interrupt.h"
#include "debug.h"

/*************************************************************************
 *  io缓冲区初始化
*************************************************************************/
void io_queue_init(IO_QUEUE* queue)
{
    lock_init(&queue->lock);
    queue->producer = NULL;
    queue->consumer = NULL;
    queue->head = 0;
    queue->tail = 0;
}
/*************************************************************************
 *  io缓冲区是否已满
*************************************************************************/
static int32_t next_pos(int32_t pos)
{
    return (pos + 1) % BUFF_SIZE;
}
bool is_queue_fill(IO_QUEUE* queue)
{
    ASSERT(intr_get_status() == INTR_OFF);

    return next_pos(queue->head) == queue->tail;
}
/*************************************************************************
 *  io缓冲区是否是空
*************************************************************************/
bool is_queue_empty(IO_QUEUE* queue)
{
    ASSERT(intr_get_status() == INTR_OFF);

    return queue->head== queue->tail;
}
/*************************************************************************
 *  排队访问缓冲区
*************************************************************************/
static void  queue_wait(THREAD_PCB** waiter)
{
    ASSERT(*waiter == NULL && waiter != NULL);

    *waiter = get_cur_thread_pcb();

    thread_block(THREAD_BLOCKED);
}
/*************************************************************************
 *  唤醒阻塞的排队访问缓冲区任务
*************************************************************************/
static void queue_wakeup(THREAD_PCB** waiter)
{
    ASSERT(*waiter != NULL);

    thread_unblock(*waiter);

    *waiter = NULL;
}
/*************************************************************************
 *  获取一个字节数据
*************************************************************************/
char io_queue_get_char(IO_QUEUE* queue)
{
    ASSERT(intr_get_status() == INTR_OFF);

    while(is_queue_empty(queue))
    {
        lock_request(&queue->lock);
        queue_wait(&queue->consumer);
        lock_release(&queue->lock);
    }

    char byte = queue->buff[queue->tail];
    queue->tail = next_pos(queue->tail);

    if(queue->producer != NULL)         // 唤醒生产者
    {
        queue_wakeup(&queue->producer);
    }

    return byte;
}
/*************************************************************************
 *  写入一个字节数据
*************************************************************************/
char io_queue_put_char(IO_QUEUE* queue, char byte)
{
    ASSERT(intr_get_status() == INTR_OFF);

    while(is_queue_fill(queue))
    {
        lock_request(&queue->lock);
        queue_wait(&queue->producer);
        lock_release(&queue->lock);
    }

    queue->buff[queue->head] = byte;
    queue->head = next_pos(queue->head);

    if(queue->consumer != NULL)     // 唤醒消费者
    {
        queue_wakeup(&queue->consumer);
    }

    return byte;
}