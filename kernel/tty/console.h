#ifndef __KERNEL_CONSOLE_H
#define __KERNEL_CONSOLE_H

#include "stdint.h"

void console_init(void);

void console_str(char* str);

void console_int(uint32_t num);

#endif