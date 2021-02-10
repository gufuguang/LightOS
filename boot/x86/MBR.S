; 2021/2/5 by gu.fuguang
; 参考 FreeDOS FAT32 boot sector 
;	|-------| 1FE0:7E00
;	|BOOTSEC|
;	|RELOC.	| MBR会把0x7c00的MBR内存拷贝到此处执行
;	|-------| 1FE0:7C00
;	...
;	|-------| 2000:0200
;	|  FAT  | 一个扇区用于读硬盘数据缓冲区
;	|-------| 2000:0000
;	...
;	|-------| 0000:7E00
;	|BOOTSEC| BIOS启动的最后一项任务就是把启动盘中的MBR加载到 0000:7C00
;	|ORIGIN | 此时的CS寄存器为0
;	|-------| 0000:7C00
;	...
;	|-------|
;	|KERNEL	| 
;	|LOADED	| loader 被加载到此处执行
;	|-------| 0060:0000
%include "fat32.inc"

%define FATSEG		    0x2000  ; 读取硬盘缓冲区

segment	.text
		org	0x7c00		     ; 告诉编译器以下代码会被加载到0x7c00处运行 
Entry:  jmp	short real_start ; 短跳转，跳转到real_start 处执行（这是BIOS跳转过来后执行的第一行代码）
		nop
;-----------------------------------------------------------------------
; 以下是FAT32文件系统第一个扇区字段定义，由于使用UDisk启动，为了简单固定
; 部分字段到MBR扇区，这样通过dd写U盘MBR扇区后就会被格式成一个简单的FAT32
; 文件系统
;-----------------------------------------------------------------------
; 上面的Entry text已经占用3byte
; 从 0x03开始的8byte OEM name
db 'L' 
db 'i'
db 'g'
db 'h' 
db 't'
db 'O'
db 'S' 
db '.'
; end OEM name
; 2B 0x0200 每扇区 512B 
db 0x00 
db 0x02 
; end
; 1B 每簇扇区数
db 0x08 
; end
; 2B保留扇区 DBR后 FAT1前有保留的36个扇区
db 0x24 
db 0x00  
; end
; 1B FAT1 FAT2
db 0x02
; end
; 4B FAT32 set 0
times	4 db 0x00
; end
; 1B HDD
db 0xf8
; end
; 2B FAT32 set 0
times	2 db 0x00
; end
; 2B 每磁道扇区数 63
db 0x3f 
db 0x00 
; end
; 2B 磁头数 
db 0xff 
db 0x00 
; end
; 4B 隐藏扇区数 
db 0x3f
times	3 db 0x00
; end
; 4B 扇区总数 16M
db 0xef 
db 0xe9 
db 0xe6 
db 0x00 
; end
; 4B FAT1 FAT2 每个FAT扇区数
db 0x9e 
db 0x39 
times	2 db 0x00 
; end
times	4 db 0x00      
; 4B 根目录簇号 2 
; 根目录起始扇区=保留扇区数+隐藏扇区数+一个FAT的扇区数×FAT表个数+(起始簇号-2)x每簇的扇区数
db 0x02
times	3 db 0x00
; end
db 0x01 
db 0x00 
; 2B 备份的扇区号
db 0x06
db 0x00
; end
times	12 db 0x00  
db 0x80 ; 对应 %define drive  bp+0x40 硬盘驱动类型	
db 0x00

db 0x29
db 0x11 
db 0xd7 
db 0xbd 
db 0xee  
times	11 db 0x20
db "FAT32"
db 0x20
db 0x20
db 0x20
; 以上占用了 0x59B
;-----------------------------------------------------------------------
; 以下是MBR代码
;-----------------------------------------------------------------------
real_start:	
		cld
		cli              ; 关中断
		sub	ax, ax       ; ax 清 0
		mov	ds, ax       ; ds 清 0
		mov	bp, 0x7c00   ; bp 栈基址 0x7c00

		; 把MBR扇区从内存0x7c00 拷贝到 0x1fe0：0x7c00
		mov	ax, 0x1FE0  
		mov	es, ax       ; 上面ds已清0
		mov	si, bp       ; 源数据地址
		mov	di, bp		 ; 目的数据地址
		mov	cx, 0x0100   ; cx用于循环次数，按字拷贝256次
		rep	movsw		 ; 循环copy 256次（512B）
		; end
		
		; 跳转到新拷贝的代码区域执行 CS：ip  CS被更新为0x1FE0
		jmp	word 0x1FE0:cont

loadseg_off	dw	0, LOADSEG   ; 4B 数据 LOADSEG是 loader被加载的内存地址

;-----------------------------------------------------------------------
; 由于是跨段跳转到此处，所以需要更新部分段寄存器
;-----------------------------------------------------------------------
cont:   
		mov	ds, ax 	; 更新ds段 0x1fe0
		mov	ss, ax  ; 栈从 [0x1fe0 :0x7be0] 向上增长
        lea sp, [bp-0x20] 
		sti
		
		; 启动的方式 0x80 HDD 参见上面FAT32头定义
		mov	[drive], dl	   
		mov	si, msg_LoadBoot  ; 打印加载信息到屏幕
		call	print

calc_params:	; 计算FAT起始扇区和数据起始扇区
		xor	eax, eax
		mov	[fat_sector], eax	; FAT头内存中这个字段信息不重要，可以被利用，此处清0

		; 1、计算FAT起始扇区  
		mov	ax, [bsResSectors]  ; 读取保留扇区  0x24
		add	eax, [nHidden]      ; 保留扇区加上隐藏扇区  0x24 + 0x3f = 0x63
		mov [fat_start], eax	; 更新FAT起始扇区 0x63（保存了文件存放扇区信息）
		mov	[data_start], eax	; 更新数据起始扇区

		; 2、计算数据扇区 2个FAT FAT1 FAT2 (见FAT规范)
		mov	 eax, [bsFATs]         ; eax = 2
		imul dword [xsectPerFat]   ; 计算FAT1+FAT2扇区数  [xsectPerFat] *eax
		add	[data_start], eax	   ; 数据扇区=保留扇区+隐藏扇区+FAT1+FAT2

		; 3、确定每个扇区的大小
		mov	ax, 512	; 扇区默认512B
fatss_scan:	
		cmp	ax, [bsBytesPerSec] ; 和上面FAT头中定义的扇区大小进行比较
		jz	fatss_found
		add	ax,ax                   ; 其它大小的扇区
		inc	word [fat_secshift]
		jmp short fatss_scan
fatss_found:
		; 4、在跟目录中查找要加载的文件
		mov	eax, [xrootClst]	; 根目录簇数（参见上面的计算方法）
ff_next_clust:	
		push eax			    ; 保存参数，下面调用函数计算
		call convert_cluster
		jc   boot_error			; 错误处理

ff_next_sector:	
		les  bx, [loadsegoff_60] ; 装载ES、bx， ES = 0x60
		call readDisk            ; 把第一个数据扇区(根目录扇区)加载到内存 0x60:0 处
		xor	di, di			     ; di 清 0
		
ff_next_entry:	            ; 查找启动文件和文件存放的数据区
		mov	 cx, 11         ; FAT32 文件名 11字节规则
		mov	 si, filename   ; 源变址放入文件名
		repe cmpsb          ; 根据读取的数据循环比较文件名 cmpsb byte ptr ds:si (kernel name) es:di
		jz	ff_done		    ; 找到启动文件

		add	di, byte 0x20	; 跳到下一个文件名处
		and	di, byte -0x20  ; 保证0x20格式对齐
		cmp	di, [bsBytesPerSec]	; 判断是否超过一个扇区
		jnz	ff_next_entry  		; 不超过继续查找

		dec dx		            ; 超过一个扇区继续读一个扇区查找
		jnz ff_next_sector
				
ff_walk_fat:	           ; 一个簇没找到，查找下一簇
		pop	 eax		   ; 恢复当前簇
		call next_cluster  ; 查找下一簇
		jmp  ff_next_clust
            
ff_done: 
		push word [es:di+0x14-11]	; 文件起始簇号高16bit
		push word [es:di+0x1A-11]	; 文件起始簇号低16位
		pop	 eax 
		sub	 bx, bx	
		
read_loader:	; 读取loader
		push eax
		call convert_cluster    ; 簇号转换 
		jc	 boot_success		; 读取完成
		
rk_in_cluster:	 		         ; 读取内核数据，读取一个簇，8个扇区
		call readDisk
		dec	dx
		jnz	rk_in_cluster		 ; 是否读取完一个簇

rk_walk_fat:	
        pop  eax
		call next_cluster  		;计算下一个簇是否需要读取
		jmp	 read_loader
		
;-----------------------------------------------------------------------
boot_success:	
		mov	bl, [drive]
		jmp	far [loadsegoff_60]  ; 跳转到加载的laoder处执行

;-----------------------------------------------------------------------
; 启动过程出错处理
;-----------------------------------------------------------------------
boot_error:	
		mov	si, msg_BootError
		call	print		; 打印错误信息

wait_key:	
		xor	ah,ah
		int	0x16			; 等待一个按键
reboot:		
		int	0x19			; 重启

;-----------------------------------------------------------------------
; 根据一个簇号查找下一个簇
; input:	EAX - cluster
; output:	EAX - next cluster
;-----------------------------------------------------------------------
next_cluster:	
		push es
		push di
		push bx
		mov	 di, ax
		shl	 di, 2	  ; FAT32

		push ax
		mov	 ax, [bsBytesPerSec]
		dec	 ax
		and	 di, ax	
		pop	 ax
		shr	 eax, 7			; 根据扇区大小计算扇区号

fat_afterss:	
		add	eax, [fat_start]	; 当前绝对扇区号
		mov	bx, FATSEG          ; 读取扇区存放地址
		mov	es, bx
		sub	bx, bx

		cmp	eax, [fat_sector]	; 扇区是否已经被加载过
		jz	cn_buffered
		
		mov	 [fat_sector], eax	; 到内存0x2000:0 计算簇使用情况
		call readDisk

cn_buffered:                    ; 计算下一个需要读的簇	
		and	byte [es:di+3],0x0f	; mask out top 4 bits
		mov	eax, [es:di]		; 下一个读取的簇号

		pop	bx
		pop di
		pop	es
		ret
;-----------------------------------------------------------------------
; 根据簇号转换成扇区号
; input:	EAX - target cluster
; output:	EAX - absolute sector
;-----------------------------------------------------------------------
convert_cluster:
		cmp	eax, 0x0ffffff8	 ; 如果是最后一个簇
		jnb	end_of_chain

		; sector = (cluster-2) * clustersize + data_start
		dec	eax
		dec	eax

		movzx edx, byte [bsSecPerClust]  ; 每簇8扇区 edx = 0x8
		push  edx
		mul	  edx                ; eax = eax * edx = 0
		pop	  edx                ; edx = 0x8
		add	  eax, [data_start]  ; 0x733c + 0x63 + 0x0 = 0x739f
		ret

end_of_chain:	
		stc			     ; 文件结尾
		ret
;-----------------------------------------------------------------------
; 打印字符到屏幕 string DS:SI modifies AX BX SI
;-----------------------------------------------------------------------
printchar:	
		xor	bx, bx		; 字符界面0页
		mov	ah, 0x0e	
		int	0x10
print:		
		lodsb 
		cmp	al, 0		; 判读是否是最后一个字符
		jne	printchar	; 继续打印
		ret	 
;-----------------------------------------------------------------------
; 使用LBA方式读取一个扇区
; input:  EAX   - 32-bit DOS sector number
;		  ES:BX - destination buffer
; output: ES:BX points one byte after the last byte read.
;		  EAX - next sector
;-----------------------------------------------------------------------
readDisk:	 			
		push	dx   ; 把 dx  si di压栈
		push	si
		push	di

read_next:	
		push eax    ; 把eax压栈
		mov  di, sp	; 记录当前栈底

		; 读取硬盘的参数压栈
		push byte 0  ; [e] sector number high 32bit (16bit)    
		push byte 0	 ; [C] sector number high 32bit (16bit)  
		push eax     ; [8] sector number low 32bit
		push es	     ; [6] buffer segment 读取数据存放的内存地址
		push bx	     ; [4] buffer offset
		push byte 1	 ; [2] 1 sector (word)
		push byte 16 ; [0] size of parameter block (word)
		mov	si, sp        ; 压栈后新栈指针
		mov	dl, [drive]   ; 硬盘类型 0x80 HDD driver
		mov	ah, 42h	      ; 访问扩展存储 read disk
		int	0x13	

		mov	sp, di	      ; sp 恢复到读取硬盘前的状态
		pop	eax	
		jnc	read_ok		  ; 正确读取到数据

		push ax		
		xor	 ah, ah		  ; 出错重新尝试读取
		int  0x13
		pop  ax		
		jmp	 read_next

read_ok:	
		inc eax			             ; 下一个扇区
		add	bx, word [bsBytesPerSec] ; 更新读取到数据大小
		jnc	no_incr_es		         ; 如果数据没有溢出

		mov	dx, es      ; 如果接下来读取的数据有溢出需要更新ES
		add	dh, 0x10		
		mov	es, dx

no_incr_es:	
		pop	di  ; 恢复函数调用压栈数据
		pop si
		pop	dx
		ret
;-----------------------------------------------------------------------
msg_LoadBoot    db "[MBR] start read loader",0
msg_BootError	db "No "         ; 找不到loader文件的打印信息    
filename	    db "LOADER  BIN"
		times 0x01FC-$+$$ db 0   ; 填补若干0，保证508B，MBR 512B
sign		    dw 0, 0xAA55     ; MBR 规定 0x55 0xAA结尾