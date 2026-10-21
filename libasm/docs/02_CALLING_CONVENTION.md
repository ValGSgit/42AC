# 02 — The calling convention (System V AMD64 ABI)

The subject says "beware of the calling convention". This is what it means. The C compiler that builds `main.c` assumes your functions follow these rules. If you break one, nothing warns you: the program just behaves strangely, sometimes far away from the bug, sometimes only on someone else's machine.

Linux and macOS both use the **System V AMD64 ABI** (Windows uses a different one, irrelevant here). Spec: see [REFERENCES.md](REFERENCES.md#abi).

## Arguments and return value

Integer and pointer arguments go in registers, in this order:

| Argument | 1st | 2nd | 3rd | 4th | 5th | 6th | 7th+ |
|----------|-----|-----|-----|-----|-----|-----|------|
| Register | `rdi` | `rsi` | `rdx` | `rcx` | `r8` | `r9` | on the stack |

None of the libasm functions take more than four.

- Return value: `rax` (or `eax` for `int` / `unsigned int`).
- Float/vector arguments use `xmm0`–`xmm7`. Not needed here.

Mnemonic for the order: "**D**iane's **S**ilk **D**ress **C**osts **$89**" → di, si, dx, cx, 8, 9.

Example: `ft_strcpy(char *dst, const char *src)` gets `dst` in `rdi`, `src` in `rsi`, and must return `dst` in `rax`.

## Who saves which register

When function A calls function B, which registers can B destroy?

| Callee-saved ("non-volatile") | Caller-saved ("volatile", "scratch") |
|-------------------------------|--------------------------------------|
| `rbx`, `rbp`, `r12`, `r13`, `r14`, `r15`, `rsp` | `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`, `r9`, `r10`, `r11`, all `xmm` |

This cuts both ways:

1. **As a callee** (every function you write): you may freely trash scratch registers. If you want to use `rbx`/`rbp`/`r12`–`r15`, you must `push` them at the start and `pop` them (in reverse order) before `ret`, so the caller gets them back intact.
2. **As a caller** (in `ft_strdup`, `ft_calloc`, the list functions): after `call malloc`, assume `rdi`, `rsi`, `rdx`, `rcx`, `r8`–`r11` contain garbage. If you need a value after the call, keep it in a callee-saved register (which `malloc` promises to preserve) or on the stack.

The classic bug: `ft_strdup` keeps `src` in `rsi`, calls `malloc`, then copies from `rsi`. Works sometimes. It's undefined.

Simple leaf functions (`ft_strlen`, `ft_strcpy`, `ft_strcmp`) never need to touch callee-saved registers: there are plenty of scratch ones.

## The stack and its 16-byte alignment

The stack grows **down**. `push x` does `rsp -= 8; [rsp] = x`. `call f` pushes the 8-byte return address.

**The rule:** right before a `call` instruction executes, `rsp` must be a multiple of 16.

Consequence: when your function starts executing (just after the caller's `call` pushed 8 bytes), `rsp` is **16n + 8**. So if your function calls something else, you must move `rsp` by **an odd number of 8-byte slots** first (8, 24, 40, … bytes) to get back to a multiple of 16.

```
caller, before call:       rsp = ...0   (aligned)
your function, at entry:   rsp = ...8   (return address pushed)
after 1 push:              rsp = ...0   aligned -> OK to call
after 2 pushes:            rsp = ...8   misaligned -> need sub rsp, 8 too
```

Ways to fix it:

```nasm
; A) you need one saved register anyway
my_func:
    push    rbx             ; saves rbx AND realigns the stack
    ...
    call    malloc wrt ..plt
    ...
    pop     rbx
    ret

; B) nothing to save
my_func:
    sub     rsp, 8          ; just realign
    call    something wrt ..plt
    add     rsp, 8          ; undo before ret, or ret jumps to garbage
    ret

; C) the classic frame pointer: push rbp is the one odd push
my_func:
    push    rbp
    mov     rbp, rsp
    ...
    leave                   ; = mov rsp, rbp; pop rbp
    ret
```

**Why it matters (not obvious):** most of the time a misaligned call works. Then glibc's `malloc`, `printf`, or anything compiled with SSE instructions like `movaps` hits a 16-byte-aligned memory operation on your misaligned stack and segfaults deep inside libc, with a backtrace that points nowhere near your code. If you crash inside libc, check alignment first.

**Count your pushes.** Every `push` must have a matching `pop` on every path to `ret`, including early error returns. Unbalanced stack = `ret` pops the wrong value = jump to a random address.

### The red zone

Leaf functions (functions that call nothing) may use the 128 bytes *below* `rsp` without moving `rsp`. You won't need it, but it's why you might see compilers do that.

## Other things the ABI guarantees

- **Direction flag (DF) is clear** on function entry and must be clear on return. So `rep stosb`/`rep movsb` go forward without needing `cld`. Don't `std` without `cld`ing back.
- **Structs** are laid out like in C: `t_list` is `data` at offset 0, `next` at offset 8, total size 16.
- **`int` args** only define the low 32 bits of the register. Don't assume `rdi`'s upper half is clean when C passes an `int fd`.

## PIE: why `call malloc` doesn't link

Modern gcc builds **position-independent executables** (PIE) by default, and the subject forbids `-no-pie`. A PIE can be loaded at any address, so the code can't contain hard-coded absolute addresses. Two consequences for NASM:

### 1. Calls to shared-library functions need `wrt ..plt`

`malloc`, `free` and `__errno_location` live in `libc.so`, whose address isn't known until runtime. Calls go through a small trampoline in your program, the **PLT** (Procedure Linkage Table), which the dynamic loader patches.

```nasm
extern malloc
    call    malloc              ; links in -no-pie only
    call    malloc wrt ..plt    ; correct
```

Without it, linking `main.c` with your library fails with (tested in this codespace, gcc 13 / binutils 2.42):

```
relocation R_X86_64_PC32 against symbol `malloc@@GLIBC_2.2.5' can not be used
when making a PIE object; recompile with -fPIE
```

Calls between **your own** functions (e.g. `ft_strdup` calling `ft_strlen` inside the same library) work without `wrt ..plt` because they end up in the same executable, but adding it doesn't hurt.

### 2. Data references need to be RIP-relative

If you have any data in `.data`/`.rodata` (e.g. a constant table in a bonus), `lea rax, [msg]` would by default encode an absolute 32-bit address, which fails with `relocation R_X86_64_32S ... can not be used when making a PIE object`. Fix: put `default rel` at the top of the file (or write `[rel msg]` each time) so addresses are computed relative to `rip`.

## The GNU-stack note

Since binutils 2.39 the linker warns:

```
warning: ft_strlen.o: missing .note.GNU-stack section implies executable stack
```

C compilers emit an empty `.note.GNU-stack` section in every object file to say "this code doesn't need an executable stack". NASM doesn't do it for you. Add this line at the end of every `.s` file:

```nasm
section .note.GNU-stack noalloc noexec nowrite progbits
```

It's not required by the subject, but a clean build looks better in an evaluation, and an executable stack is a real security weakness.

## macOS differences (in case you evaluate on a Mac)

- Symbols get a leading underscore: `global _ft_strlen`, `call _malloc`.
- Object format: `nasm -f macho64`.
- `errno` accessor is `___error` (three underscores: the C name `__error` plus the Mach-O underscore), not `__errno_location`.
- Syscall numbers are offset by `0x2000000` (`write` = `0x2000004`), and errors are signalled with the carry flag, not a negative `rax`.

A portable library can use `%ifdef` with a `-D` flag passed from the Makefile, but you only need to support the platform you're evaluated on. Ask your campus.

## Checklist

- [ ] I can list the 6 argument registers in order and the 7 callee-saved ones.
- [ ] Every function of mine that does `call` realigns the stack, and I can show the arithmetic.
- [ ] I keep values I need after a `call` in callee-saved registers, and I push/pop those.
- [ ] Every `call` to libc has `wrt ..plt`; files with data have `default rel`.
