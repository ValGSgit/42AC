# Grace.s, commented

The subject allows `Grace.s` only one comment, so here's the same program with comments, for reading only. The template string is left out.

```nasm
; This program writes its own source code into Grace_kid.s

%define KID "Grace_kid.s"     ; macro 1: output file
%define SRC "..."             ; macro 2: the template
%macro FT 0                   ; macro 3: the whole program
bits 64
default rel                   ; rip-relative addresses (needed for PIE)
section .note.GNU-stack noalloc noexec nowrite progbits ; no executable stack
section .data
    kid db KID, 0
    mode db "w", 0
    src db SRC, 0
section .text
    global main
    extern fopen, fprintf, fclose
main:
    push rbp                  ; save frame pointer
    mov rbp, rsp              ; new stack frame
    push rbx                  ; save rbx, will hold the FILE
    sub rsp, 8                ; keep the stack 16-byte aligned
    lea rdi, [kid]            ; f = fopen(KID, w)
    lea rsi, [mode]
    call fopen wrt ..plt
    test rax, rax
    jz .error                 ; fopen failed
    mov rbx, rax              ; keep the FILE
    mov rdi, rbx              ; fprintf(f, src, 10, 34, src)
    lea rsi, [src]            ; format = src
    mov edx, 10               ; newline
    mov ecx, 34               ; double quote
    lea r8, [src]             ; src again, prints itself
    xor eax, eax              ; no float args
    call fprintf wrt ..plt
    mov rdi, rbx              ; fclose(f)
    call fclose wrt ..plt
    test eax, eax
    jnz .error                ; fclose failed
    xor eax, eax              ; return 0
    jmp .end
.error:
    mov eax, 1                ; return 1
.end:
    add rsp, 8                ; undo the alignment padding
    pop rbx                   ; restore saved registers
    pop rbp
    ret
%endmacro

FT                            ; expands into the whole program
```

The `%define`s and the `%macro` produce no code on their own. The program only appears when the last line, `FT`, expands the macro.
