#ifndef __KERNEL_IO_QUEUE_H
#define __KERNEL_IO_QUEUE_H

#include "stdint.h"
#include "thread.h"
#include "semaphore.h"

#define BUFF_SIZE 64

typedef struct io_queue {
    THREAD_LOCK lock;
    THREAD_PCB* producer;
    THREAD_PCB* consumer;
    char buff[BUFF_SIZE];
    int32_t head;
    int32_t tail;
}IO_QUEUE;

void io_queue_init(IO_QUEUE* queue);
bool is_queue_fill(IO_QUEUE* queue);
bool is_queue_empty(IO_QUEUE* queue);
char io_queue_get_char(IO_QUEUE* queue);
char io_queue_put_char(IO_QUEUE* queue, char byte);
#endif