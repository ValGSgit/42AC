# libasm

Rewriting a few libc functions in x86-64 assembly (NASM, Intel syntax, Linux) and bundling them into a static library, `libasm.a`.

This folder starts out as **a study guide, not a solution**. The docs explain everything you need to write the code yourself, including the parts the subject takes for granted (calling convention, `errno`, PIE linking, SIMD). There are small snippets showing *syntax* and *patterns*, but no finished function. You write the `.s` files.

## Where to start

Read the docs in order. Each one ends with a short checklist or exercises, so you know when you're ready to move on.

| # | Doc | What's in it |
|---|-----|--------------|
| 1 | [docs/01_ASM_BASICS.md](docs/01_ASM_BASICS.md) | Registers, sizes, NASM syntax, the instructions you'll actually use, flags and jumps. |
| 2 | [docs/02_CALLING_CONVENTION.md](docs/02_CALLING_CONVENTION.md) | System V ABI: where arguments go, which registers you must preserve, stack alignment, calling `malloc` from asm, PIE and `wrt ..plt`. **The most important doc.** |
| 3 | [docs/03_SYSCALLS_AND_ERRNO.md](docs/03_SYSCALLS_AND_ERRNO.md) | How `syscall` works, how the kernel reports errors, and how to set `errno` yourself. |
| 4 | [docs/04_MANDATORY_GUIDE.md](docs/04_MANDATORY_GUIDE.md) | Function by function: what it must do, the edge cases, the traps, and how to test it. |
| 5 | [docs/05_BONUS_GUIDE.md](docs/05_BONUS_GUIDE.md) | `ft_atoi_base` and the linked list functions. |
| 6 | [docs/06_SIMD_MEMCHR.md](docs/06_SIMD_MEMCHR.md) | `ft_simd_memchr`: SSE2 and SWAR, and how to never read into an unmapped page. |
| 7 | [docs/07_BUILD_AND_TEST.md](docs/07_BUILD_AND_TEST.md) | What the Makefile needs, `ar`, linking the test `main`, gdb, valgrind, writing good tests. |
| - | [docs/REFERENCES.md](docs/REFERENCES.md) | Manuals, ABI spec, instruction references, tools. |

## Suggested order of work

1. Read docs 1–3 once, without coding.
2. Create `libasm.h` and a `main.c` first, so `make test` works from minute one. For the doc 1 exercises, use `make try S=scratch/add.s` (see below).
3. `ft_strlen` → `ft_strcpy` → `ft_strcmp`. No calls, no syscalls, just loops. Get comfortable with gdb here.
4. `ft_write` → `ft_read`. First syscalls, first `errno`.
5. `ft_strdup` → `ft_calloc`. First calls into libc, first time stack alignment and callee-saved registers really matter.
6. Polish the mandatory part until it's perfect. The bonus isn't graded otherwise.
7. Bonus: `ft_list_size` → `ft_list_push_front` → `ft_list_sort` → `ft_list_remove_if` → `ft_atoi_base` → `ft_simd_memchr`.

## The layout you'll end up with

This is a suggestion; only the names `libasm.a` and `*_bonus.s` are imposed by the subject.

```
libasm/
├── Makefile              <- provided: auto-detects ft_*.s, has test/debug helpers (make help)
├── libasm.h              <- prototypes + t_list, for main.c
├── main.c                <- the test program the subject asks you to submit
├── ft_strlen.s
├── ft_strcpy.s
├── ft_strcmp.s
├── ft_write.s
├── ft_read.s
├── ft_strdup.s
├── ft_calloc.s
├── ft_atoi_base_bonus.s
├── ft_list_push_front_bonus.s
├── ft_list_size_bonus.s
├── ft_list_sort_bonus.s
├── ft_list_remove_if_bonus.s
├── ft_simd_memchr_bonus.s
└── docs/
```

Prototypes for `libasm.h`, straight from the man pages and the subject:

```c
size_t       ft_strlen(const char *s);
char        *ft_strcpy(char *dst, const char *src);
int          ft_strcmp(const char *s1, const char *s2);
ssize_t      ft_write(int fd, const void *buf, size_t count);
ssize_t      ft_read(int fd, void *buf, size_t count);
char        *ft_strdup(const char *s);
void        *ft_calloc(size_t nmemb, size_t size);

typedef struct s_list { void *data; struct s_list *next; } t_list;

int          ft_atoi_base(char *str, char *base);
void         ft_list_push_front(t_list **begin_list, void *data);
unsigned int ft_list_size(t_list *begin_list);
void         ft_list_sort(t_list **begin_list, int (*cmp)());
void         ft_list_remove_if(t_list **begin_list, void *data_ref,
                               int (*cmp)(), void (*free_fct)(void *));
void        *ft_simd_memchr(const void *s, int c, size_t n);
```

## Using the Makefile

| Command | What it does |
|---------|--------------|
| `make` / `make bonus` | Build `libasm.a`. Every `ft_*.s` goes in; `make bonus` also adds every `ft_*_bonus.s`. |
| `make clean` / `fclean` / `re` | The usual. |
| `make test` | Compile `main.c` against the library and run it. `ARGS="..."` passes arguments. |
| `make test_bonus` | Same, compiled with `-DBONUS` against the bonus library. Put bonus tests in `#ifdef BONUS`. |
| `make valgrind` / `gdb` / `strace` | Run the tests under that tool. Add `B=1` for the bonus build. |
| `make re DEBUG=1` | Rebuild with debug info, for stepping through your asm in gdb. |
| `make syms` | `nm` the library: every `ft_*` should show as `T`. |
| `make relink-check` | Confirms `make` and `make bonus` don't rebuild anything needlessly (evaluators check this). |
| `make try S=scratch/add.s` | Assemble one file outside the library and run it with `scratch/add.c` as its `main`. `C=other.c` picks a different driver, `RUN="gdb -q"` runs it under a tool. |

Because the Makefile picks up sources with `wildcard`, you never have to edit it when you add a function. Some evaluators prefer explicit file lists. If yours does, replace the two `wildcard` lines with lists.

## What you need

- Linux on x86-64 (this codespace is fine, so is WSL).
- `nasm`, `gcc` (used to compile `main.c` and as the linker), `make`, `ar` (comes with binutils).
- Strongly recommended: `gdb` and `valgrind`.

On Debian/Ubuntu: `sudo apt install nasm gcc make gdb valgrind`

## The rules that are easy to miss

These are all in the subject, but scattered. Every one of them can cost you the evaluation.

- **64-bit, NASM, Intel syntax, `.s` files.** No inline asm in C.
- **`-no-pie` is forbidden.** So your code must be position-independent: `default rel` for data, `wrt ..plt` for calls into libc. See [doc 2](docs/02_CALLING_CONVENTION.md#pie-why-call-malloc-doesnt-link).
- **Respect the calling convention.** A function that clobbers `rbx` may "work" in your test and crash in the evaluator's.
- **Set `errno`** in `ft_write`/`ft_read` on failure, via `__errno_location` (Linux) or `___error` (macOS).
- **`ft_calloc` must detect `nmemb * size` overflow itself.** Don't just pass the product to `malloc`.
- **The Makefile must not relink** when nothing changed, and needs `$(NAME) all clean fclean re` plus `bonus`.
- **Bonus files end in `_bonus.s`.** Bonus is only graded if the mandatory part is perfect.
- **Submit a `main`** that tests the functions against the real ones.
- **No crashes** except on undefined behavior (e.g. `ft_strlen(NULL)` may segfault, just like the real one).
- **AI transparency:** the subject asks you to say what was AI-generated. These docs were written with an AI assistant; your `.s` files should be yours, and you should be able to explain every line during the defense.

## Defense prep

Be ready to answer, without notes:

- Which registers hold the 1st…6th arguments? Which hold the return value? Which are callee-saved?
- Why must `rsp` be 16-byte aligned before a `call`, and how did you make sure it is?
- What does `syscall` clobber? How do you know a syscall failed?
- What does `__errno_location` return, and why is `errno` a function call and not a plain global?
- What's `wrt ..plt`? What is PIE and why does it matter here?
- How does your `ft_calloc` detect overflow?
- (Bonus) Which instruction in `ft_simd_memchr` compares several bytes at once? Why can't your loads fault?
