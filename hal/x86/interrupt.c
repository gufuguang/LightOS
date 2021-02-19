// add by gu.fuguang 2021/2/10
#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

#define IDT_MOD_NAME "[IDT] "
#define IDT_DESC_CNT 0x30
/*************************************************************************
 *  8259A端口
*************************************************************************/
#define PIC_M_CTRL     0x20     // 主片控制端口
#define PIC_M_DATA     0x21     // 主片数据端口
#define PIC_S_CTRL     0xa0     // 从片控制端口
#define PIC_S_DATA     0xa1     // 从片数据端口

#define EFLAGS_IF 0x00000200
//通过把ELFLAGS压栈，再弹出到内存获取EFLAGS状态
#define GET_EFLAGS(EFLAGS_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAGS_VAR))

// 中断号对应的中断名称
char* intr_name[IDT_DESC_CNT];
// 中断服务函数数组，由中断入口函数调用
intr_handler intr_handler_array[IDT_DESC_CNT];

// 中断门描述符数组
typedef struct idt_gate_desc {
    uint16_t intr_func_low_word;
    uint16_t selector;
    uint8_t  reserved;
    uint8_t  attributes;
    uint16_t intr_func_high_word;
}IDT_GATE_DESC;
static IDT_GATE_DESC idt_gate_array[IDT_DESC_CNT];
// 中断入口函数数组，在intr_entry实现通用结构
extern intr_handler intr_entry_table[IDT_DESC_CNT];

static void create_idt_desc(IDT_GATE_DESC *idt_desc, uint8_t attr, intr_handler intr_func);
/*************************************************************************
 *  8259A中断控制器初始化
*************************************************************************/
static void pic_8259A_init(void) 
{
    put_str(IDT_MOD_NAME"8259A init start...\n");
    // 初始化主片8259A 需要按严格ICW1~ICW4顺序写入
    out8(PIC_M_CTRL, 0x11);     // ICW1 边沿触发，级联8259A
    out8(PIC_M_DATA, 0x20);     // ICW2 中断号起始0x20，内部中断占0~0x1f
    out8(PIC_M_DATA, 0x04);     // ICW3 IRQ2接从片8259A
    out8(PIC_M_DATA, 0x01);     // ICW4 EOI

     // 初始化从片8259A 
    out8(PIC_S_CTRL, 0x11);     // ICW1 边沿触发，级联8259A
    out8(PIC_S_DATA, 0x28);     // ICW2 中断号起始0x28
    out8(PIC_S_DATA, 0x02);     // ICW3 连接主片8259A IRQ2
    out8(PIC_S_DATA, 0x01);     // ICW4 EOI

    out8(PIC_M_DATA, 0xfc);     // 打开主片IRQ0，接收时钟中断
    out8(PIC_S_DATA, 0xff);     // 暂时不打开从片中断

    put_str(IDT_MOD_NAME"8259A init done\n");
}
/*************************************************************************
 * 中断描述符门结构 （高32bits）TYPE字段 D 1 1 0  D=0 16位模式 D=1 32位模式
 *-------------------------------------------------------------------------
 *|                31 ~  16        | 15 | 14~13 | 12 | 11~8 | 7~5 |  4~0  |
 *------------------------------------------------------------------------
 *|  中断处理程序在目标段内偏移31~16 | P  |  DPL  | S  | TYPE | 000 | 未使用|
 *------------------------------------------------------------------------
 * （低32bits） 
 *-------------------------------------------------------------------------
 *|                31 ~  16        |           15 ~0                      |
 *------------------------------------------------------------------------
 *|  中断处理程序在目标代码段选择子  |     中断处理程序在目标段内偏移15~0   |
 *------------------------------------------------------------------------
 *idt_desc  中断门描述符数组
 *attr      门属性
 *intr_func 中断服务函数
*************************************************************************/
static void create_idt_desc(IDT_GATE_DESC *idt_desc, uint8_t attr, intr_handler intr_func)
{
    idt_desc->intr_func_low_word = (uint32_t)intr_func & 0x0000FFFF;
    idt_desc->selector = SELECTOR_K_CODE;
    idt_desc->reserved = 0;
    idt_desc->attributes = attr;
    idt_desc->intr_func_high_word = ((uint32_t)intr_func & 0xFFFF0000) >> 16;
}
/*************************************************************************
 * 初始化中断描述符门结构
*************************************************************************/
static void idt_desc_init(void)
{
    int i;
    put_str(IDT_MOD_NAME"idt desc init start...\n");
    for(i = 0; i < IDT_DESC_CNT; i++)
    {
        //填充中断服务函数功能，bios的中断服务函数在进入保护模式后已失效
        create_idt_desc(&idt_gate_array[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str(IDT_MOD_NAME"idt desc init done\n");
}
/*************************************************************************
 * 异常中断功能初始化
*************************************************************************/
static void general_intr_handler(uint8_t vector_no) {
    uint32_t vector = vector_no;
    if (vector == 0x27 || vector == 0x2f) {
        return;
    }

    put_str("\n");
    put_str(IDT_MOD_NAME"!!!!          exection message begin   !!!\n");
    //put_str(intr_name[vector]);

    if(vector == 14)        //异常虚拟地址错误
    {
        int page_fault_vaddr = 0;
        asm("movl %%cr2, %0":"=r"(page_fault_vaddr));
        put_str("page fault addr is ");
        put_int(page_fault_vaddr);
        put_str("\n");
    }
    
    put_str(IDT_MOD_NAME"!!!!          exection message end  !!!\n");
    while(1);
}
/*************************************************************************
 * 注册中断服务函数
*************************************************************************/
void register_handler(uint8_t vector_no, intr_handler intr_func)
{
    intr_handler_array[vector_no] = intr_func;
}
/*************************************************************************
 * 异常中断功能初始化
*************************************************************************/
static void exception_init(void) 
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        intr_handler_array[i]  = general_intr_handler;
        intr_name[i]           = "unknown";
    }

    intr_name[0]  = "#DE Divide Error";
    intr_name[1]  = "#DB Debug Exception";
    intr_name[2]  = "NMI Interrupt";
    intr_name[3]  = "#BP Breakpoint Exception";
    intr_name[4]  = "#OF overflow Exception";
    intr_name[5]  = "#BR BOUND Rang Exceeded Exception";
    intr_name[6]  = "#UD Invalid Opcode Exception";
    intr_name[7]  = "#NM Device Not Available Exception";
    intr_name[8]  = "#DF Double Fault Exception";
    intr_name[9]  = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    //intrName[15]
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Aliagnment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}
/*************************************************************************
 * 中断描述符初始化
*************************************************************************/
void idt_init(void)
{
    put_str(IDT_MOD_NAME"idt init start\n");

    idt_desc_init();           // 初始化中断入口函数

    exception_init();          // 初始化异常中断处理函数

    pic_8259A_init();          // 初始化8259A中断控制器

    // IDTR 寄存器 48bits  47~16 IDT内存起始地址  15~0 IDT界限
    uint64_t idt_reg = ((sizeof(idt_gate_array) - 1) | (((uint64_t)idt_gate_array << 32) >> 16));
    asm volatile ("lidt %0" : : "m" (idt_reg));
    //uint32_t tmp = (uint32_t)((uint64_t)idt_reg >> 32);
    //put_int((uint32_t)idt_reg);
    //put_str("\n");
    //put_int(tmp);
    //put_int((uint32_t)idt_gate_array);

    put_str(IDT_MOD_NAME"idt init done\n");
}
/*************************************************************************
 * 获取中断状态位
*************************************************************************/
INTR_STATUS intr_get_status(void) 
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);

    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}
/*************************************************************************
 * 设置中断状态位
*************************************************************************/
INTR_STATUS intr_set_status(INTR_STATUS status) {
    return (status & INTR_ON) ? intr_enable() : intr_disable();
}
/*************************************************************************
 * 使能中断
*************************************************************************/
INTR_STATUS intr_enable(void) {
    INTR_STATUS old_status;
    if(INTR_ON == intr_get_status()) 
    {
        old_status = INTR_ON;

        return old_status;

    } else {
        old_status = INTR_OFF;
        asm volatile("sti");

        return old_status;
    }
}
/*************************************************************************
 * 关中断
*************************************************************************/
INTR_STATUS intr_disable(void) {
    INTR_STATUS old_status;
    if(INTR_ON == intr_get_status()) 
    {
        old_status = INTR_ON;
        asm volatile("cli" : : :"memory");

        return old_status;
    } else {
        old_status = INTR_OFF;

        return old_status;
    }
}

