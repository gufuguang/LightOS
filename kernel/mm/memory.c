// add by gu.fuguang 2021/2/15
#include "global.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "print.h"
#include "semaphore.h"

#define MM_MOD_NAME "[MM] "
/*************************************************************************
 *  位图初始化，把位图中所有数据清0
*************************************************************************/
#define PAGE_SIZE        4096
#define MM_BITMAP_BASE   0xc0800000     // 8M 内存管理位图存放地址
#define K_HEAP_START     0xc2000000     // 32M 内核申请的起始地址
#define KERNEL_RESERVE   0x2000000      // 预留低端32M给内核使用
/*************************************************************************
 *  定义物理内存池和虚拟内存池
*************************************************************************/
// 整个物理内存池管理
typedef struct phy_mm_pool {
    BITMAP      mm_pool_bitmap;
    uint32_t    phy_addr_start;
    uint32_t    mm_pool_size;
    THREAD_LOCK lock;
}PHY_MM_POOL;

PHY_MM_POOL      k_phy_mm_pool, app_phy_mm_pool;
VIRTUAL_MM_POOL  k_vir_mm_pool;
/*************************************************************************
 *  在虚拟内存池中申请pg_cnt个虚拟内存页
*************************************************************************/
static void* vaddr_get(MM_POOL_TYPE pg_type, uint32_t pg_cnt) 
{
    uint32_t  vaddr_start = 0;
    int32_t   bit_offset_start = -1;
    uint32_t  cnt = 0;

    if(pg_type == PF_KERNEL)   // 内核内存池
    {
        bit_offset_start = bitmap_scan(&k_vir_mm_pool.vaddr_bitmap, pg_cnt);
        if(bit_offset_start == -1)
        {
            return NULL;
        }
        while(cnt < pg_cnt)
        {
            bitmap_set(&k_vir_mm_pool.vaddr_bitmap, bit_offset_start + cnt++, 1);
        }
        vaddr_start = k_vir_mm_pool.vaddr_start + bit_offset_start * PG_SIZE;
    } else {    // 用户内存池
        THREAD_PCB* pcb = get_cur_thread_pcb();

        bit_offset_start = bitmap_scan(&pcb->app_mm_pool.vaddr_bitmap, pg_cnt);
        if(bit_offset_start == -1)
        {
            return NULL;
        }

        while(cnt < pg_cnt)
        {
            bitmap_set(&pcb->app_mm_pool.vaddr_bitmap, bit_offset_start + cnt, 1);
            cnt++;
        }
        vaddr_start = pcb->app_mm_pool.vaddr_start + bit_offset_start * PG_SIZE;

        // APP申请的空间不能覆盖到内核空间
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }

    return (void *)vaddr_start;
}
/*************************************************************************
 *  虚拟地址对应的页表内一项的虚拟地址，可以访问到页表的内容
*************************************************************************/
uint32_t* vaddr_pte_ptr( uint32_t vaddr)
{
    uint32_t* pte = (uint32_t *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_OFFSET(vaddr) * 4);

    return pte;
}
/*************************************************************************
 *  虚拟地址对应的页目录表内一项的虚拟地址，可以访问到页目录表的内容
*************************************************************************/
uint32_t* vaddr_pde_ptr(uint32_t vaddr)
{
    uint32_t* pde = (uint32_t *)((0xfffff000) + PDE_OFFSET(vaddr) * 4);

    return pde;
}
/*************************************************************************
 *  从内存池中申请一页内存,返回物理地址
*************************************************************************/
static void* palloc(PHY_MM_POOL* mm_pool)
{
    int bit_offset = bitmap_scan(&mm_pool->mm_pool_bitmap, 1);
    if(bit_offset == -1)
    {
        return NULL;
    }
    bitmap_set(&mm_pool->mm_pool_bitmap, bit_offset, 1);
    uint32_t pg_phy_addr = ((bit_offset * PAGE_SIZE) + mm_pool->phy_addr_start);

    return (void *)pg_phy_addr;
}
/*************************************************************************
 *  在虚拟地址页表上填上物理地址
*************************************************************************/
static void page_table_add(void* vaddr, void* pg_phy_addr)
{
    uint32_t  pg_add_addr = (uint32_t)pg_phy_addr;
    uint32_t* pde = vaddr_pde_ptr((uint32_t)vaddr);  // 获取虚拟地址对应的PDE位置虚拟地址
    uint32_t* pte = vaddr_pte_ptr((uint32_t)vaddr);

    if(*pde & 0x1)
    {
        ASSERT(!(*pte & 0x1));
        if(!(*pte & 0x1))
        {
            *pte = (pg_add_addr | PG_US_U | PG_RW_W | PG_P_1);
        } else {
            PANIC("pte repeat");
            *pte = (pg_add_addr | PG_US_S | PG_RW_W | PG_P_1);
        }
    } else {  // 页目录项不存在，需要先申请一页的页表，把页表物理地址填写页目录项内，再填写页表项
        uint32_t pte_pg_addr = (uint32_t)palloc(&k_phy_mm_pool);

        *pde = (pte_pg_addr | PG_US_U | PG_RW_W | PG_P_1);
        // 把页表内容情0，pte是页表内偏移后的地址，把低12位清0，即为页表的虚拟地址
        memset((void *)((int)pte & 0xfffff000), 0, PAGE_SIZE);

        ASSERT(!(*pte & 0x1));
        *pte = (pg_add_addr | PG_US_U | PG_RW_W | PG_P_1);
    }
}
/*************************************************************************
 *  分配指定页数的内存空间，返回虚拟地址
*************************************************************************/
void* malloc_page(MM_POOL_TYPE pg_type, uint32_t pg_cnt)
{
    ASSERT(pg_cnt > 0);

    // 先获取需要的虚拟页
    void* vaddr_start = vaddr_get(pg_type, pg_cnt);
    if(vaddr_start == NULL)
    {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t cnt = pg_cnt;
    PHY_MM_POOL* mm_pool = pg_type & PF_KERNEL ? & k_phy_mm_pool : &app_phy_mm_pool;

    while(cnt-- > 0)
    {
        void* pg_phy_addr = palloc(mm_pool);
        if(pg_phy_addr == NULL) 
        {
            return NULL;
        }
        // 把物理地址和虚拟地址在页表中关联起来
        page_table_add((void*)vaddr, pg_phy_addr);
        vaddr += PAGE_SIZE;
    }

    return vaddr_start;
}
/*************************************************************************
 *  获取一页内核内存空间，返回虚拟地址
*************************************************************************/
void* get_kernel_pages(uint32_t pg_cnt) 
{
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if(vaddr != NULL)
    {
        memset(vaddr, 0, pg_cnt * PAGE_SIZE);
    }

    return vaddr;
}

/*************************************************************************
 *  获取一页APP内存空间，返回虚拟地址
*************************************************************************/
void* get_app_pages(uint32_t pg_cnt) 
{
    lock_request(&app_phy_mm_pool.lock);
    void* vaddr = malloc_page(PF_APP, pg_cnt);
    if(vaddr != NULL)
    {
        memset(vaddr, 0, pg_cnt * PAGE_SIZE);
    }

    lock_release(&app_phy_mm_pool.lock);

    return vaddr;
}
/*************************************************************************
 *  把一页虚拟地址和物理地址相关联
*************************************************************************/
void* get_one_page(MM_POOL_TYPE pg_type, uint32_t vaddr) 
{
    PHY_MM_POOL* mm_pool = pg_type & PF_KERNEL ? &k_phy_mm_pool : &app_phy_mm_pool;
    lock_request(&mm_pool->lock);

    THREAD_PCB* pcb = get_cur_thread_pcb();
    int32_t  bit_idx = -1;

    if(pcb->pg_dir != NULL && pg_type == PF_APP) // 用户空间添加虚拟内存占用
    {
        bit_idx = (vaddr - pcb->app_mm_pool.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&pcb->app_mm_pool.vaddr_bitmap, bit_idx, 1);
    }
    else if(pcb->pg_dir == NULL && pg_type == PF_KERNEL) 
    {
        bit_idx = (vaddr - k_vir_mm_pool.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&k_vir_mm_pool.vaddr_bitmap, bit_idx, 1);
    }
    else
    {
        PANIC("get_one_pages error\n");
    }

    void* pg_phy_addr = palloc(mm_pool);     //申请物理内存
    if(pg_phy_addr == NULL)
    {
        return NULL;
    }

    page_table_add((void*)vaddr, pg_phy_addr); //物理内存和虚拟内存关联
    lock_release(&mm_pool->lock);

    return (void*)vaddr;
}
/*************************************************************************
 *  虚拟地址转换成物理地址
*************************************************************************/
uint32_t addr_v2p(uint32_t vaddr)
{
    uint32_t* pte = vaddr_pte_ptr(vaddr);

    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}
/*************************************************************************
 *  内存池初始化，参数是内存大小
*************************************************************************/
static void mm_pool_init(uint32_t mm_size) 
{
    put_str(MM_MOD_NAME"mem pool int start \n");
    uint32_t used_mm = KERNEL_RESERVE;
    uint32_t free_mm = mm_size - used_mm;
    uint32_t all_free_page = free_mm / PAGE_SIZE;    // 整个内存可用的页数   

    uint32_t k_free_page = (256 * 1024 * 1024) / 4096 ; //内核预留256M空间
    uint32_t app_free_page = all_free_page - k_free_page;

    uint32_t k_bitmap_len = k_free_page / 8;       // 内核页数需要的位图管理空间
    uint32_t app_bitmap_len = app_free_page / 8;   // APP页数需要的位图管理空间

    uint32_t k_phy_start = used_mm;   // 内核使用的起始物理地址
    uint32_t app_phy_start = used_mm + k_free_page * PAGE_SIZE;     // APP使用的起始物理地址

    // 初始化内核管理物理内存池
    k_phy_mm_pool.phy_addr_start = k_phy_start;
    k_phy_mm_pool.mm_pool_size = k_free_page * PAGE_SIZE;
    k_phy_mm_pool.mm_pool_bitmap.bitmap_bytes_len = k_bitmap_len;
    k_phy_mm_pool.mm_pool_bitmap.bits = (void *)MM_BITMAP_BASE;
    bitmap_init(&k_phy_mm_pool.mm_pool_bitmap);
    // 初始化内核管理虚拟内存池
    k_vir_mm_pool.vaddr_bitmap.bitmap_bytes_len = k_bitmap_len;
    k_vir_mm_pool.vaddr_bitmap.bits = (void *)(MM_BITMAP_BASE + k_bitmap_len + 4096);
    k_vir_mm_pool.vaddr_start = K_HEAP_START;
    bitmap_init(&k_vir_mm_pool.vaddr_bitmap);
    put_str(MM_MOD_NAME"kernel mem pool bitmap init:");
    put_int((int)k_phy_mm_pool.mm_pool_bitmap.bits);
    put_str("  phy addr start:");
    put_int(k_phy_mm_pool.phy_addr_start);
    put_str("\n");

    // 初始化APP管理内存池
    app_phy_mm_pool.phy_addr_start = app_phy_start;
    app_phy_mm_pool.mm_pool_size = app_free_page * PAGE_SIZE;
    app_phy_mm_pool.mm_pool_bitmap.bitmap_bytes_len = app_bitmap_len;
    app_phy_mm_pool.mm_pool_bitmap.bits = (void *)(MM_BITMAP_BASE + (k_bitmap_len + 4096)*2 );
    bitmap_init(&app_phy_mm_pool.mm_pool_bitmap);
    put_str(MM_MOD_NAME"app mem pool bitmap init:");
    put_int((int)app_phy_mm_pool.mm_pool_bitmap.bits);
    put_str("  phy addr start:");
    put_int(app_phy_mm_pool.phy_addr_start);
    put_str("\n");

    put_str(MM_MOD_NAME"mem pool init done \n");
}
/*************************************************************************
 *  内存管理初始化
*************************************************************************/
void mm_init(uint32_t mm_size)
{
    put_str(MM_MOD_NAME"mem init start\n");
    mm_pool_init(mm_size);
    lock_init(&k_phy_mm_pool.lock);
    lock_init(&app_phy_mm_pool.lock);
    put_str(MM_MOD_NAME"mem init done \n");
}