;*************************************************************************
;  GDT 描述符 64 bit  高 32bit
; -------------------------------------------------------------------------
; | 31~24 | 23 | 22 | 21 |  20 | 19~16 | 15 | 14~13 | 12  | 11~8 |   7~0  |
; ------------------------------------------------------------------------
; | 段基址| G  | D/B|  L | AVL | 段界限 | P  |  DPL  | S   | TYPE | 段基址 |
; ------------------------------------------------------------------------
;*************************************************************************
GdtDesc_G_4K      equ   1_00000000000000000000000b   ; G位为1，以4K计算段界限大小	
GdtDesc_D_32      equ    1_0000000000000000000000b   ; D/B位为1，32位模式
GdtDesc_L         equ     0_000000000000000000000b   ; L 位为0 32位代码段， 1是64位代码段

GdtLimitCode2     equ     1111_0000000000000000b    ; 代码段高4位段界限
GdtLimitData2     equ     1111_0000000000000000b    ; 数据段高4位段界限
GdtLimitVideo2    equ     0000_0000000000000000b    ; 显示段高4位段界限

GdtDesc_P         equ         1_000000000000000b    ; 段在内存中为 1

GdtDesc_DPL0      equ          00_0000000000000b    ; 特权级 0
GdtDesc_DPL1      equ          01_0000000000000b    ; 特权级 1
GdtDesc_DPL2      equ          10_0000000000000b    ; 特权级 2
GdtDesc_DPL3      equ          11_0000000000000b    ; 特权级 3
	
GdtDesc_S_Code    equ           1_000000000000b     ; S = 1非系统段
GdtDesc_S_Data    equ           1_000000000000b     ; 非系统段
GdtDesc_S_Sys     equ           0_000000000000b     ; S = 0系统段
GdtDesc_Type_Code equ            1000_00000000b     ; 代码段类型只执行代码 
GdtDesc_Type_Data equ            0010_00000000b     ; 数据段类型不可执行

; 代码段高32Bit定义
GdtDescCodeHigh equ (0x00 << 24) + GdtDesc_G_4K + GdtDesc_D_32 + GdtDesc_L \
                     + GdtLimitCode2 + GdtDesc_P + GdtDesc_DPL0 + GdtDesc_S_Code \
                     + GdtDesc_Type_Code + 0x00

; 数据段高32Bit定义
GdtDescDataHigh equ (0x00 << 24) + GdtDesc_G_4K + GdtDesc_D_32 + GdtDesc_L \
                     + GdtLimitData2 + GdtDesc_P + GdtDesc_DPL0 + GdtDesc_S_Data \
                     + GdtDesc_Type_Data + 0x00

; 显示段高32Bit定义
GdtDescVideoHigh equ (0x00 << 24) + GdtDesc_G_4K + GdtDesc_D_32 + GdtDesc_L \
                     + GdtLimitVideo2 + GdtDesc_P + GdtDesc_DPL0 + GdtDesc_S_Data \
                     + GdtDesc_Type_Data + 0x0B

;*************************************************************************
;  CS 在32位保护模式下成为选择子 
; ---------------------------------
; |    15 ~ 3   |   3   |   2~0   |
; ---------------------------------
; | 描述符索引值 |  TI   |   RPL   |
; ---------------------------------
;*************************************************************************
TI_GDT   equ   000b    ; 使用全局描述符
TI_LDT   equ   100b    ; 局部描述符

RPL0     equ    00b    ; 特权级相关
RPL1     equ    01b
RPL2     equ    10b
RPL3     equ    11b

; 选择子属性
SelectorCode  equ  (0x0001 << 3) + TI_GDT + RPL0
SelectorData  equ  (0x0002 << 3) + TI_GDT + RPL0
SelectorVideo equ  (0x0003 << 3) + TI_GDT + RPL0

; 页表属性位
PAGE_DIR_TABLE_POS equ  0x1000000  ; 页表放在16M起始位置
PG_P               equ  1b         ; 在内存中标志位
PG_RW_R            equ  00b        ; 内存读写属性
PG_RW_W            equ  10b
PG_US_S            equ  000b
PG_US_U            equ  100b       ; 用户属性，所以特权级都可以访问