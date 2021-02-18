[bits 32]
section .text
global thread_switch
thread_switch:      ; 任务切换函数，主要保存ABI规定的函数调用参数和切换栈
        push esi
        push edi
        push ebx
        push ebp
        mov  eax, [esp + 20]  ; 当前任务PCB块
        mov [eax], esp        ; 保存当前任务栈

        mov eax, [esp + 24]   ; 取下一个任务PCB
        mov esp, [eax]        ; 切换到下一个任务栈
        pop ebp
        pop ebx
        pop edi
        pop esi
        ret