; This program writes its own source code into Grace_kid.s
%define KID "Grace_kid.s"
%define SRC "; This program writes its own source code into Grace_kid.s%1$c%%define KID %2$cGrace_kid.s%2$c%1$c%%define SRC %2$c%3$s%2$c%1$c%%macro FT 0%1$cbits 64%1$cdefault rel%1$csection .note.GNU-stack noalloc noexec nowrite progbits%1$csection .data%1$c    kid db KID, 0%1$c    mode db %2$cw%2$c, 0%1$c    src db SRC, 0%1$csection .text%1$c    global main%1$c    extern fopen, fprintf, fclose%1$cmain:%1$c    push rbp%1$c    mov rbp, rsp%1$c    push rbx%1$c    sub rsp, 8%1$c    lea rdi, [kid]%1$c    lea rsi, [mode]%1$c    call fopen wrt ..plt%1$c    test rax, rax%1$c    jz .error%1$c    mov rbx, rax%1$c    mov rdi, rbx%1$c    lea rsi, [src]%1$c    mov edx, 10%1$c    mov ecx, 34%1$c    lea r8, [src]%1$c    xor eax, eax%1$c    call fprintf wrt ..plt%1$c    mov rdi, rbx%1$c    call fclose wrt ..plt%1$c    test eax, eax%1$c    jnz .error%1$c    xor eax, eax%1$c    jmp .end%1$c.error:%1$c    mov eax, 1%1$c.end:%1$c    add rsp, 8%1$c    pop rbx%1$c    pop rbp%1$c    ret%1$c%%endmacro%1$cFT%1$c"
%macro FT 0
bits 64
default rel
section .note.GNU-stack noalloc noexec nowrite progbits
section .data
    kid db KID, 0
    mode db "w", 0
    src db SRC, 0
section .text
    global main
    extern fopen, fprintf, fclose
main:
    push rbp
    mov rbp, rsp
    push rbx
    sub rsp, 8
    lea rdi, [kid]
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
    xor eax, eax
    call fprintf wrt ..plt
    mov rdi, rbx
    call fclose wrt ..plt
    test eax, eax
    jnz .error
    xor eax, eax
    jmp .end
.error:
    mov eax, 1
.end:
    add rsp, 8
    pop rbx
    pop rbp
    ret
%endmacro
FT
