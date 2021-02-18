// add by gu.fuguang 2021/2/14
#include "string.h"
#include "stdint.h"
#include "debug.h"
/*************************************************************************
 *  设置一片内存为设定数值
*************************************************************************/
void memset(void* dest, uint8_t value, uint32_t size) 
{
    ASSERT(dest != NULL);

    uint8_t* tmp_dest = (uint8_t*)dest;

    while(size-- > 0) 
    {
        *tmp_dest++ = value;
    }

    return;
}
/*************************************************************************
 *  拷贝一片内存
*************************************************************************/
void memcpy(void* dest, const void* src, uint32_t size) 
{
    ASSERT(dest != NULL  && src != NULL);

    uint8_t* tmp_dest = (uint8_t*)dest;
    const uint8_t* tmp_src  = (uint8_t *) src;

    while(size-- > 0) 
    {
        *tmp_dest++ = *tmp_src++;
    }

    return;
}
/*************************************************************************
 *  比较一片内存
*************************************************************************/
int memcmp(const void* mem_a, const void* mem_b, uint32_t size) 
{
    ASSERT(mem_a != NULL &&  mem_b!= NULL);

    const char* tmp_a = mem_a;
    const char* tem_b = mem_b;

    while(size-- > 0) {
        if(*tmp_a != *tem_b) {
            return *tmp_a > *tem_b ? 1 : -1;
        }

        tmp_a++;
        tem_b++;
    }

    return 0;
}
/*************************************************************************
 *  字符串拷贝
*************************************************************************/
char* strcpy(char* dest, const char* src) 
{
    ASSERT(dest != NULL && src != NULL);

    char* tmp_dest = dest;
    
    while((*dest++ = *src++));

    return tmp_dest;
}
/*************************************************************************
 *  字符串长度
*************************************************************************/
uint32_t strlen(const char* src) 
{
    ASSERT(src != NULL);

    const char* tmp_src = src;

    while(*tmp_src++);

    return (tmp_src - src - 1);
}
/*************************************************************************
 *  字符串比较
*************************************************************************/
int8_t strcmp(const char* str_a, const char* str_b) 
{
    ASSERT((str_a != NULL) && (str_b != NULL));

    const char* tmp_a = str_a;
    const char* tem_b = str_b;

    while( tmp_a != 0 && *tmp_a == *tem_b) 
    {
        tmp_a++;
        tem_b++;
    }

    return (*tmp_a < *tem_b ? -1 : *tmp_a > *tem_b);
}
/*************************************************************************
 *  从前往后查找字符在字符串中首次出现的位置
*************************************************************************/
char* strchr(const char* src, const uint8_t ch) 
{
    ASSERT(src != NULL);

    const char* tmp_src = src;

    while( *tmp_src != 0) 
    {
        if(*tmp_src == ch) 
        {
            return (char *)tmp_src;
        }
        tmp_src++;
    }

    return NULL;
}
/*************************************************************************
 *  从后往前查找字符在字符串中首次出现的位置
*************************************************************************/
char* strrchr(const char* src, const uint8_t ch) 
{
    ASSERT(src != NULL);

    const char* tmp_src   = src;
    const char* last_char = NULL;

    while(*tmp_src != 0) 
    {
        if(*tmp_src == ch) 
        {
            last_char = tmp_src;
        }
        tmp_src++;
    }

    return (char*)last_char;
}
/*************************************************************************
 *  字符串拼接
*************************************************************************/
char* strcat(char* dest, const char* src) 
{
    ASSERT(dest != NULL && src != NULL);

    char * tmp_dest  = dest;
    const char* tmp_src   = src;

    while(*tmp_dest++);

    --tmp_dest;

    while((*tmp_dest++ = *tmp_src++));

    return tmp_dest;
}
/*************************************************************************
 *  查找字符在字符串中出现的次数
*************************************************************************/
uint32_t strchrs(const char* dest, uint8_t ch) 
{
    ASSERT(dest != NULL);

    uint32_t hit_cnt = 0;
    const char* tmp_dest = dest;

    while(*tmp_dest != 0) 
    {
        if(*tmp_dest == ch) 
        {
            hit_cnt++;
        }
        tmp_dest++;
    }

    return hit_cnt;
}
