# 06 — ft_simd_memchr

```c
void *ft_simd_memchr(const void *s, int c, size_t n);
```

Same result as `memchr`: pointer to the first byte in `s[0..n-1]` equal to `(unsigned char)c`, or `NULL`. The twist: each comparison must test **several bytes at once**, and you must **never read memory that could be unmapped**.

The subject accepts two techniques. Pick one, master it, and be ready to point at the instruction that does the parallel comparison.

## The safety rule, explained

Memory is mapped in **pages** of 4096 bytes. If `s` is valid, the whole page containing `s` is mapped. The page after `s + n` might not be: reading even one byte there segfaults.

Key fact: **4096 is a multiple of 16 (and of 8).** So a 16-byte load from a **16-byte-aligned address** can never straddle two pages: the 16 bytes are all in the same page. If at least one of those bytes belongs to your buffer, the page is mapped, and the whole load is safe.

```
page boundary                                          page boundary
|                                                              |
| ... [aligned 16] [aligned 16] [aligned 16] [aligned 16] ...  |  (unmapped?)
            ^s                           ^s+n
```

So the strategy is:
- **Only ever load aligned chunks** (address rounded down to a multiple of 16: `and rax, -16`).
- **Only load a chunk if it contains at least one byte of `[s, s+n)`.**
- Bytes in a loaded chunk that are outside `[s, s+n)` are harmless to read, but must be **ignored** in the result.

An *unaligned* 16-byte load at `s + n - 1` could cross into the next page. That's the bug the subject warns about.

`n == 0`: return `NULL` without loading anything.

**Wrap-around (not obvious):** people do call `memchr(s, c, SIZE_MAX)` when they know `c` is in there. Then `s + n` overflows. If you compute an end pointer, clamp it (`if (end < s) end = UINTPTR_MAX`), or work with remaining counts instead.

---

## Option A: SSE2 (16 bytes at a time)

SSE2 is part of the x86-64 baseline: every 64-bit x86 CPU has it, so no CPU detection needed.

### The instructions

| Instruction | What it does |
|-------------|--------------|
| `movd xmm1, esi` | copy 32 bits from a GPR into the low lane of `xmm1` |
| `punpcklbw xmm1, xmm1` | interleave low bytes: `c` → `c c` in each 16-bit lane |
| `pshuflw xmm1, xmm1, 0` | copy word 0 into words 0–3 |
| `pshufd xmm1, xmm1, 0` | copy dword 0 into dwords 0–3 → `c` in all 16 bytes |
| `movdqa xmm0, [rdi]` | aligned 16-byte load (faults if not aligned, which helps catch bugs) |
| `pcmpeqb xmm0, xmm1` | **the parallel compare**: for each of 16 bytes, `0xFF` if equal, else `0x00` |
| `pmovmskb eax, xmm0` | take the top bit of each of the 16 bytes → 16-bit mask in `eax` (bit *i* = byte *i* matched) |
| `bsf eax, eax` / `tzcnt eax, eax` | index of the lowest set bit = first match. `bsf` is undefined on 0 input, so test for 0 first. |

The three-instruction broadcast (`movd`/`punpcklbw`/`pshuflw`/`pshufd`) is the SSE2 way. With SSSE3 you could do `pxor xmm2, xmm2` + `pshufb xmm1, xmm2`, and with AVX2 `vpbroadcastb`, but those need CPU support; SSE2 is the safe choice.

### Turning the mask into an answer

Little-endian: byte *i* of the chunk is bit *i* of the mask, so "lowest set bit" = "first in memory".

1. **First chunk** = `s` rounded down to 16. `off = s & 15` bytes at the start of it are *before* `s`. Clear the low `off` bits of the mask: shift right by `off` then back left, or `and` with `-1 << off`. (Shift counts go in `cl`.)
2. **Last chunk**: if the buffer ends inside this chunk, with `k` valid bytes from the chunk's start (1 ≤ k ≤ 16), keep only the low `k` bits: `and` with `(1 << k) - 1`. With 32-bit registers, `k = 16` is fine (`1 << 16` doesn't overflow).
3. If the mask is non-zero: answer = chunk address + `tzcnt(mask)`.
4. Otherwise: next aligned chunk (+16), unless you've passed the end → `NULL`.

A chunk can be both first and last (small `n`): apply both masks.

### Sketch

```
if n == 0: return NULL
broadcast c into xmm1
p   = s & ~15
end = s + n   (clamped on overflow)
mask = movemask(pcmpeqb(load_aligned(p), xmm1))
mask &= ~0 << (s - p)
loop:
    if end - p <= 16: mask &= (1 << (end - p)) - 1
    if mask: return p + tzcnt(mask)
    p += 16
    if p >= end: return NULL
    mask = movemask(pcmpeqb(load_aligned(p), xmm1))
```

You can unroll to 32 or 64 bytes per iteration later for speed, but get this correct first.

---

## Option B: SWAR (8 bytes in a general-purpose register)

"SIMD Within A Register": treat a 64-bit register as 8 one-byte lanes and use ordinary integer arithmetic so that all 8 lanes are processed by the same instructions.

### Broadcast

```
pattern = (unsigned char)c * 0x0101010101010101   // c in all 8 bytes
```

(`movzx` then `imul` with the constant in a register: `mov rdx, 0x0101010101010101`. 64-bit immediates only fit in `mov`.)

### Finding a matching byte

`x = word ^ pattern` makes every matching byte **zero**. Then the classic "has a zero byte" trick ([Stanford bit hacks](https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord)):

```
t = (x - 0x0101010101010101) & ~x & 0x8080808080808080
```

`t` has bit 7 of byte *i* set if byte *i* of `x` is zero. Why: subtracting 1 from a zero byte borrows and sets its high bit; `& ~x` removes bytes whose high bit was already set (≥ 0x80) before the subtraction.

**The subtlety:** the borrow from a zero byte propagates into the next byte up, so a byte *above* a true zero can be a false positive (e.g. a byte equal to `c ^ 0x01`). False positives can only appear **above** a real zero, never below. So the **lowest** set bit is always a real match, and that's all memchr needs: first match = `tzcnt(t) / 8` (`shr 3`).

### Masking the edges (careful)

- **Bytes before `s` in the first aligned word:** don't mask them *after* the trick. A real zero there could create a false positive in a valid byte above it, and masking would remove the real one and keep the false one. Instead, neutralize them *before*: set those bytes of `x` to `0xFF` (`x |= (1 << 8*off) - 1`). A `0xFF` byte is never zero and never produces a borrow.
- **Bytes past the end:** masking `t` after the trick is fine (false positives are only above real zeros, and you keep the lowest bit).
- **Shift trap:** `shl rax, cl` uses `cl mod 64`. "Shift by 64 to get 0" silently does nothing. When `k = 8` valid bytes, special-case it (don't mask at all).

Alignment and page safety work exactly as in option A, with 8 instead of 16.

---

## Defense checklist

- [ ] I can point at `pcmpeqb` (SSE) or the `xor` / `sub` / `not` / `and` sequence (SWAR) and explain what each lane does.
- [ ] I can explain why an aligned load can't cross a page, and why my code never loads a chunk entirely past `s + n`.
- [ ] I handle: `n == 0`, `s` unaligned, match before `s` in the first chunk (must be ignored), match after `s + n` in the last chunk (must be ignored), `c` > 255 or negative (only the low byte counts), match at the very first and very last valid byte.

## Testing it properly

The dangerous bugs only show up at page edges, so build a buffer that **ends exactly at a page boundary followed by an unmapped page**:

```c
#include <sys/mman.h>
char *p = mmap(NULL, 2 * 4096, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
mprotect(p + 4096, 4096, PROT_NONE);      // second page: any access segfaults
// buffers ending at p + 4096 have nothing readable after them
for (size_t n = 0; n <= 64; n++)
    for (size_t start = 0; start < 32; start++) { ... s = p + 4096 - n - start ... }
```

Then loop over every start offset 0–31 and every length 0–64, every match position (and no match), and compare with the real `memchr`. If that passes without a segfault, your boundary handling is right.

Note on valgrind: reading past the end of a `malloc`'d buffer within an aligned chunk may be reported as "invalid read of size 16/8". glibc's own SIMD routines get special treatment, yours don't. Valgrind's `--partial-loads-ok` option governs this; the `mmap` test above is the more meaningful one.
