# 01 — x86-64 and NASM basics

Just enough assembly to write libasm. Not a full course: the [references](REFERENCES.md) go deeper.

## What an assembler does

You write mnemonics (`mov rax, 1`), `nasm` turns them into machine-code bytes in an **object file** (`.o`, ELF64 format on Linux). It doesn't make an executable. Several `.o` files get bundled into a static library (`ar`), and then the **linker** (called through `gcc`) combines your library, a C `main`, and libc into a program.

```
ft_strlen.s --nasm-->  ft_strlen.o --ar--> libasm.a --+
                                                     +--gcc (linker)--> ./test
main.c      --gcc -c-> main.o  ------------------------+   (+ libc)
```

The C compiler and your asm never "see" each other. The only thing connecting them is a **symbol name** (`ft_strlen`) and an agreement on how arguments and return values travel: the **calling convention** ([doc 2](02_CALLING_CONVENTION.md)).

## Registers

x86-64 has 16 general-purpose 64-bit registers. Each can also be used through smaller "views" of its low bits:

| 64-bit | low 32 | low 16 | low 8 | Conventional role (System V) |
|--------|--------|--------|-------|------------------------------|
| `rax`  | `eax`  | `ax`   | `al`  | return value, syscall number |
| `rbx`  | `ebx`  | `bx`   | `bl`  | callee-saved |
| `rcx`  | `ecx`  | `cx`   | `cl`  | 4th arg, `rep` counter, shift count |
| `rdx`  | `edx`  | `dx`   | `dl`  | 3rd arg, high half of `mul` |
| `rsi`  | `esi`  | `si`   | `sil` | 2nd arg |
| `rdi`  | `edi`  | `di`   | `dil` | 1st arg |
| `rbp`  | `ebp`  | `bp`   | `bpl` | callee-saved (optional frame pointer) |
| `rsp`  | `esp`  | `sp`   | `spl` | **stack pointer**, don't misuse |
| `r8`   | `r8d`  | `r8w`  | `r8b` | 5th arg |
| `r9`   | `r9d`  | `r9w`  | `r9b` | 6th arg |
| `r10`  | `r10d` | `r10w` | `r10b`| scratch (4th syscall arg) |
| `r11`  | `r11d` | `r11w` | `r11b`| scratch (clobbered by `syscall`) |
| `r12`–`r15` | `r12d`… | `r12w`… | `r12b`… | callee-saved |

Plus `rip` (instruction pointer, you don't write it directly), `rflags` (the flags), and 16 vector registers `xmm0`–`xmm15` (128 bits each) used for the SIMD bonus.

### The zero-extension trap (not obvious, important)

- Writing a **32-bit** register **zeroes the upper 32 bits** of the 64-bit one. `mov eax, -1` leaves `rax = 0x00000000FFFFFFFF`.
- Writing an **8- or 16-bit** register **leaves the rest untouched**. `mov al, 5` keeps whatever garbage was in bits 8–63 of `rax`.

Consequences:
- `xor eax, eax` is the idiomatic way to zero `rax` (shorter than `xor rax, rax`, same effect).
- To load one byte into a full register cleanly, use `movzx eax, byte [rdi]` (zero-extend) or `movsx` (sign-extend). Don't `mov al, [rdi]` and then use `rax`.
- When a C function returns `int`, only `eax` matters. When it returns `size_t` or a pointer, all of `rax` matters.
- When C passes you an `int` (e.g. `fd` in `write`), only `edi` is meaningful. The upper half of `rdi` is **not guaranteed** to be zero or sign-extended.

## NASM syntax

```nasm
default rel                 ; make [label] RIP-relative (needed for PIE, see doc 2)

section .data               ; initialized, writable data
counter:    dq 0            ; dq = 8 bytes, dd = 4, dw = 2, db = 1

section .rodata             ; read-only data
msg:        db "hello", 10, 0   ; bytes: 'h' 'e' 'l' 'l' 'o' '\n' '\0'

section .bss                ; zero-initialized, writable, takes no space in the file
buffer:     resb 64         ; reserve 64 bytes

section .text               ; code
global  my_func             ; export the symbol so the linker (and C) can see it
extern  malloc              ; declare a symbol defined elsewhere

my_func:                    ; a label is just a name for an address
    xor     eax, eax
.loop:                      ; a label starting with '.' is local to the previous non-local label
    ...
    ret
```

- **Intel syntax**: destination first. `mov rax, rbx` means `rax = rbx`. (AT&T, the GNU default, is the reverse, with `%` and `$` everywhere: `movq %rbx, %rax`. The subject forbids it.)
- `;` starts a comment.
- `[...]` means "memory at this address". `mov rax, rdi` copies the pointer; `mov rax, [rdi]` loads 8 bytes from where it points.
- When the size isn't obvious from a register, you must say it: `mov byte [rdi], 0`, `cmp byte [rsi], 0`, `mov dword [rax], 12`. Sizes: `byte` 1, `word` 2, `dword` 4, `qword` 8, `oword`/`dqword` 16.
- Local labels (`.loop`, `.done`) let every file reuse the same names. Use them.

### Addressing modes

The general form is `[base + index*scale + displacement]`, with scale 1, 2, 4 or 8:

```nasm
mov al, [rdi]              ; *rdi
mov al, [rdi + 3]          ; rdi[3]
mov al, [rdi + rcx]        ; rdi[rcx]           (char array)
mov eax, [rdi + rcx*4]     ; ((int *)rdi)[rcx]  (int array)
mov rax, [rdi + 8]         ; ((t_list *)rdi)->next  (data is at +0, next at +8)
lea rax, [rdi + rcx]       ; rax = rdi + rcx, NO memory access
```

`lea` ("load effective address") computes the address and stops there. It's also handy as a 3-operand add: `lea rax, [rdi + rsi*2 + 1]`.

## Instructions you'll actually use

| Instruction | Meaning | Notes |
|-------------|---------|-------|
| `mov d, s` | `d = s` | Can't do memory→memory. |
| `movzx d, s` / `movsx d, s` | load small value, zero/sign-extend | `movzx eax, byte [rdi]` |
| `lea d, [addr]` | `d = address` | No memory access, no flags. |
| `add`, `sub` | `d += s`, `d -= s` | Set flags. |
| `inc`, `dec` | `d++`, `d--` | Set flags (except CF). |
| `neg d` | `d = -d` | Useful for syscall errors. |
| `and`, `or`, `xor`, `not` | bitwise | `xor eax, eax` zeroes. |
| `shl`, `shr`, `sar` | shifts | `sar` keeps the sign. |
| `imul d, s` | signed multiply, truncated | Good for `result = result * base`. |
| `mul s` | **unsigned** `rdx:rax = rax * s` | Sets CF/OF if the result didn't fit in `rax`. That's your calloc overflow check. |
| `cmp a, b` | computes `a - b`, sets flags, discards result | |
| `test a, b` | computes `a & b`, sets flags, discards result | `test rax, rax` = "is rax zero / negative?" |
| `jmp label` | unconditional jump | |
| `jcc label` | conditional jump | See below. |
| `cmovcc d, s` | conditional move | Branchless `if`. |
| `push r` / `pop r` | `rsp -= 8; [rsp] = r` / reverse | |
| `call f` | push return address, jump to `f` | Moves `rsp` by 8! |
| `ret` | pop return address, jump to it | |
| `syscall` | enter the kernel | [Doc 3](03_SYSCALLS_AND_ERRNO.md). |
| `rep stosb` | `memset(rdi, al, rcx)` | Advances `rdi`, decrements `rcx` to 0. |
| `rep movsb` | `memcpy(rdi, rsi, rcx)` | Same idea. |
| `bsf d, s` / `tzcnt d, s` | index of lowest set bit | For the SIMD bonus. |

## Flags and conditional jumps

Arithmetic and `cmp`/`test` set flags in `rflags`. The ones you'll care about:

- **ZF** (zero): result was 0.
- **SF** (sign): result's top bit was 1.
- **CF** (carry): unsigned overflow / borrow.
- **OF** (overflow): signed overflow.

After `cmp a, b`:

| Condition | Unsigned | Signed |
|-----------|----------|--------|
| `a == b` | `je` / `jz` | `je` / `jz` |
| `a != b` | `jne` / `jnz` | `jne` / `jnz` |
| `a < b`  | `jb` (below) | `jl` (less) |
| `a <= b` | `jbe` | `jle` |
| `a > b`  | `ja` (above) | `jg` (greater) |
| `a >= b` | `jae` | `jge` |

Also: `js` (sign set, i.e. negative), `jc`/`jnc` (carry), `jo` (overflow).

**Not obvious:** choosing between `jb` and `jl` is how *you* decide if numbers are signed or unsigned. The CPU doesn't know. Comparing bytes for `strcmp`? Must be **unsigned** (the man page says the comparison is done as `unsigned char`).

## A loop, end to end

C:

```c
size_t count_a(const char *s)
{
    size_t n = 0;
    while (*s) { if (*s == 'a') n++; s++; }
    return n;
}
```

NASM:

```nasm
global count_a
section .text
count_a:                        ; rdi = s
    xor     eax, eax            ; n = 0 (rax is the return value)
.loop:
    movzx   ecx, byte [rdi]     ; c = *s
    test    ecx, ecx
    jz      .done               ; if c == 0, stop
    cmp     ecx, 'a'
    jne     .next
    inc     rax                 ; n++
.next:
    inc     rdi                 ; s++
    jmp     .loop
.done:
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
```

The last line is explained in [doc 2](02_CALLING_CONVENTION.md#the-gnu-stack-note). This function is deliberately *not* one of the libasm functions; it's the shape you'll reuse.

## Exercises before moving on

Write these in a scratch folder (not the project), link them with a tiny `main.c`, and check them in gdb:

1. `long ft_add(long a, long b)` — one instruction plus `ret`.
2. `int ft_max(int a, int b)` — with a jump, then again with `cmov`. Careful: `int`, so 32-bit registers and signed comparison.
3. `long ft_sum(const int *arr, size_t n)` — scaled addressing, `movsxd` to widen each `int` to 64 bits.
4. `void ft_bzero(void *p, size_t n)` — once with a loop, once with `rep stosb`.

Build and run one:

```sh
nasm -f elf64 add.s -o add.o
gcc -Wall -Wextra -Werror main.c add.o -o t && ./t
```

## Checklist

- [ ] I know what `mov eax, …` does to the upper half of `rax`, and why I'd use `movzx`.
- [ ] I can read `[rdi + rcx*8 + 8]` and know the difference between `lea` and `mov` with it.
- [ ] I know when to use `jb` vs `jl`.
- [ ] I know what `global`, `extern`, `section` and local `.labels` do.
