#ifndef __LIB_STRING_H
#define __LIB_STRING_H
#include "stdint.h"
void memset(void* dest, uint8_t value, uint32_t size);
void memcpy(void* dest, const void* src, uint32_t size);
int memcmp(const void* _a, const void* _b, uint32_t size);
char* strcpy(char* dest, const char* src);
uint32_t strlen(const char* src);
int8_t strcmp(const char* a, const char* b);
char* strchr(const char* src, const uint8_t ch);
char* strrchr(const char* src, const uint8_t ch);
char* strcat(char* dest, const char* src);
uint32_t strchrs(const char* dest, uint8_t ch);
#endif