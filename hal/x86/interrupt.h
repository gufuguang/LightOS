#ifndef __LIB_KERNEL_INTERRUPT_H
#define __LIB_KERNEL_INTERRUPT_H
#include "stdint.h"
typedef void* intr_handler;

void idt_init(void);

typedef enum intr_status {
    INTR_OFF,
    INTR_ON
}INTR_STATUS;

INTR_STATUS intr_get_status(void);
INTR_STATUS intr_set_status(INTR_STATUS status);
INTR_STATUS intr_enable(void);
INTR_STATUS intr_disable(void);
void register_handler(uint8_t vector_no, intr_handler intr_func);
#endif