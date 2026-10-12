# Grace.s, line by line

The subject says Grace must contain **exactly one comment**, so I couldn't comment every instruction in `ASM/Grace.s` like I did in Colleen and Sully. This is the same program with every line explained, for reading only. Don't assemble this version: the extra comments would fail the evaluation, and the string is left out to keep things readable.

```nasm
; This program writes its own source code into Grace_kid.s    <- the one allowed comment

%define KID "Grace_kid.s"               ; macro 1: name of the file we write
%define SRC "...the template..."        ; macro 2: the whole file as a printf format string
%macro FT 0                             ; macro 3: everything up to %endmacro is the actual program
bits 64                                 ; generate 64-bit code
default rel                             ; [label] means rip-relative, needed for PIE executables
section .note.GNU-stack noalloc noexec nowrite progbits ; tell the linker we don't need an executable stack
section .data                           ; initialized data
    kid db KID, 0                       ; "Grace_kid.s" followed by a 0 byte (C string)
    mode db "w", 0                      ; fopen mode: write, truncate if it exists
    src db SRC, 0                       ; the template as a C string
section .text                           ; code
    global main                         ; export main so the C runtime can call it
    extern fopen, fprintf, fclose       ; these come from libc
main:
    push rbp                            ; save the caller's frame pointer
    mov rbp, rsp                        ; start our own stack frame
    push rbx                            ; rbx is callee-saved: save it before we use it
    sub rsp, 8                          ; 3 x 8 bytes pushed so far, so this brings rsp back to 16-byte alignment

    lea rdi, [kid]                      ; arg 1: file name
    lea rsi, [mode]                     ; arg 2: "w"
    call fopen wrt ..plt                ; rax = FILE pointer, or 0 if it failed
    test rax, rax                       ; is it 0?
    jz .error                           ; yes: return 1 instead of crashing
    mov rbx, rax                        ; keep the FILE pointer in rbx, libc calls won't overwrite it

    mov rdi, rbx                        ; arg 1: the file
    lea rsi, [src]                      ; arg 2: the format string
    mov edx, 10                         ; arg 3: 10 = newline, used by every %1$c
    mov ecx, 34                         ; arg 4: 34 = double quote, used by every %2$c
    lea r8, [src]                       ; arg 5: the string again, printed by %3$s in place of itself
    xor eax, eax                        ; variadic call: al = number of vector registers used = 0
    call fprintf wrt ..plt              ; write the whole file into Grace_kid.s

    mov rdi, rbx                        ; arg 1: the file
    call fclose wrt ..plt               ; flush and close, eax = 0 on success
    test eax, eax                       ; did it fail?
    jnz .error                          ; yes: return 1
    xor eax, eax                        ; return value 0
    jmp .end                            ; skip the error path
.error:
    mov eax, 1                          ; return value 1
.end:
    add rsp, 8                          ; undo the alignment padding
    pop rbx                             ; restore rbx
    pop rbp                             ; restore the frame pointer
    ret                                 ; back to the C runtime
%endmacro                               ; end of macro 3

FT                                      ; "call" the macro: this one word expands into the whole program above
```

## Two things that trip people up

**Why does nothing happen until the last line?** `%define` and `%macro` only *define* things. They produce no code. The assembler only emits sections, data and `main` when it reaches `FT` on the last line and pastes the macro body in. That's how the program "runs by calling a macro" without any routine besides `main`.

**Why `sub rsp, 8`?** When `main` starts, `rsp` is 8 bytes off a 16-byte boundary, because the `call` that got us here pushed a return address. The ABI wants it aligned at every `call` we make. `push rbp` and `push rbx` move it another 16 bytes, which leaves it still 8 off, so we drop it 8 more. Sully avoids this by pushing a third register (`r12`) instead.
