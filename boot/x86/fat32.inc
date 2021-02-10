;-----------------------------------------------------------------------
; 下面是宏定义，定义FAT32文件系统头相关字节偏移量
;  MBR代码把	bp 初始化成 7c00h
;-----------------------------------------------------------------------
; %define bsOemName	    bp+0x03	; OEM label (8)
%define bsBytesPerSec	bp+0x0b ; bytes/sector (dw)
%define bsSecPerClust	bp+0x0d	; sectors/allocation unit (db)
%define bsResSectors	bp+0x0e	; # reserved sectors (dw)
%define bsFATs		    bp+0x10	; # of fats (db)
%define sectPerTrack	bp+0x18	; # sectors/track
; %define nHeads	    bp+0x1a	; # heads (dw)
%define nHidden		    bp+0x1c	; # hidden sectors (dd)
; %define nSectorHuge	bp+0x20	; # sectors if > 65536 (dd)
%define xsectPerFat  	bp+0x24	; Sectors/Fat (dd)
			                    ; +0x28 dw flags (for fat mirroring)
			                    ; +0x2a dw filesystem version (usually 0)
%define xrootClst	    bp+0x2c	; Starting cluster of root directory (dd)
			                    ; +0x30 dw -1 or sector number of fs.-info sector
			                    ; +0x32 dw -1 or sector number of boot sector backup
			                    ; (+0x34 .. +0x3f reserved)
%define drive		    bp+0x40	; Drive number 硬盘类型
%define loadsegoff_60	bp+loadseg_off-Entry
%define fat_sector	    bp+0x44	; last accessed FAT sector (dd); (overwriting unused bytes)
%define fat_start	    bp+0x48 ; first FAT sector (dd) (overwriting unused bytes)
%define data_start	    bp+0x4c ; first data sector (dd) (overwriting unused bytes)
%define	fat_secshift    fat_afterss-1 ; each fat sector describes 2^??  clusters (db) (selfmodifying)

; loader被加载的内存地址
%define LOADSEG		     0x0060 ; loader 加载地址
LOADER_BASE_ADDR  equ    0x0600 ; loader 加载地址

; kernel 被加载的内存地址
%define KERNEL_BIN_BASE_ADDR	0x70000
%define PT_NULL                 0