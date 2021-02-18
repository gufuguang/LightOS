; add by gu.fuguang 2021/2/10

TI_GDT   equ  0
RPL0     equ  0
;loader  中显示段在代码段和数据段之后
SELECTOR_VIDEO  equ  (0x0003 << 3) + TI_GDT + RPL0

section .data
put_int_buff  dq 0           ; 存放要打印的整数的字符
[bits 32]
section .text
;*************************************************************************
; 输出字符串到屏幕
;*************************************************************************
global put_str                    
put_str:                     ; 在屏幕上打印字符串
        push ebx
        push ecx
        xor  ecx, ecx
        mov  ebx, [esp + 12] ; 获取压入栈中的字符串地址
.loop_put_char:
        mov  cl, [ebx]
        cmp  cl, 0           ; 判断是否是字符串结尾
        jz   .str_put_over   ; 跳到字符串输出完成
        push ecx             ; 把要打印的字符压栈
        call put_char
        add  esp, 4          ; ecx 出栈
        inc  ebx             ; 下一个字符
        jmp  .loop_put_char 
.str_put_over:
        pop  ecx
        pop  ebx
        ret
;*************************************************************************
; 输出一个字符到屏幕
;*************************************************************************
global put_char
put_char:                   ; 在屏幕上输出一个字符
        pushad
        mov ax, SELECTOR_VIDEO
        mov gs, ax

        mov dx, 0x03d4      ; 获取当前光标位置
        mov al, 0x0e        ; 高8位光标位置
        out dx, al
        mov dx, 0x03d5
        in  al, dx
        mov ah, al
        mov dx, 0x03d4
        mov al, 0x0f        ; 低8位光标位置
        out dx, al
        mov dx, 0x03d5
        in  al, dx

        mov bx, ax
        mov ecx, [esp + 36] ; 获取要打印的字符 pushad压入了8x4=32字节

        cmp cl, 0xd         ; CR 判断是否是回车
        jz .is_carriage_return
        cmp cl, 0xa         ; LF 判断是否是换行
        jz .is_line_feed

        cmp cl, 0x08
        jz  .is_backspace
        jmp .put_other

.is_backspace:             ; 回退键处理
        dec bx
        shl bx, 1          ; 光标位置减1乘2即为上一个字符的位置
        mov word [gs:bx], 0x0720 ; 把删除的字符填空白
        ;inc bx
        ;mov byte [gs:bx], 0x07
        shr bx, 1
        jmp .set_cursor

.put_other:
        shl bx, 1

        mov [gs:bx], cl
        inc bx,
        mov byte [gs:bx], 0x07
        shr bx, 1
        inc bx
        cmp bx, 2000
        jl .set_cursor

.is_line_feed:              ; 如果是回车和换行字符
.is_carriage_return:
        xor dx, dx          ; 被除数的高16位
        mov ax, bx          ; 被除数的低16位，光标位置
        mov si, 80 
        div si              ; 余数放在 dx中
        sub bx, dx          ; 光标取整数
.is_carriage_return_end:
        add bx, 80          ; 光标更新到下一行位置  
        cmp bx, 2000        ; 判断是否超过一屏最后一行
.is_line_feed_end:
        jl .set_cursor      ; 未超过一屏，更新光标位置

.roll_screen:               ; 超过一屏，需要把上面的字符依次向上位移一行
        cld
        mov ecx, 960        ; 四字节搬运，需要960次，（2000-80）*2 / 4 
        mov esi, 0xc00b80a0
        mov edi, 0xc00b8000 ; 依次把下一行搬运到上一行
        rep movsd

        mov ebx, 3840
        mov ecx, 80         ; 清空最后一行
.cls_last_line:
        mov  word [gs:ebx], 0x0720  ; 填充空格键
        add  ebx, 2
        loop .cls_last_line
        mov  bx, 1920       ; 光标更新到最后一行

.set_cursor:
        mov dx, 0x03d4      ; 更新光标高8位
        mov al, 0x0e
        out dx, al
        mov dx, 0x3d5
        mov al, bh
        out dx, al
        mov dx, 0x03d4      ; 更新光标低8位
        mov al, 0x0f
        out dx, al
        mov dx, 0x03d5
        mov al, bl
        out dx, al
.put_char_done:             ; 完成一个字符的打印
        popad
        ret
;*************************************************************************
; 输出一个整数到屏幕
;*************************************************************************
global put_int
put_int:
        pushad
        mov ebp, esp
        mov eax, [ebp + 4 * 9] ; 取出要打印的整数
        mov edx, eax
        mov edi, 7             ; 转换后字符在buff中的位置
        mov ecx, 8             ; 一个整数对应8个字符
        mov ebx, put_int_buff
.4bits_hex2char:               ; 4bits的数转换成一个字符显示
        and edx, 0x0000000F
        cmp edx, 9             ; 大于9是A~F
        jg  .is_A2F
        add edx, '0'
        jmp .store
.is_A2F:
        sub edx, 10
        add edx, 'A'
.store:
        mov  [ebx + edi], dl   ; 存储转换后的字符
        dec  edi
        shr  eax, 4            ; 右移4bits继续计算
        mov  edx, eax
        loop .4bits_hex2char

.ready_to_print:
        inc edi
.skip_prefix_0:
        cmp edi, 8             ; 判断数字高位是否为0，为0不输出
        je  .full0
.go_on_skip:
        mov cl, [put_int_buff + edi]
        inc edi
        cmp cl, '0'
        je .skip_prefix_0      ; 判断数字高位是否为0，为0不输出
        dec edi
        jmp .put_each_num      ; 遇到数字打印数字

.full0:                        ; 整数为0，打印一个0
        mov cl, '0'
.put_each_num:
        push ecx               ; 压入要打印的字符
        call put_char
        add  esp, 4
        inc  edi               ; 循环取数字打印
        mov  cl, [put_int_buff + edi]
        cmp  edi, 8
        jl  .put_each_num
        popad
        ret

