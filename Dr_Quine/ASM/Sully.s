i equ 5
; Writes Sully_X.s, compiles it and runs it while X >= 0
bits 64
default rel

section .note.GNU-stack noalloc noexec nowrite progbits

section .data
    self db __FILE__, 0
    prefix db "Sully_", 0
    namefmt db "Sully_%d.s", 0
    cmdfmt db "nasm -f elf64 Sully_%1$d.s -o Sully_%1$d.o && gcc Sully_%1$d.o -o Sully_%1$d && rm -f Sully_%1$d.o && ./Sully_%1$d", 0
    mode db "w", 0
    src db "i equ %4$d%1$c; Writes Sully_X.s, compiles it and runs it while X >= 0%1$cbits 64%1$cdefault rel%1$c%1$csection .note.GNU-stack noalloc noexec nowrite progbits%1$c%1$csection .data%1$c    self db __FILE__, 0%1$c    prefix db %2$cSully_%2$c, 0%1$c    namefmt db %2$cSully_%%d.s%2$c, 0%1$c    cmdfmt db %2$cnasm -f elf64 Sully_%%1$d.s -o Sully_%%1$d.o && gcc Sully_%%1$d.o -o Sully_%%1$d && rm -f Sully_%%1$d.o && ./Sully_%%1$d%2$c, 0%1$c    mode db %2$cw%2$c, 0%1$c    src db %2$c%3$s%2$c, 0%1$c%1$csection .bss%1$c    name resb 64%1$c    cmd resb 256%1$c%1$csection .text%1$c    global main%1$c    extern strncmp, sprintf, fopen, fprintf, fclose, system%1$c%1$cmain:%1$c    push rbp%1$c    mov rbp, rsp%1$c    push rbx%1$c    push r12%1$c    mov r12d, i%1$c    lea rdi, [self]%1$c    lea rsi, [prefix]%1$c    mov edx, 6%1$c    call strncmp wrt ..plt%1$c    test eax, eax%1$c    jnz .check%1$c    dec r12d%1$c.check:%1$c    cmp r12d, 0%1$c    jl .done%1$c    lea rdi, [name]%1$c    lea rsi, [namefmt]%1$c    mov edx, r12d%1$c    xor eax, eax%1$c    call sprintf wrt ..plt%1$c    lea rdi, [name]%1$c    lea rsi, [mode]%1$c    call fopen wrt ..plt%1$c    test rax, rax%1$c    jz .error%1$c    mov rbx, rax%1$c    mov rdi, rbx%1$c    lea rsi, [src]%1$c    mov edx, 10%1$c    mov ecx, 34%1$c    lea r8, [src]%1$c    mov r9d, r12d%1$c    xor eax, eax%1$c    call fprintf wrt ..plt%1$c    mov rdi, rbx%1$c    call fclose wrt ..plt%1$c    test eax, eax%1$c    jnz .error%1$c    lea rdi, [cmd]%1$c    lea rsi, [cmdfmt]%1$c    mov edx, r12d%1$c    xor eax, eax%1$c    call sprintf wrt ..plt%1$c    lea rdi, [cmd]%1$c    call system wrt ..plt%1$c    test eax, eax%1$c    jnz .error%1$c.done:%1$c    xor eax, eax%1$c    jmp .end%1$c.error:%1$c    mov eax, 1%1$c.end:%1$c    pop r12%1$c    pop rbx%1$c    pop rbp%1$c    ret%1$c", 0

section .bss
    name resb 64
    cmd resb 256

section .text
    global main
    extern strncmp, sprintf, fopen, fprintf, fclose, system

main:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    mov r12d, i
    lea rdi, [self]
    lea rsi, [prefix]
    mov edx, 6
    call strncmp wrt ..plt
    test eax, eax
    jnz .check
    dec r12d
.check:
    cmp r12d, 0
    jl .done
    lea rdi, [name]
    lea rsi, [namefmt]
    mov edx, r12d
    xor eax, eax
    call sprintf wrt ..plt
    lea rdi, [name]
    lea rsi, [mode]
    call fopen wrt ..plt
    test rax, rax
    jz .error
    mov rbx, rax
    mov rdi, rbx
    lea rsi, [src]
    mov edx, 10
    mov ecx, 34
    lea r8, [src]
    mov r9d, r12d
    xor eax, eax
    call fprintf wrt ..plt
    mov rdi, rbx
    call fclose wrt ..plt
    test eax, eax
    jnz .error
    lea rdi, [cmd]
    lea rsi, [cmdfmt]
    mov edx, r12d
    xor eax, eax
    call sprintf wrt ..plt
    lea rdi, [cmd]
    call system wrt ..plt
    test eax, eax
    jnz .error
.done:
    xor eax, eax
    jmp .end
.error:
    mov eax, 1
.end:
    pop r12
    pop rbx
    pop rbp
    ret
