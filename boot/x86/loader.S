; 2021/2/6 by gu.fuguang
; 虽然MBR已经可以往内存中加载数据，但MBR只有512B，所有初始化工作都放在MBR中是放不下的
; 所以需要loader来做相关的初始化工作

%include "fat32.inc"
%include "GDTDesc.inc"

%define loadsegoff_60	0x7000
%define KERNELSEG	    0x7000
%define FATSEG		    0x2000
LoaderStackTop     equ  LOADER_BASE_ADDR

section loader vstart=LOADER_BASE_ADDR
jmp Begin
;*************************************************************************
;  GDT 描述符 64 bit  高 32bit
; -------------------------------------------------------------------------
; | 31~24 | 23 | 22 | 21 |  20 | 19~16 | 15 | 14~13 | 12  | 11~8 |   7~0  |
; -------------------------------------------------------------------------
; | 段基址| G  | D/B|  L | AVL | 段界限 | P  |  DPL  | S   | TYPE | 段基址 |
; -------------------------------------------------------------------------
;  GDT 描述符 64 bit  低 32bit
; -------------------------------------------------------------------------
; |            31 ~ 16                |        15  ~ 0                    |
; -------------------------------------------------------------------------
; |            段基址  15~0           |        段界限 15~0                 |
;*************************************************************************
;  GDT 描述符定义(规定第一个不可用) 描述符8B
GdtBase :      dd  0x00000000
               dd  0x00000000
; 代码段GDT描述符
CodeGdtDesc :  dd  0x0000ffff
               dd  GdtDescCodeHigh
; 数据段GDT描述符
DataGdtDesc :  dd  0x0000ffff
               dd  GdtDescDataHigh
; 显示段GDT描述符
VideoGdtDesc:  dd  0x80000007
               dd  GdtDescVideoHigh
; 当前 GDT 占用内存大小
GdtSize       equ  $ - GdtBase
GdtLimit      equ  GdtSize -1
; 预留几个描述符空间
times   60    dq 0

; GDTR 寄存器 48bit | GDT内存起始地址  47~15  | GDT 界限 15~0 |
GdtPtr        dw    GdtLimit
              dd    GdtBase
;*************************************************************************
;  启动信息
;*************************************************************************
meg_load       db "[LOADER] start find kernel"
msg_LoadKernel db "[LOADER] Loading kernel",0
msg_BootError  db "No "
filename	   db "KERNEL  BIN"  ; 内核文件名 kernel.bin

loadseg_off	   dw	0, KERNELSEG ; 内核加载的内存地址

   		msg_mem  db "[LOADER] get computer memory "
		msg_area db "[LOADER] mem start 0x00000000 len 0x00000000 Type Sys"
    	memptr   db "0000MB"
    	ards_buf times 200 db 0
    	ards_nr  dw 0
    	total_mem_bytes dd 0

Begin:
		mov ax, 0     ; 把各段寄存器清0
		mov ds, ax
		mov ss, ax
		mov fs, ax
		mov	es, ax  
		mov ax, 0xb800
		mov gs, ax    ; 显示段
		; 打印loader信息
		mov ax, meg_load ;- LOADER_BASE_ADDR
		mov bp, ax
		mov cx, 26      ; 字符串长度，不包括'\0'
		mov ax, 0x1301 
    	mov bx, 0xf
    	mov dx, 0x0100 
    	int 0x10
;*************************************************************************
;  查找加载kernel.bin 和MBR查找加载loader.bin代码相同
;*************************************************************************
calc_params:	
		mov	bp, 0x7c00   
        xor	eax, eax
		mov	[fat_sector], eax	
		mov	ax, [bsResSectors]	
		add	eax, [nHidden]  
		mov [fat_start], eax
		mov	[data_start], eax	

		mov	 eax, [bsFATs]	
		imul dword [xsectPerFat]	
		add	 [data_start], eax	
		mov	 ax, 512 
fatss_scan:	
		cmp	ax, [bsBytesPerSec]  
		jz	fatss_found
		add	ax,ax
		inc	word [fat_secshift]	
		jmp short fatss_scan 
fatss_found:
		mov	eax, [xrootClst]	 
ff_next_clust:	
		push eax		 
		call convert_cluster
		jc	 boot_error		
ff_next_sector:	
		les	 bx, [loadsegoff_60]	 
		push ax
		mov  ax, 0x7000
    	mov  es, ax
		pop  ax
		call readDisk

		xor	di, di		
ff_next_entry:	
		mov	 cx, 11
		mov	 si, filename
		repe cmpsb
		jz	 ff_done	 

		add	 di, byte 0x20 
		and	 di, byte -0x20  
		cmp	 di, [bsBytesPerSec] 
		jnz	 ff_next_entry

		dec  dx		
		jnz	 ff_next_sector

ff_walk_fat:	
  		pop	 eax			 
		call next_cluster 
		jmp	 ff_next_clust
            
ff_done:	
		push word [es:di+0x14-11] 
		push word [es:di+0x1A-11]	 
		pop	 eax			 

		sub	 bx, bx	
;-----------------------------------------------------------------------
read_kernel:	
		push eax
		call convert_cluster
		jc	 boot_success	 
 
rk_in_cluster:	
		call  readDisk
		dec	  dx
		jnz   rk_in_cluster	

rk_walk_fat:
		pop eax
		call	next_cluster
		jmp	read_kernel	
;-----------------------------------------------------------------------
boot_success:
		jmp	LoaderBegin  ; 加载完成跳转到启动代码
;-----------------------------------------------------------------------
boot_error:	
		mov	 si, msg_BootError
		call print			

wait_key:	
		xor	ah,ah
		int	0x16		
reboot:		
		int	0x19			
;-----------------------------------------------------------------------
next_cluster:	push	es
		push	di
		push	bx

		mov	di, ax
		shl	di, 2			 

		push ax
		mov	 ax, [bsBytesPerSec]
		dec	 ax
		and	 di, ax		 
		pop	 ax

		shr	eax, 7		 
fat_afterss: 
		add	eax, [fat_start] 
		mov	bx, FATSEG
		mov	es, bx
		sub	bx, bx

		cmp	eax, [fat_sector]	 
		jz	cn_buffered
 
		mov	[fat_sector],eax	 
		call	readDisk
 
cn_buffered:	
		and	byte [es:di+3],0x0f	 
		mov	eax, [es:di]	 
		pop	bx
		pop di
		pop	es
		ret
;-----------------------------------------------------------------------
convert_cluster:
		cmp	eax, 0x0ffffff8	 
		jnb	end_of_chain
 
		dec	eax
		dec	eax
 
		movzx edx, byte [bsSecPerClust]  
		push  edx
		mul	  edx           
		pop   edx           
		add	  eax, [data_start]  
		ret

end_of_chain:	
		stc	 
		ret
;-----------------------------------------------------------------------
printchar:	
		xor	bx, bx	 
		mov	ah, 0x0e 
		int	0x10	 
print:		
		lodsb		 
		cmp	  al, 0		 
		jne	  printchar	 
		ret	 
;-----------------------------------------------------------------------
readDisk:	
		push dx
		push si
		push di

read_next:	
		push eax
		mov	 di, sp	 
		push byte 0	 
		push byte 0	 
		push eax	 
		push es 
		push bx	 
		push byte 1	 
		push byte 16	 
 
		mov	si, sp   
		mov	dl, [drive]  
		mov	ah, 42h	 
		int	0x13	

		mov	sp, di 
		pop	eax	 
 
		jnc	read_ok	 

		push ax		 
		xor	 ah, ah		 
		int	 0x13
		pop	 ax		 
		jmp  read_next

read_ok:	
		inc eax		 
		add	bx, word [bsBytesPerSec]  
		jnc	no_incr_es	 

		mov	dx, es
		add	dh, 0x10	 
		mov	es, dx

no_incr_es:	
		pop	di
		pop si
		pop	dx
		ret
;-----------------------------------------------------------------------
; loader 初始化代码
;-----------------------------------------------------------------------
LoaderBegin:
		jmp  LoaderStart

LoaderStart :
		mov ax, 0     ; 把各段寄存器清0
		mov ds, ax
		mov ss, ax
		mov fs, ax
		mov	es, ax  
		mov ax, 0xb800
		mov gs, ax    ; 显示段

    	xor ebx, ebx
    	mov edx, 0x534d4150   ; 固定标签名 对应字符串"SMAP"
    	mov di,  ards_buf
.e820_memget_loop:
    	mov eax, 0x0000e820   ; BIOS 0xe820 获取内存大小
    	mov ecx, 20
    	int 0x15

    	jc .e820_failed_try_e801

    	add di, cx
    	inc word [ards_nr]
    	cmp ebx, 0

    	jnz .e820_memget_loop      

    	mov cx, [ards_nr]
    	mov ebx, ards_buf
    	;xor edx, edx
.find_max_mem_area:
    	mov  eax, [ebx]
    	mov  edx, [ebx + 8]
        mov [total_mem_bytes], edx
		call print_mem_area

    	add  ebx, 20
.next_ards:
    	loop .find_max_mem_area
    	jmp 0:protect_mode

.e820_failed_try_e801:
    	hlt       ; 获取内存失败停止在这儿
    	jmp $

print_mem_area:
		push eax
		push ebx
		push edx
		push ecx

		mov bp, msg_area + 28
    	mov cx, 0
.base_addr2char:
    	cmp cx, 8        ; 内存数字转换成字符串
		jz .mem_len
    	xor edx, edx
		mov ebx, 16
		div ebx  
		cmp dl, 9
		jg .is_A2F    
    	add dl, '0'
		jmp .store_hex
.is_A2F:
		sub dl, 10
		add dl, 'A'
.store_hex:
    	mov [bp], dl
    	inc cx
   		sub bp, 1
    	jmp .base_addr2char

.mem_len:
		mov eax, [total_mem_bytes]
		mov bp, msg_area + 43
    	mov cx, 0
.mem_len2char:
    	cmp cx, 8        ; 内存数字转换成字符串
		jz print_area_str
    	xor edx, edx
		mov ebx, 16
		div ebx 
		cmp dl, 9
		jg .is_A2F1    
    	add dl, '0'
		jmp .store_hex1
.is_A2F1:
		sub dl, 10
		add dl, 'A'
.store_hex1:      
    	mov [bp], dl
    	inc cx
   		sub bp, 1
    	jmp .mem_len2char

print_area_str:
		pop  ecx
		push ecx
		mov ax, [ards_nr]
		sub ax, cx
		shl ax, 8
		add ax, 0x200
		mov dx, ax

    	mov ax, 0
    	mov es, ax
    	mov ax, msg_area  
    	mov bp, ax
    	mov cx, 53       
    	mov ax, 0x1301 
    	mov bx, 0xf
    	int 0x10

		pop  ecx
		pop  edx
		pop  ebx
		pop  eax
		ret
;*************************************************************************
;  准备进入保护模式 分三步
;  1、打开 A20 地址线（实模式下地址回绕）
;  2、加载GDT
;  3、将cr0 pe 位置 1 保护模式使能
;*************************************************************************
protect_mode:
		in  al, 0x92            
		or  al, 0000_0010B
		out 0x92, al    ; 0x92端口的第一位置1，打开 A20

		lgdt [GdtPtr]   ; 加载GDT

		mov  eax, cr0
		or   eax, 0x00000001
		mov  cr0, eax   ; cr0  0 bit置1

		; 长跳转清空流水线，下面使用32位指令编码模式，当前为0特权级
		jmp dword SelectorCode : protectModeStart
use32
msg_cpu_mode db "[LOADER] cpu in protected mode"
protectModeStart :
    	cli    ; 先关中断，BIOS注册的中断程序在保护模式下失效，实体机上中断会触发异常
    	mov ax, SelectorData
    	mov ds, ax
    	mov es, ax
    	mov ss, ax
    	mov esp, LoaderStackTop
    	mov ax, SelectorVideo
    	mov gs, ax

		mov ecx, 30
		mov eax, [ards_nr]
		add eax, 2
		mov ebx, 160
		mul ebx
		mov edx, msg_cpu_mode
.print_protected_mode:
        mov bl, [edx]
    	mov byte [gs:eax], bl  ; 写内存方式显示字符串
		add eax, 2
		add edx, 1
		loop .print_protected_mode

    	call create_page   ; 创建页表
    	sgdt [GdtPtr]      ; 存储原GDT所在位置
		; 页表已经创建完成，低16M内存可以通过0xC0000000方式方位
    	mov ebx, [GdtPtr + 2]                   ; GDT 存放地址
    	or  dword [ebx + 0x18 + 4], 0xc0000000  ; 更新显示段地址
    	add dword [GdtPtr + 2], 0xc0000000      ; 更新GDT 的地址
    	add esp, 0xc0000000                     ; 更新栈
    	mov eax, PAGE_DIR_TABLE_POS
    	mov cr3, eax                            ; 页目录表位置

    	mov eax, cr0
    	or  eax, 0x80000000
    	mov cr0, eax                            ; 开启分页

    	lgdt [GdtPtr]                           ; 重新加载GDT
		; 以后代码通过页表转换地址
		jmp SelectorCode:enter_kernel           ; 更新了GDT，刷新流水线
enter_kernel:
%define KERNEL_ENTRY_POINT 0xc0101000           ; 内核被解析到1M + 4KB处
		call kernel_init
		mov  esp, 0xc009f000
		mov  eax, 0x7A000000                    ; 内存大小做为参数传给内核
		push eax
		push eax
		jmp KERNEL_ENTRY_POINT
    	jmp $                                   ; 进入kernel后不会再返回到这个地方来

;*************************************************************************
;  创建页目录及页表，页表放在16M起始位置
;*************************************************************************
;	   |-------|
;	      ...
;	   |-------| 
;	   |  PTE  | 
;	   |-------|0x1002000  1M + 4KB + 4KB
;	   |  PTE  | 
;	   |-------|0x1001000  1M + 4KB
;	   | 1023  |-----------------
;      |  ...  |                |
;	   |  0    |  PDE           |
;	   |-------|0x1000000 16M ---
create_page:
		mov ecx, 4096
		mov esi, 0
.clear_page_dir:               ; 16M后 4KB页目录表清0
    	mov byte [PAGE_DIR_TABLE_POS + esi] , 0
    	inc esi
    	loop .clear_page_dir

.create_pde:
		mov eax, PAGE_DIR_TABLE_POS
    	add eax, 0x1000    ;  第一个页表 16M + 4KB
    	mov ebx, eax
		; 后面更新页表地址还需要使用低!M空间，所以第1和768项页目录表相同
    	or eax, PG_US_U | PG_RW_W | PG_P    ; 设置页表内存属性
    	mov [PAGE_DIR_TABLE_POS + 0], eax   ; 第一个页目录表指向第一个页表
		; 先创建内核高1G空间前16M的页目录表，指向低端0~16M空间，可以访问0~16M 内存 
    	mov [PAGE_DIR_TABLE_POS + 0xc00], eax  
   		add eax,  0x1000
    	mov [PAGE_DIR_TABLE_POS + 0xc04], eax 
    	add eax,  0x1000
		mov [PAGE_DIR_TABLE_POS + 0xc08], eax 
    	add eax,  0x1000
		mov [PAGE_DIR_TABLE_POS + 0xc0c], eax  
		; 最后一个页目录表指向自己 0x1000000， 可以通过这个表项修改页表内容
    	sub eax, 0x4000  
    	mov [PAGE_DIR_TABLE_POS + 4092], eax    

		; 创建页目录表对应的PTE，内核使用16M低端物理地址  
    	mov ecx, 4096    
    	mov esi, 0
    	mov edx, PG_US_U | PG_RW_W | PG_P 
.create_pte: 
    	mov [ebx + esi * 4], edx   ; 给PTE表填写内容0、4KB、8KB...
    	add edx, 4096 
    	inc esi
    	loop .create_pte           ; 创建完成后内核通过页表访问低16M内存

		; 创建内核高1G空间剩余的PDE 对应的页目录 772~1022 
    	mov eax, PAGE_DIR_TABLE_POS
    	add eax, 0x5000            ; 第5个页表的位置
    	or  eax, PG_US_U | PG_RW_W | PG_P
    	mov ebx, PAGE_DIR_TABLE_POS
    	mov ecx, 251               ; 上面已经初始化了5个页目录表，此时高1G空间省251
   		mov esi, 772
.create_kernel_pde:
    	mov [ebx + esi * 4], eax   ; 内核空间的页目录表 772 ~ 1022 
    	inc esi
    	add eax, 0x1000            ; 下一个页目录表的页表空间
    	loop .create_kernel_pde
   		ret

;*************************************************************************
;  解析内核镜像（elf解析） 内核被加载到 0x70000内存处
;*************************************************************************
kernel_init:
		xor eax, eax
		xor ebx, ebx
		xor ecx, ecx
		xor edx, edx

		mov dx, [KERNEL_BIN_BASE_ADDR + 42]      ; elf header 偏移42字节记录 program header大小
		mov ebx, [KERNEL_BIN_BASE_ADDR + 28]     ; 记录第一个program header的偏移量  
		add ebx, KERNEL_BIN_BASE_ADDR            ; program header 地址
		mov cx, [KERNEL_BIN_BASE_ADDR + 44]      ; program header 个数

.each_segment:
		cmp byte [ebx +0], PT_NULL               ; program header 0类型表示未使用
		je .PT_NULL

		push dword [ebx + 16]                    ; 压入拷贝字节数 

		mov  eax, [ebx + 4]                      ; 要拷贝的数据的源地址
		add  eax, KERNEL_BIN_BASE_ADDR
		push eax

		push dword [ebx + 8]                     ; 目的地址 p_vaddr

		call mem_cpy
		add esp, 12                              ; 压入的三个参数弹栈

.PT_NULL:
		add ebx, edx                              ; 加上一个头大小进入下一个program header
		loop .each_segment
		ret

mem_cpy:
		cld
		push ebp
		mov  ebp, esp
		push ecx

		mov edi, [ebp + 8]
		mov esi, [ebp + 12]
		mov ecx, [ebp + 16]
		rep movsb

		pop ecx
		pop ebp
		ret