; This program prints its own source code when run.
;
; How it works: src below is a printf format string that contains the whole
; file, except that newlines, double quotes and the string itself are replaced
; by numbered placeholders. printf fills them in with 10 (newline), 34 (quote)
; and src itself, which rebuilds this exact file on stdout.

bits 64                                 ; generate 64-bit code
default rel                             ; [label] means rip-relative, needed for PIE executables

section .note.GNU-stack noalloc noexec nowrite progbits ; tell the linker we do not need an executable stack

section .data                           ; initialized data
    src db "; This program prints its own source code when run.%1$c;%1$c; How it works: src below is a printf format string that contains the whole%1$c; file, except that newlines, double quotes and the string itself are replaced%1$c; by numbered placeholders. printf fills them in with 10 (newline), 34 (quote)%1$c; and src itself, which rebuilds this exact file on stdout.%1$c%1$cbits 64                                 ; generate 64-bit code%1$cdefault rel                             ; [label] means rip-relative, needed for PIE executables%1$c%1$csection .note.GNU-stack noalloc noexec nowrite progbits ; tell the linker we do not need an executable stack%1$c%1$csection .data                           ; initialized data%1$c    src db %2$c%3$s%2$c, 0%1$c%1$csection .text                           ; code%1$c    global main                         ; export main so the C runtime can call it%1$c    extern printf                       ; printf comes from libc%1$c%1$c; print_source: calls printf(src, 10, 34, src)%1$cprint_source:%1$c    push rbp                            ; save the caller's frame pointer, this also realigns rsp to 16 bytes%1$c    mov rbp, rsp                        ; start our own stack frame%1$c    lea rdi, [src]                      ; arg 1: the format string%1$c    mov esi, 10                         ; arg 2: 10 is the ASCII code of a newline%1$c    mov edx, 34                         ; arg 3: 34 is the ASCII code of a double quote%1$c    lea rcx, [src]                      ; arg 4: the string again, printed in place of itself%1$c    xor eax, eax                        ; variadic call: al = number of vector registers used = 0%1$c    call printf wrt ..plt               ; call printf through the PLT (works with PIE)%1$c    pop rbp                             ; restore the caller's frame pointer%1$c    ret                                 ; go back to main%1$c%1$cmain:%1$c    push rbp                            ; save the frame pointer, rsp is now 16-byte aligned%1$c    mov rbp, rsp                        ; start main's stack frame%1$c    ; Call the routine that prints the source code%1$c    call print_source                   ; print the whole file%1$c    xor eax, eax                        ; return value 0 means success%1$c    pop rbp                             ; restore the frame pointer%1$c    ret                                 ; return to the C runtime, which flushes stdout and exits%1$c", 0

section .text                           ; code
    global main                         ; export main so the C runtime can call it
    extern printf                       ; printf comes from libc

; print_source: calls printf(src, 10, 34, src)
print_source:
    push rbp                            ; save the caller's frame pointer, this also realigns rsp to 16 bytes
    mov rbp, rsp                        ; start our own stack frame
    lea rdi, [src]                      ; arg 1: the format string
    mov esi, 10                         ; arg 2: 10 is the ASCII code of a newline
    mov edx, 34                         ; arg 3: 34 is the ASCII code of a double quote
    lea rcx, [src]                      ; arg 4: the string again, printed in place of itself
    xor eax, eax                        ; variadic call: al = number of vector registers used = 0
    call printf wrt ..plt               ; call printf through the PLT (works with PIE)
    pop rbp                             ; restore the caller's frame pointer
    ret                                 ; go back to main

main:
    push rbp                            ; save the frame pointer, rsp is now 16-byte aligned
    mov rbp, rsp                        ; start main's stack frame
    ; Call the routine that prints the source code
    call print_source                   ; print the whole file
    xor eax, eax                        ; return value 0 means success
    pop rbp                             ; restore the frame pointer
    ret                                 ; return to the C runtime, which flushes stdout and exits
