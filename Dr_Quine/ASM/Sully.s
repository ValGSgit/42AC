i equ 5
; Writes Sully_X.s, compiles it and runs it while X >= 0.
; Children (built from Sully_*.s) subtract 1 first, the original does not.

bits 64
default rel                   ; rip-relative addresses (needed for PIE)

section .note.GNU-stack noalloc noexec nowrite progbits ; no executable stack

section .data
    self db __FILE__, 0       ; file we were assembled from
    prefix db "Sully_", 0
    namefmt db "Sully_%d.s", 0
    cmdfmt db "nasm -f elf64 Sully_%1$d.s -o Sully_%1$d.o && gcc Sully_%1$d.o -o Sully_%1$d && rm -f Sully_%1$d.o && ./Sully_%1$d", 0
    mode db "w", 0
    src db "i equ %4$d%1$c; Writes Sully_X.s, compiles it and runs it while X >= 0.%1$c; Children (built from Sully_*.s) subtract 1 first, the original does not.%1$c%1$cbits 64%1$cdefault rel                   ; rip-relative addresses (needed for PIE)%1$c%1$csection .note.GNU-stack noalloc noexec nowrite progbits ; no executable stack%1$c%1$csection .data%1$c    self db __FILE__, 0       ; file we were assembled from%1$c    prefix db %2$cSully_%2$c, 0%1$c    namefmt db %2$cSully_%%d.s%2$c, 0%1$c    cmdfmt db %2$cnasm -f elf64 Sully_%%1$d.s -o Sully_%%1$d.o && gcc Sully_%%1$d.o -o Sully_%%1$d && rm -f Sully_%%1$d.o && ./Sully_%%1$d%2$c, 0%1$c    mode db %2$cw%2$c, 0%1$c    src db %2$c%3$s%2$c, 0%1$c%1$csection .bss%1$c    name resb 64              ; next file name%1$c    cmd resb 256              ; shell command%1$c%1$csection .text%1$c    global main%1$c    extern strncmp, sprintf, fopen, fprintf, fclose, system%1$c%1$cmain:%1$c    push rbp                  ; save frame pointer%1$c    mov rbp, rsp              ; new stack frame%1$c    push rbx                  ; save rbx, will hold the FILE%1$c    push r12                  ; save r12, will hold the counter%1$c    mov r12d, i               ; counter = i%1$c%1$c    lea rdi, [self]           ; are we a child?%1$c    lea rsi, [prefix]%1$c    mov edx, 6%1$c    call strncmp wrt ..plt    ; 0 if self starts with Sully_%1$c    test eax, eax%1$c    jnz .check                ; original: keep counter%1$c    dec r12d                  ; child: counter - 1%1$c.check:%1$c    cmp r12d, 0%1$c    jl .done                  ; below 0: stop%1$c%1$c    lea rdi, [name]           ; name = Sully_X.s%1$c    lea rsi, [namefmt]%1$c    mov edx, r12d%1$c    xor eax, eax              ; no float args%1$c    call sprintf wrt ..plt%1$c%1$c    lea rdi, [name]           ; f = fopen(name, w)%1$c    lea rsi, [mode]%1$c    call fopen wrt ..plt%1$c    test rax, rax%1$c    jz .error                 ; fopen failed%1$c    mov rbx, rax              ; keep the FILE%1$c%1$c    mov rdi, rbx              ; fprintf(f, src, 10, 34, src, counter)%1$c    lea rsi, [src]            ; format = src%1$c    mov edx, 10               ; newline%1$c    mov ecx, 34               ; double quote%1$c    lea r8, [src]             ; src again, prints itself%1$c    mov r9d, r12d             ; counter for line 1%1$c    xor eax, eax              ; no float args%1$c    call fprintf wrt ..plt%1$c%1$c    mov rdi, rbx              ; fclose(f)%1$c    call fclose wrt ..plt%1$c    test eax, eax%1$c    jnz .error                ; fclose failed%1$c%1$c    lea rdi, [cmd]            ; cmd = build and run Sully_X%1$c    lea rsi, [cmdfmt]%1$c    mov edx, r12d%1$c    xor eax, eax              ; no float args%1$c    call sprintf wrt ..plt%1$c    lea rdi, [cmd]            ; system(cmd)%1$c    call system wrt ..plt%1$c    test eax, eax%1$c    jnz .error                ; command failed%1$c.done:%1$c    xor eax, eax              ; return 0%1$c    jmp .end%1$c.error:%1$c    mov eax, 1                ; return 1%1$c.end:%1$c    pop r12                   ; restore saved registers%1$c    pop rbx%1$c    pop rbp%1$c    ret%1$c", 0

section .bss
    name resb 64              ; next file name
    cmd resb 256              ; shell command

section .text
    global main
    extern strncmp, sprintf, fopen, fprintf, fclose, system

main:
    push rbp                  ; save frame pointer
    mov rbp, rsp              ; new stack frame
    push rbx                  ; save rbx, will hold the FILE
    push r12                  ; save r12, will hold the counter
    mov r12d, i               ; counter = i

    lea rdi, [self]           ; are we a child?
    lea rsi, [prefix]
    mov edx, 6
    call strncmp wrt ..plt    ; 0 if self starts with Sully_
    test eax, eax
    jnz .check                ; original: keep counter
    dec r12d                  ; child: counter - 1
.check:
    cmp r12d, 0
    jl .done                  ; below 0: stop

    lea rdi, [name]           ; name = Sully_X.s
    lea rsi, [namefmt]
    mov edx, r12d
    xor eax, eax              ; no float args
    call sprintf wrt ..plt

    lea rdi, [name]           ; f = fopen(name, w)
    lea rsi, [mode]
    call fopen wrt ..plt
    test rax, rax
    jz .error                 ; fopen failed
    mov rbx, rax              ; keep the FILE

    mov rdi, rbx              ; fprintf(f, src, 10, 34, src, counter)
    lea rsi, [src]            ; format = src
    mov edx, 10               ; newline
    mov ecx, 34               ; double quote
    lea r8, [src]             ; src again, prints itself
    mov r9d, r12d             ; counter for line 1
    xor eax, eax              ; no float args
    call fprintf wrt ..plt

    mov rdi, rbx              ; fclose(f)
    call fclose wrt ..plt
    test eax, eax
    jnz .error                ; fclose failed

    lea rdi, [cmd]            ; cmd = build and run Sully_X
    lea rsi, [cmdfmt]
    mov edx, r12d
    xor eax, eax              ; no float args
    call sprintf wrt ..plt
    lea rdi, [cmd]            ; system(cmd)
    call system wrt ..plt
    test eax, eax
    jnz .error                ; command failed
.done:
    xor eax, eax              ; return 0
    jmp .end
.error:
    mov eax, 1                ; return 1
.end:
    pop r12                   ; restore saved registers
    pop rbx
    pop rbp
    ret
