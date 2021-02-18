#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"

#define PG_P_1  1
#define PG_P_0  0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

// 虚拟地址对应在页表内的表项偏移
#define PDE_OFFSET(vaddr) ((vaddr & 0xffc00000) >> 22)
#define PTE_OFFSET(vaddr) ((vaddr & 0x003ff000) >> 12)

// 内存池类型
typedef enum mm_pool_type {
    PF_KERNEL = 1,
    PF_APP = 2,
}MM_POOL_TYPE;

// 虚拟地址管理
typedef struct virtual_mm_pool {
    BITMAP   vaddr_bitmap;
    uint32_t vaddr_start;
}VIRTUAL_MM_POOL;


//extern PHY_MM_POOL kernel_mm_pool, app_mm_pool;

// 虚拟地址对应的PTE和PDE的表项虚拟地址，可以通过此地址来修改页表
uint32_t* vaddr_pte_ptr( uint32_t vaddr);
uint32_t* vaddr_pde_ptr(uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

void mm_init(uint32_t mm_size);

void* malloc_page(MM_POOL_TYPE pg_type, uint32_t pg_nt);
void* get_kernel_pages(uint32_t pg_nt);
void* get_app_pages(uint32_t pg_cnt);
void* get_one_page(MM_POOL_TYPE pg_type, uint32_t vaddr);

#endif