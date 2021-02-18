// add by gu.fuguang 2021/2/14
#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
/*************************************************************************
 *  位图初始化，把位图中所有数据清0
*************************************************************************/
void bitmap_init(BITMAP* bitmap) 
{
    memset(bitmap->bits, 0, bitmap->bitmap_bytes_len);
}
/*************************************************************************
 *  获取位图某一位的值
*************************************************************************/
int bitmap_scan_test(BITMAP* bitmap, uint32_t bit_idx) 
{
    uint32_t byte_idx = bit_idx /8;
    uint32_t bit_offset = bit_idx % 8;

    return (bitmap->bits[byte_idx] & (BITMAP_MASK << bit_offset));
}
/*************************************************************************
 *  获取连续cnt个空闲位图项
*************************************************************************/
int bitmap_scan(BITMAP* bitmap, uint32_t cnt) 
{
    uint32_t idx_byte = 0;

    // 查找第一个有空闲位的字节
    while((0xff == bitmap->bits[idx_byte]) && (idx_byte < bitmap->bitmap_bytes_len)) 
    {
        idx_byte++;
    }

    ASSERT(idx_byte <= bitmap->bitmap_bytes_len);
    if(idx_byte == bitmap->bitmap_bytes_len) 
    {
        return -1;          // 位图空间已用完
    }

    int idx_bit = 0;

    // 查找空闲位在字节中的偏移
    while((uint8_t)(BITMAP_MASK << idx_bit) & bitmap->bits[idx_byte]) 
    {
        idx_bit++;
    }

    int bit_idx_start = idx_byte * 8 + idx_bit;
    if(cnt == 1) 
    {
        return bit_idx_start;
    }

    // 从空闲位开始到位图结束的bits数
    uint32_t bit_left = bitmap->bitmap_bytes_len * 8 - bit_idx_start;
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 1;         // 连续空闲位计数

    bit_idx_start = -1;
    while(bit_left-- > 0) 
    {
        // 计算连续空闲位
        if(!(bitmap_scan_test(bitmap, next_bit))) 
        {
            count++;
        } else {
            count = 0;
        }

        // 找到需要的连续空闲位退出
        if(count == cnt) 
        {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        next_bit++;
    }

    return bit_idx_start;
}
/*************************************************************************
 *  位图置位
*************************************************************************/
void bitmap_set(BITMAP* bitmap, uint32_t bit_idx, int8_t value) 
{
    ASSERT((value == 0) || (value == 1));

    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_offset = bit_idx % 8;

    if(value) {
        bitmap->bits[byte_idx] |= (BITMAP_MASK << bit_offset);
    } else {
        bitmap->bits[byte_idx] &= ~(BITMAP_MASK << bit_offset);
    }
}