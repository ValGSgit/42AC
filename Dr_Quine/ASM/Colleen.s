; This program prints its own source code when run.
; src holds the whole file. printf fills in newlines (10), quotes (34) and src itself.

bits 64
default rel                   ; rip-relative addresses (needed for PIE)

section .note.GNU-stack noalloc noexec nowrite progbits ; no executable stack

section .data
    src db "; This program prints its own source code when run.%1$c; src holds the whole file. printf fills in newlines (10), quotes (34) and src itself.%1$c%1$cbits 64%1$cdefault rel                   ; rip-relative addresses (needed for PIE)%1$c%1$csection .note.GNU-stack noalloc noexec nowrite progbits ; no executable stack%1$c%1$csection .data%1$c    src db %2$c%3$s%2$c, 0%1$c%1$csection .text%1$c    global main%1$c    extern printf%1$c%1$cprint_source:%1$c    push rbp                  ; save frame pointer%1$c    mov rbp, rsp              ; new stack frame%1$c    lea rdi, [src]            ; format = src%1$c    mov esi, 10               ; newline%1$c    mov edx, 34               ; double quote%1$c    lea rcx, [src]            ; src again, prints itself%1$c    xor eax, eax              ; no float args%1$c    call printf wrt ..plt     ; printf(src, 10, 34, src)%1$c    pop rbp                   ; restore frame pointer%1$c    ret%1$c%1$cmain:%1$c    push rbp                  ; save frame pointer%1$c    mov rbp, rsp              ; new stack frame%1$c    ; Call the routine that prints the source code%1$c    call print_source%1$c    xor eax, eax              ; return 0%1$c    pop rbp                   ; restore frame pointer%1$c    ret%1$c", 0

section .text
    global main
    extern printf

print_source:
    push rbp                  ; save frame pointer
    mov rbp, rsp              ; new stack frame
    lea rdi, [src]            ; format = src
    mov esi, 10               ; newline
    mov edx, 34               ; double quote
    lea rcx, [src]            ; src again, prints itself
    xor eax, eax              ; no float args
    call printf wrt ..plt     ; printf(src, 10, 34, src)
    pop rbp                   ; restore frame pointer
    ret

main:
    push rbp                  ; save frame pointer
    mov rbp, rsp              ; new stack frame
    ; Call the routine that prints the source code
    call print_source
    xor eax, eax              ; return 0
    pop rbp                   ; restore frame pointer
    ret
