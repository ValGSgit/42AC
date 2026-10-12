; This program prints its own source code when run.
bits 64
default rel

section .note.GNU-stack noalloc noexec nowrite progbits

section .data
    src db "; This program prints its own source code when run.%1$cbits 64%1$cdefault rel%1$c%1$csection .note.GNU-stack noalloc noexec nowrite progbits%1$c%1$csection .data%1$c    src db %2$c%3$s%2$c, 0%1$c%1$csection .text%1$c    global main%1$c    extern printf%1$c%1$cprint_source:%1$c    push rbp%1$c    mov rbp, rsp%1$c    lea rdi, [src]%1$c    mov esi, 10%1$c    mov edx, 34%1$c    lea rcx, [src]%1$c    xor eax, eax%1$c    call printf wrt ..plt%1$c    pop rbp%1$c    ret%1$c%1$cmain:%1$c    push rbp%1$c    mov rbp, rsp%1$c    ; Call the routine that prints the source code%1$c    call print_source%1$c    xor eax, eax%1$c    pop rbp%1$c    ret%1$c", 0

section .text
    global main
    extern printf

print_source:
    push rbp
    mov rbp, rsp
    lea rdi, [src]
    mov esi, 10
    mov edx, 34
    lea rcx, [src]
    xor eax, eax
    call printf wrt ..plt
    pop rbp
    ret

main:
    push rbp
    mov rbp, rsp
    ; Call the routine that prints the source code
    call print_source
    xor eax, eax
    pop rbp
    ret
