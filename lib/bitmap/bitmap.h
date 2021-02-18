#ifndef __LIB_BITMAP_H
#define __LIB_BITMAP_H
#include "stdint.h"

#define BITMAP_MASK  1

typedef struct bitmap {
    uint32_t bitmap_bytes_len;  // 位图占用的字节数
    uint8_t* bits;              // 位图的起始地址
}BITMAP;

void bitmap_init(BITMAP* bitmap);
int  bitmap_scan_test(BITMAP* bitmap, uint32_t bit_idx);
int  bitmap_scan(BITMAP* bitmap, uint32_t cnt);
void bitmap_set(BITMAP* bitmap, uint32_t bit_idx, int8_t value);
#endif