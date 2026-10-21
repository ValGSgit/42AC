# 04 — The mandatory functions

For each function: the contract (from the man page), the plan in plain words, the traps, and what to test. No finished code, that's your part. If you get stuck, write the C version first in the simplest form possible (only `while`, `if`, pointer increments), then translate it line by line.

Read the real man pages too: `man 3 strlen`, `man 2 write`, etc. If `man` is missing in the codespace: `sudo apt install man-db manpages-dev` and `sudo unminimize` (or read them online, see [REFERENCES.md](REFERENCES.md)).

---

## ft_strlen

```c
size_t ft_strlen(const char *s);
```

**Contract:** number of bytes before the first `'\0'`.

**Plan:** keep an index (or a moving pointer), look at one byte at a time, stop at 0, return the count in `rax`.

**Traps:**
- The return type is `size_t`: the result must be in the full `rax`, not just `eax`.
- Compare a **byte**: `cmp byte [rdi + rax], 0`. Without `byte`, NASM complains it doesn't know the size.

**Variants worth knowing (pick one, be able to explain it):**
- Index loop: `rax` = counter, `[rdi + rax]`.
- Two-pointer: walk a copy of `rdi`, return `end - start`.
- `repne scasb`: the old "string instruction" way. Set `al = 0`, `rcx = -1`, run `repne scasb`, then the length can be computed from `rcx` (`not rcx; dec rcx`). Short, but slower than a plain loop on modern CPUs and harder to explain. Fine if you understand it.

**Tests:** `""`, `"a"`, a long string (a few KB), a string with high bytes (`"\xff\xfe"`).

---

## ft_strcpy

```c
char *ft_strcpy(char *dst, const char *src);
```

**Contract:** copy `src` *including the terminating `'\0'`* into `dst`. Return `dst`.

**Plan:** loop: load a byte from `src`, store it at `dst`, if it was 0 stop, otherwise advance. Return the *original* `dst`.

**Traps:**
- You can't `mov [rdi], [rsi]`: no memory-to-memory moves. Go through a register (`al`, `cl`...).
- Copy the `'\0'` too. Easiest: store first, *then* test.
- If you advance `rdi`, you've lost the original pointer. Either copy it to `rax` at the start, or use an index register and never move `rdi`.

**Tests:** `""` (dst must become `""`), normal string, compare `ft_strcpy(buf, s) == buf` (pointer equality), compare buffer contents with the real `strcpy`. Pre-fill `dst` with garbage (`memset(buf, 'X', ...)`) so a missing `'\0'` shows up.

---

## ft_strcmp

```c
int ft_strcmp(const char *s1, const char *s2);
```

**Contract:** negative, zero, or positive depending on whether `s1` is less than, equal to, or greater than `s2`. The man page: *the comparison is done using unsigned characters.*

**Plan:** walk both strings together. Stop when the bytes differ, or when the byte from `s1` is `'\0'` (if both are `'\0'` they're also equal, so this covers the end). Return `(unsigned char)*s1 - (unsigned char)*s2`.

**Traps:**
- **Unsigned bytes.** Load with `movzx`. If you use `movsx` (or compare with `jl`/`jg`), `"\x80"` will sort before `"a"`. glibc says `"\x80" > "a"`.
- **Subtract in 32 bits** after zero-extending both. `eax = byte1 - byte2` gives a correct signed `int` in `[-255, 255]`. Subtracting in 8 bits (`sub al, cl`) wraps around.
- **Exact value:** the man page only promises the *sign*. glibc's actual return values vary by version/CPU (sometimes -1/1, sometimes the difference). Your tests should compare signs, not exact values, unless you know what you're comparing to. Returning the byte difference is the common choice and easy to defend.

**Tests:** equal strings, `""` vs `""`, `""` vs `"a"` both ways, prefix (`"abc"` vs `"abcd"`), differs in the middle, `"\x80"` vs `"a"`, `"\xff"` vs `"\x01"`.

---

## ft_write

```c
ssize_t ft_write(int fd, const void *buf, size_t count);
```

**Contract:** `man 2 write`. Returns the number of bytes written, or -1 with `errno` set.

**Plan:** syscall 1 with the arguments as they arrived; if the result is negative, go to the error path from [doc 3](03_SYSCALLS_AND_ERRNO.md#the-pattern-in-steps).

**Traps:**
- Don't set `errno` on success.
- Return `-1`, not `-errno`.
- `errno` is 4 bytes.
- The stack must be aligned before `call __errno_location`, and balanced before `ret` on **both** paths.
- `fd` is an `int`: in theory the upper half of `rdi` is unspecified. The kernel only looks at the low 32 bits of `fd` for `write`, so passing `rdi` straight through is fine. Know that if asked.

**Tests:** write to stdout; to stderr; to a file (`open`, `ft_write`, `close`, read back); `fd = -1` → -1 and `errno == EBADF`; `buf = NULL` with `count > 0` → -1 and `EFAULT`; `count = 0` → 0. Always compare return value **and** `errno` with the real `write`.

---

## ft_read

```c
ssize_t ft_read(int fd, void *buf, size_t count);
```

**Contract:** `man 2 read`. Same shape as `ft_write`, syscall number 0. Returns 0 at end of file.

**Tests:** read from a file (compare with real `read` on a second fd of the same file), read the whole file in small chunks until 0, read from stdin (`echo hello | ./test`), bad fd → `EBADF`, `NULL` buffer → `EFAULT`, a directory fd → `EISDIR`.

---

## ft_strdup

```c
char *ft_strdup(const char *s);
```

**Contract:** allocate `strlen(s) + 1` bytes with `malloc`, copy `s` into it, return it. If `malloc` fails, return `NULL` (`errno` is already set to `ENOMEM` by `malloc`).

**Plan:**
1. Save `s` somewhere that survives calls.
2. Length = `ft_strlen(s)` (call your own function!), plus one.
3. `malloc(length)`.
4. If `NULL`, return `NULL`.
5. `ft_strcpy(new, s)` returns `new`, which is exactly what you want to return.

**Traps (this is where the calling convention bites):**
- After `call ft_strlen` and `call malloc`, `rdi` no longer holds `s`. Keep it in a callee-saved register (`rbx`, `r12`...) that you push at entry and pop at exit.
- Alignment: count your pushes. One push = aligned. Two pushes = misaligned, add `sub rsp, 8`.
- `call malloc wrt ..plt` ([doc 2](02_CALLING_CONVENTION.md#pie-why-call-malloc-doesnt-link)).
- Don't forget the `+1` for `'\0'`.
- Pop everything on the `NULL` path too.

**Tests:** `""`, normal string, long string; check the result `!= s` (new memory) and the content is equal; `free` it. Run under valgrind: no invalid read/write, no leak.

---

## ft_calloc

```c
void *ft_calloc(size_t nmemb, size_t size);
```

**Contract:** allocate `nmemb * size` bytes, all zero. On failure return `NULL` with `errno = ENOMEM`. The subject adds: **detect `nmemb * size` overflow yourself**, don't rely on `malloc` to fail on the wrapped-around product.

**Why the overflow check matters:** `calloc(0x100000001, 0x100000000)` mathematically asks for an enormous amount, but in 64-bit arithmetic the product wraps to a small number. A naive `malloc(nmemb * size)` succeeds with a tiny buffer, the caller thinks it has a huge array, and writes past the end. That's a real class of security bug (heap overflow via integer overflow).

**How to detect it in asm (not obvious, very short):** `mul` does an unsigned 64×64 → 128-bit multiply: `rdx:rax = rax * operand`. If the high half `rdx` is non-zero, the product didn't fit, and the CPU tells you by setting **CF and OF**. So: put one factor in `rax`, `mul` by the other, then `jc` (or `jo`) to your overflow path. Note that `mul` overwrites `rdx`, so read its operand *before* clobbering anything you still need.

**Plan:**
1. Product with overflow check. On overflow: `errno = ENOMEM` (12) via `__errno_location`, return `NULL`.
2. Save the product (you need it after `malloc` to know how much to zero).
3. `malloc(product)`. If `NULL`, return `NULL` (malloc set errno).
4. Zero the bytes: a loop, or `rep stosb` (`rdi` = destination, `rcx` = count, `al` = 0). `rep stosb` advances `rdi`, so keep the pointer from `malloc` somewhere else to return it.
5. Return the pointer.

**Traps:**
- `nmemb == 0` or `size == 0`: product 0, no overflow. glibc returns a unique pointer you can `free`. Calling `malloc(0)` gives you exactly that; no special case needed. Don't treat 0 as an error.
- The same callee-saved / alignment discipline as `ft_strdup`, now with two values to keep across the call (the product and maybe nothing else, think about it).
- Don't use `imul` for the check: it's signed, so its overflow flag answers a different question.

**Tests:** small sizes, check every byte is 0 (malloc'd memory is often already zero on fresh pages, so first `malloc`+fill+`free` a block of the same size to make it likely you get dirty memory back); `(0, 10)`, `(10, 0)`, `(0, 0)` → non-NULL, freeable; `(SIZE_MAX, 2)` → `NULL` and `errno == ENOMEM`; `(1ULL << 32, 1ULL << 32)` → overflow → `NULL`; a huge but non-overflowing request like `(1, SIZE_MAX)` → `NULL` from `malloc`, `errno == ENOMEM`. Valgrind clean.

---

## Checklist before calling the mandatory part done

- [ ] Every function matches the real one on every test, including return value **and** `errno`.
- [ ] No warnings from `nasm`, `gcc -Wall -Wextra -Werror`, or `ld`.
- [ ] `valgrind --leak-check=full` on the test program: 0 errors, 0 leaks.
- [ ] Every function that calls something: stack aligned at every `call`, balanced at every `ret`.
- [ ] I can explain every line without notes.
