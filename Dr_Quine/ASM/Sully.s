i equ 5
; Writes Sully_X.s, compiles it and runs it while X >= 0
;
; The counter i lives on line 1. The original program writes Sully_5.s with
; i = 5. Every child (whose file name starts with Sully_) first subtracts 1,
; so Sully_5 writes Sully_4, and so on. Sully_0 would need -1, so it stops.

bits 64                                 ; generate 64-bit code
default rel                             ; [label] means rip-relative, needed for PIE executables

section .note.GNU-stack noalloc noexec nowrite progbits ; tell the linker we do not need an executable stack

section .data                           ; initialized data
    self db __FILE__, 0                 ; name of the file this was assembled from, filled in by nasm
    prefix db "Sully_", 0               ; children are assembled from files starting with this
    namefmt db "Sully_%d.s", 0          ; sprintf format for the name of the next file
    cmdfmt db "nasm -f elf64 Sully_%1$d.s -o Sully_%1$d.o && gcc Sully_%1$d.o -o Sully_%1$d && rm -f Sully_%1$d.o && ./Sully_%1$d", 0
    mode db "w", 0                      ; fopen mode: write, truncate if it exists
    src db "i equ %4$d%1$c; Writes Sully_X.s, compiles it and runs it while X >= 0%1$c;%1$c; The counter i lives on line 1. The original program writes Sully_5.s with%1$c; i = 5. Every child (whose file name starts with Sully_) first subtracts 1,%1$c; so Sully_5 writes Sully_4, and so on. Sully_0 would need -1, so it stops.%1$c%1$cbits 64                                 ; generate 64-bit code%1$cdefault rel                             ; [label] means rip-relative, needed for PIE executables%1$c%1$csection .note.GNU-stack noalloc noexec nowrite progbits ; tell the linker we do not need an executable stack%1$c%1$csection .data                           ; initialized data%1$c    self db __FILE__, 0                 ; name of the file this was assembled from, filled in by nasm%1$c    prefix db %2$cSully_%2$c, 0               ; children are assembled from files starting with this%1$c    namefmt db %2$cSully_%%d.s%2$c, 0          ; sprintf format for the name of the next file%1$c    cmdfmt db %2$cnasm -f elf64 Sully_%%1$d.s -o Sully_%%1$d.o && gcc Sully_%%1$d.o -o Sully_%%1$d && rm -f Sully_%%1$d.o && ./Sully_%%1$d%2$c, 0%1$c    mode db %2$cw%2$c, 0                      ; fopen mode: write, truncate if it exists%1$c    src db %2$c%3$s%2$c, 0%1$c%1$csection .bss                            ; uninitialized data, zeroed at startup%1$c    name resb 64                        ; buffer for the next file name%1$c    cmd resb 256                        ; buffer for the shell command%1$c%1$csection .text                           ; code%1$c    global main                         ; export main so the C runtime can call it%1$c    extern strncmp, sprintf, fopen, fprintf, fclose, system ; all from libc%1$c%1$cmain:%1$c    push rbp                            ; save the frame pointer%1$c    mov rbp, rsp                        ; start main's stack frame%1$c    push rbx                            ; rbx and r12 are callee-saved: save them before using them%1$c    push r12                            ; after 3 pushes rsp is 16-byte aligned again%1$c    mov r12d, i                         ; r12 = the counter, it survives the libc calls below%1$c%1$c    ; Are we a child? strncmp(self, prefix, 6) returns 0 if self starts with Sully_%1$c    lea rdi, [self]                     ; arg 1: our own source file name%1$c    lea rsi, [prefix]                   ; arg 2: Sully_%1$c    mov edx, 6                          ; arg 3: compare only the first 6 characters%1$c    call strncmp wrt ..plt              ; eax = 0 if they match%1$c    test eax, eax                       ; set the flags from eax%1$c    jnz .check                          ; not 0: we are the original, keep the counter as it is%1$c    dec r12d                            ; we are a child: counter = counter - 1%1$c.check:%1$c    cmp r12d, 0                         ; compare the counter with 0%1$c    jl .done                            ; below 0: nothing more to create, stop here%1$c%1$c    ; name = Sully_X.s%1$c    lea rdi, [name]                     ; arg 1: destination buffer%1$c    lea rsi, [namefmt]                  ; arg 2: format%1$c    mov edx, r12d                       ; arg 3: the counter%1$c    xor eax, eax                        ; variadic call: no vector registers used%1$c    call sprintf wrt ..plt              ; write the file name into name%1$c%1$c    ; f = fopen(name, w)%1$c    lea rdi, [name]                     ; arg 1: file name%1$c    lea rsi, [mode]                     ; arg 2: mode w%1$c    call fopen wrt ..plt                ; rax = FILE pointer, or 0 on error%1$c    test rax, rax                       ; did fopen fail?%1$c    jz .error                           ; yes: return 1%1$c    mov rbx, rax                        ; keep the FILE pointer in rbx, which libc will not overwrite%1$c%1$c    ; fprintf(f, src, 10, 34, src, counter) writes our own source with the new counter%1$c    mov rdi, rbx                        ; arg 1: the file%1$c    lea rsi, [src]                      ; arg 2: the format string%1$c    mov edx, 10                         ; arg 3: newline%1$c    mov ecx, 34                         ; arg 4: double quote%1$c    lea r8, [src]                       ; arg 5: the string again, printed in place of itself%1$c    mov r9d, r12d                       ; arg 6: the counter, printed on line 1%1$c    xor eax, eax                        ; variadic call: no vector registers used%1$c    call fprintf wrt ..plt              ; write the file%1$c%1$c    mov rdi, rbx                        ; arg 1: the file%1$c    call fclose wrt ..plt               ; flush and close it, eax = 0 on success%1$c    test eax, eax                       ; did fclose fail?%1$c    jnz .error                          ; yes: return 1%1$c%1$c    ; cmd = assemble, link, remove the .o and run Sully_X%1$c    lea rdi, [cmd]                      ; arg 1: destination buffer%1$c    lea rsi, [cmdfmt]                   ; arg 2: format%1$c    mov edx, r12d                       ; arg 3: the counter%1$c    xor eax, eax                        ; variadic call: no vector registers used%1$c    call sprintf wrt ..plt              ; build the command%1$c    lea rdi, [cmd]                      ; arg 1: the command%1$c    call system wrt ..plt               ; run it in /bin/sh, eax = its exit status%1$c    test eax, eax                       ; did something in the chain fail?%1$c    jnz .error                          ; yes: return 1%1$c.done:%1$c    xor eax, eax                        ; return value 0%1$c    jmp .end                            ; skip the error path%1$c.error:%1$c    mov eax, 1                          ; return value 1%1$c.end:%1$c    pop r12                             ; restore the callee-saved registers in reverse order%1$c    pop rbx%1$c    pop rbp                             ; restore the frame pointer%1$c    ret                                 ; return to the C runtime%1$c", 0

section .bss                            ; uninitialized data, zeroed at startup
    name resb 64                        ; buffer for the next file name
    cmd resb 256                        ; buffer for the shell command

section .text                           ; code
    global main                         ; export main so the C runtime can call it
    extern strncmp, sprintf, fopen, fprintf, fclose, system ; all from libc

main:
    push rbp                            ; save the frame pointer
    mov rbp, rsp                        ; start main's stack frame
    push rbx                            ; rbx and r12 are callee-saved: save them before using them
    push r12                            ; after 3 pushes rsp is 16-byte aligned again
    mov r12d, i                         ; r12 = the counter, it survives the libc calls below

    ; Are we a child? strncmp(self, prefix, 6) returns 0 if self starts with Sully_
    lea rdi, [self]                     ; arg 1: our own source file name
    lea rsi, [prefix]                   ; arg 2: Sully_
    mov edx, 6                          ; arg 3: compare only the first 6 characters
    call strncmp wrt ..plt              ; eax = 0 if they match
    test eax, eax                       ; set the flags from eax
    jnz .check                          ; not 0: we are the original, keep the counter as it is
    dec r12d                            ; we are a child: counter = counter - 1
.check:
    cmp r12d, 0                         ; compare the counter with 0
    jl .done                            ; below 0: nothing more to create, stop here

    ; name = Sully_X.s
    lea rdi, [name]                     ; arg 1: destination buffer
    lea rsi, [namefmt]                  ; arg 2: format
    mov edx, r12d                       ; arg 3: the counter
    xor eax, eax                        ; variadic call: no vector registers used
    call sprintf wrt ..plt              ; write the file name into name

    ; f = fopen(name, w)
    lea rdi, [name]                     ; arg 1: file name
    lea rsi, [mode]                     ; arg 2: mode w
    call fopen wrt ..plt                ; rax = FILE pointer, or 0 on error
    test rax, rax                       ; did fopen fail?
    jz .error                           ; yes: return 1
    mov rbx, rax                        ; keep the FILE pointer in rbx, which libc will not overwrite

    ; fprintf(f, src, 10, 34, src, counter) writes our own source with the new counter
    mov rdi, rbx                        ; arg 1: the file
    lea rsi, [src]                      ; arg 2: the format string
    mov edx, 10                         ; arg 3: newline
    mov ecx, 34                         ; arg 4: double quote
    lea r8, [src]                       ; arg 5: the string again, printed in place of itself
    mov r9d, r12d                       ; arg 6: the counter, printed on line 1
    xor eax, eax                        ; variadic call: no vector registers used
    call fprintf wrt ..plt              ; write the file

    mov rdi, rbx                        ; arg 1: the file
    call fclose wrt ..plt               ; flush and close it, eax = 0 on success
    test eax, eax                       ; did fclose fail?
    jnz .error                          ; yes: return 1

    ; cmd = assemble, link, remove the .o and run Sully_X
    lea rdi, [cmd]                      ; arg 1: destination buffer
    lea rsi, [cmdfmt]                   ; arg 2: format
    mov edx, r12d                       ; arg 3: the counter
    xor eax, eax                        ; variadic call: no vector registers used
    call sprintf wrt ..plt              ; build the command
    lea rdi, [cmd]                      ; arg 1: the command
    call system wrt ..plt               ; run it in /bin/sh, eax = its exit status
    test eax, eax                       ; did something in the chain fail?
    jnz .error                          ; yes: return 1
.done:
    xor eax, eax                        ; return value 0
    jmp .end                            ; skip the error path
.error:
    mov eax, 1                          ; return value 1
.end:
    pop r12                             ; restore the callee-saved registers in reverse order
    pop rbx
    pop rbp                             ; restore the frame pointer
    ret                                 ; return to the C runtime
