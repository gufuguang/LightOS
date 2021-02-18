; add by gu.fuguang 2021/2/10
[bits 32]
%define ERROR_CODE nop
%define ZERO push 0

extern intr_handler_array

section .data
global intr_entry_table
intr_entry_table:

; 宏定义中断向量表 两个参数
%macro VECTOR 2         
section .text
intr%1entry: 
    %2                  ; 处理错误码压栈
    push ds
    push es
    push fs
    push gs
    pushad
    
    mov al, 0x20        ; 向主片和从片8259A发送EOI中断结束命令
    out 0xa0, al
    out 0x20, al

    push %1             ; 压入中断向量号，调用中断处理函数
    call [intr_handler_array + %1 * 4]
    jmp intrExit

section .data
    dd intr%1entry     ; 中断入口函数地址表， 编译器会把数据段和代码段分开存放

%endmacro

section .text
global intrExit
intrExit:               ; 中断退出弹栈
    add esp, 4
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4
    iretd

;*************************************************************************
;  定义中断入口函数表
;*************************************************************************
VECTOR 0x00, ZERO       ; intr0entry
VECTOR 0x01, ZERO       ; intr1entry
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ZERO
VECTOR 0x09, ZERO
VECTOR 0x0a, ZERO
VECTOR 0x0b, ZERO
VECTOR 0x0c, ZERO
VECTOR 0x0d, ZERO
VECTOR 0x0e, ZERO
VECTOR 0x0f, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ZERO
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO
VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ZERO
VECTOR 0x19, ZERO
VECTOR 0x1a, ZERO
VECTOR 0x1b, ZERO
VECTOR 0x1c, ZERO
VECTOR 0x1d, ZERO
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO
VECTOR 0x20, ZERO       ; intr32entry 时钟中断
VECTOR 0x21, ZERO
VECTOR 0x22, ZERO
VECTOR 0x23, ZERO
VECTOR 0x24, ZERO
VECTOR 0x25, ZERO
VECTOR 0x26, ZERO
VECTOR 0x27, ZERO
VECTOR 0x28, ZERO
VECTOR 0x29, ZERO
VECTOR 0x2a, ZERO
VECTOR 0x2b, ZERO
VECTOR 0x2c, ZERO
VECTOR 0x2d, ZERO
VECTOR 0x2e, ZERO
VECTOR 0x2f, ZERO
