# whoolOS
a easy os for x86
nasm -f elf -o arch/x86/intEntry.o arch/x86/intEntry.S 




gcc -m32 -I arch/x86/ -c -fno-builtin -o obj/main.o kernel/main.c

nasm -f elf32 -o obj/print.o arch/x86/print.S 

nasm -f elf32 -o obj/intEntry.o arch/x86/intEntry.S 

gcc -m32 -I arch/x86/ -I inc -I inc/x86 -c -fno-builtin -o obj/interrupt.o arch/x86/interrupt.c

gcc -m32 -I arch/x86/ -I inc -I inc/x86 -c -fno-builtin -o obj/bspInit.o arch/x86/bspInit.c

ld -m elf_i386 -Ttext 0xc0001500 -e main -o obj/kernel.bin obj/main.o obj/bspInit.o obj/interrupt.o obj/print.o obj/intEntry.o