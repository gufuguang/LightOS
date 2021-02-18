#******************************************************************************
#	Makefile
#******************************************************************************
obj = ./obj

#******************************************************************************
#  Loader加载程序的入口
#******************************************************************************
ENTRY_POINT = 0xc0101000
#******************************************************************************
#  编译器相关
#******************************************************************************
AS = nasm
CC = gcc
LD = ld

LIB = -I inc/ -I inc/x86 -I hal/x86 -I dev/timer -I dev/keyboard -I lib/string \
		-I lib/bitmap -I lib/list -I kernel/mm -I kernel/sem -I kernel/tty \
		-I kernel/ -I kernel/thread -I kernel/debug -I lib/ioqueue -I dev/keyboard \
		-I kernel/app

LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(obj)/kernel.map
################ C ##############################
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
SRC = $(wildcard kernel/*.c) $(wildcard kernel/thread/*.c) $(wildcard kernel/debug/*.c) \
	  $(wildcard kernel/mm/*.c) $(wildcard lib/string/*.c) $(wildcard lib/bitmap/*.c) \
 	  $(wildcard lib/list/*.c) $(wildcard hal/x86/*.c) $(wildcard dev/timer/*.c) \
	  $(wildcard dev/keyboard/*.c) $(wildcard kernel/sem/*.c) $(wildcard kernel/tty/*.c) \
	  $(wildcard lib/ioqueue/*.c) $(wildcard kernel/app/*.c)

OBJS = $(patsubst %.c,${obj}/%.o,$(notdir ${SRC}))

$(OBJS): $(obj)/%.o: $(SRC)
	$(CC) $(CFLAGS) -c $?
	-mv *.o $(obj)/
################ ASM ##############################
ASFLAGS = -f elf32
ASM_SRC = $(wildcard kernel/thread/*.S) $(wildcard hal/x86/*.S) 
ASM_OBJS = $(patsubst %.S,${obj}/%.o,$(notdir ${ASM_SRC}))

$(obj)/intr_entry.o: hal/x86/intr_entry.S
	$(AS) $(ASFLAGS) $< -o $@
$(obj)/print.o: hal/x86/print.S
	$(AS) $(ASFLAGS) $< -o $@
$(obj)/switch.o: kernel/thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@
############### LD ###############################
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(obj)/kernel.map
$(obj)/kernel.bin: $(OBJS) $(ASM_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
############### 目标 ###############################
.PHONY : clean all

clean:
	cd $(obj) && rm -rf ./*

all: $(obj)/kernel.bin