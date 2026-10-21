# 03 — Syscalls and errno

`ft_write` and `ft_read` don't call libc's `write`/`read` (that would be cheating). They talk to the kernel directly with the `syscall` instruction, and then do the one job libc's wrapper does on top: set `errno` when something goes wrong.

## The `syscall` instruction (Linux x86-64)

| | Register |
|-|----------|
| Syscall number | `rax` |
| Arguments 1–6 | `rdi`, `rsi`, `rdx`, **`r10`**, `r8`, `r9` |
| Return value | `rax` |
| Destroyed by `syscall` | `rcx`, `r11` (and `rax`, which holds the result) |

Almost the same registers as a function call. Two differences:

- The 4th argument goes in `r10`, not `rcx`, because the CPU uses `rcx` to remember where to return to. `write` and `read` only have 3 args, so this doesn't affect you here.
- `rcx` and `r11` are destroyed.

Numbers for this project (from `/usr/include/x86_64-linux-gnu/asm/unistd_64.h`):

| Syscall | Number |
|---------|--------|
| `read`  | 0 |
| `write` | 1 |

Fun consequence: `ft_write(fd, buf, count)` arrives with `fd` in `rdi`, `buf` in `rsi`, `count` in `rdx`, which is exactly where the syscall wants them. You only need to set `rax`.

## How the kernel reports errors

The raw syscall doesn't know about `errno`. It returns:

- a value ≥ 0 on success (bytes written/read),
- **minus the error code** on failure, e.g. `-9` for `EBADF` (bad file descriptor) or `-14` for `EFAULT` (bad address).

Strictly, the error range is `-4095` to `-1`, but for `read`/`write` (which return at most `SSIZE_MAX`) "negative = error" is fine: `test rax, rax` then `js .error`.

The libc **contract** you must reproduce (see `man 2 write`, RETURN VALUE): on error, return **-1** and set `errno` to the (positive) error code.

## Setting errno

### Why `errno` is a function

You might expect `errno` to be a global `int`. It isn't: in a multi-threaded program, each thread has its own `errno`. In glibc's `<errno.h>`:

```c
extern int *__errno_location(void);
# define errno (*__errno_location ())
```

So `errno = 9;` in C compiles to "call `__errno_location`, get a pointer to *this thread's* `errno`, store 9 there". You do the same. On macOS the function is called `__error` (`___error` from asm). The subject mentions both.

### The pattern, in steps

When the syscall returned a negative value in `rax`:

1. Turn it into the positive error code (`neg`).
2. Keep that code somewhere that survives a `call`: `__errno_location` is a normal C function and may destroy every scratch register.
3. Make sure the stack is 16-byte aligned for the call ([doc 2](02_CALLING_CONVENTION.md#the-stack-and-its-16-byte-alignment)).
4. `call __errno_location wrt ..plt` — `rax` now holds an `int *`.
5. Store the code through the pointer, as a **dword** (`errno` is an `int`, 4 bytes, not 8).
6. Undo whatever you did to the stack, put `-1` in `rax`, `ret`.

A couple of syntax pieces you'll need, without the full function:

```nasm
extern __errno_location

    mov     dword [rax], ecx        ; *errno_ptr = code (4 bytes)
    mov     rax, -1                 ; return -1 as a 64-bit ssize_t
```

Think about step 2 for a minute: the cleanest solution saves a callee-saved register with `push` and uses it to hold the code, and that one `push` also fixes the alignment for step 3. Two problems solved with one instruction.

### What not to do

- **Don't set `errno` on success.** libc never resets it to 0. Tests often do `errno = 0; ft_write(...)` and expect it untouched.
- **Don't return the negative code** (e.g. -9). The caller expects exactly -1.
- **Don't write a `qword` into `errno`.** You'd overwrite 4 bytes of whatever lives after it in thread-local storage.
- **Don't write to a global named `errno`** you declared yourself. It won't be the one C reads.

## errno for library functions

`ft_strdup` and `ft_calloc` call `malloc`. If `malloc` fails, it has already set `errno = ENOMEM`; you just return `NULL`. The exception is the overflow case in `ft_calloc`: you refuse *before* calling `malloc`, so you must set `errno = ENOMEM` (12) yourself, the same way as above. That's what glibc's `calloc` does (checked in this codespace: `calloc(SIZE_MAX, 2)` returns `NULL` with `errno == 12`).

## Useful error codes

From `/usr/include/asm-generic/errno-base.h`, or run `errno -l` (package `moreutils`):

| Name | Value | Easy way to trigger it |
|------|-------|------------------------|
| `EBADF` | 9 | `write(-1, "a", 1)`, `read(42, buf, 1)` |
| `EFAULT` | 14 | `write(1, NULL, 3)`, `read(0, NULL, 3)` |
| `EISDIR` | 21 | `read()` on an fd from `open(".", O_RDONLY)` |
| `ENOMEM` | 12 | your `ft_calloc(SIZE_MAX, 2)` |

## Watching syscalls happen

`strace` shows every syscall a program makes, with arguments and return values. Great for checking `ft_write` really calls `write(2)` with the right args:

```sh
strace -e trace=read,write ./test
```

## Checklist

- [ ] I know which registers `syscall` uses and which it destroys.
- [ ] I know how the kernel signals failure and what libc returns instead.
- [ ] I can explain why `errno` goes through `__errno_location()` and that it's a 4-byte `int`.
- [ ] My error path keeps the stack aligned and balanced.
