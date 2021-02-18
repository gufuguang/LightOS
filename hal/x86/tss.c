// add by gu.fuguang 2021/2/18
#include "tss.h"
#include "string.h"
#include "print.h"

#define TSS_MOD_NAME "[TSS] "

static TSS core_tss;
/*************************************************************************
 *  更新0特权级线程栈
*************************************************************************/
void update_tss_esp0(THREAD_PCB* pcb)
{
    core_tss.esp0 = (uint32_t*)((uint32_t)pcb + PG_SIZE);
}
/*************************************************************************
 *  创建GDT描述符,填充相关字段
*************************************************************************/
static GDT_DESC gdt_desc_init(uint32_t* desc_addr, uint32_t limit, \
                                uint8_t attr_low, uint8_t attr_high)
{
    uint32_t desc_base = (uint32_t)desc_addr;
    GDT_DESC desc;

    desc.limit_low_word = limit & 0x0000ffff;
    desc.base_low_word = desc_base & 0x0000ffff;
    desc.base_mid_byte = ((desc_base & 0x00ff0000) >> 16);
    desc.attr_low_byte = attr_low;
    desc.limit_attr_high = ((limit & 0x000f0000) >> 16) + (uint8_t)attr_high;
    desc.base_high_byte = desc_base >> 24;

    return desc;
}
/*************************************************************************
 *  在GDT中加入TSS段
*************************************************************************/
void tss_init(void)
{
    put_str(TSS_MOD_NAME"TSS init start\n");

    memset(&core_tss, 0, sizeof(TSS));
    core_tss.ss0 = SELECTOR_K_STACK;
    core_tss.io_base = sizeof(TSS);

    uint64_t gdtr_data = 0; 
    asm volatile("sgdt %0": "=m"(gdtr_data)::);      // 更新GDTR

    //TSS加入GDT loader中GDT被放在0x600处， tss 0x600 + 0x20  
    *((GDT_DESC*)((uint32_t)(gdtr_data >> 16) + 0x20)) = gdt_desc_init((uint32_t*)&core_tss, \
                                sizeof(core_tss)-1, TSS_ATTR_LOW, TSS_ATTR_HIGH);

    //APP CODE加入GDT l
    *((GDT_DESC*)((uint32_t)(gdtr_data >> 16) + 0x28)) = gdt_desc_init((uint32_t*)0, 0xfffff, \
                                    GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);

    //APP data加入GDT 
    *((GDT_DESC*)((uint32_t)(gdtr_data >> 16) + 0x30)) = gdt_desc_init((uint32_t*)0, 0xfffff, \
                                    GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);  

    gdtr_data = ((8*7 -1) | gdtr_data);

    asm volatile("lgdt %0": : "m"(gdtr_data));      // 更新GDTR
    asm volatile("ltr %w0": : "r"(SELECTOR_TSS));   // 加载TR

    put_str(TSS_MOD_NAME"TSS init done\n");
}
