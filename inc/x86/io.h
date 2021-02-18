#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

/********************************************************
 * 向端口写入一个字节
 *******************************************************/
static inline void out8(uint16_t port, uint8_t data) 
{
    // N 为立即数约束 端口号0~255， d为寄存器约束 dx
    asm volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}
/********************************************************
 * 向端口连续写入指定的字
 *******************************************************/
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) 
{
    asm volatile("cld; rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}
/********************************************************
 * 从端口读取一个字节数据
 *******************************************************/
static inline uint8_t in8(uint16_t port) 
{
    uint8_t data;
    
    asm volatile("inb  %w1, %b0" : "=a" (data) : "Nd"(port));

    return data;
}
/********************************************************
 * 从端口连续读入指定字数到内存
 *******************************************************/
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) 
{
    asm volatile("cld; rep innsw" :  "+D" (addr), "+c" (word_cnt) : "d"(port) : "memory");
}
#endif