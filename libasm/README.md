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
7. Copy the mandatory part into `libasm_bonus/` with the `_bonus` names (`ft_strlen.s` → `ft_strlen_bonus.s`, `libasm.h` → `libasm_bonus.h`, `main.c` → `main_bonus.c`).
8. Bonus, in `libasm_bonus/`: `ft_list_size` → `ft_list_push_front` → `ft_list_sort` → `ft_list_remove_if` → `ft_atoi_base` → `ft_simd_memchr`.

## Mandatory and bonus

The two parts live in separate folders, and each one is a complete library on its own:

- **This folder is the mandatory part**: the seven functions, `libasm.h` and `main.c`. No bonus code in here, and no `#ifdef BONUS` in `main.c`.
- **`libasm_bonus/` is the full library with the bonus**: a copy of the mandatory functions plus the bonus ones, its own header, its own test `main` and its own Makefile. Nothing in there includes or assembles anything from the mandatory folder. Every file has a `_bonus` suffix, so the duplicated mandatory functions are `ft_strlen_bonus.s` and so on. The symbols inside stay `ft_strlen`, ... of course.

`make` here builds `libasm.a` with the mandatory functions. `make bonus` builds `libasm_bonus/libasm.a` with the mandatory and the bonus functions. The two never overwrite each other, so `make bonus` followed by `make` doesn't rebuild or drop anything.

## The layout you'll end up with

This is a suggestion; only the names `libasm.a` and `*_bonus.s` are imposed by the subject. Right now the folders only have the Makefiles and the docs. The `.s` files, headers and mains are yours to write.

```
libasm/
├── Makefile                      <- provided: auto-detects ft_*.s, has test/debug helpers (make help)
├── libasm.h                      <- prototypes, for main.c
├── main.c                        <- the test program the subject asks you to submit
├── ft_strlen.s
├── ft_strcpy.s
├── ft_strcmp.s
├── ft_write.s
├── ft_read.s
├── ft_strdup.s
├── ft_calloc.s
├── libasm_bonus/
│   ├── Makefile                  <- provided: auto-detects ft_*_bonus.s, same helpers
│   ├── libasm_bonus.h            <- all the prototypes + t_list, for main_bonus.c
│   ├── main_bonus.c              <- tests for everything, mandatory and bonus
│   ├── ft_strlen_bonus.s         <- the seven mandatory functions again...
│   ├── ...
│   ├── ft_calloc_bonus.s
│   ├── ft_atoi_base_bonus.s      <- ...and the bonus ones
│   ├── ft_list_push_front_bonus.s
│   ├── ft_list_size_bonus.s
│   ├── ft_list_sort_bonus.s
│   ├── ft_list_remove_if_bonus.s
│   └── ft_simd_memchr_bonus.s
└── docs/
```

Prototypes, straight from the man pages and the subject. `libasm.h` gets the first seven, `libasm_bonus.h` gets all of them:

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

From this folder:

| Command | What it does |
|---------|--------------|
| `make` | Build `libasm.a` from every `ft_*.s` in this folder (the mandatory part). |
| `make bonus` | Run the Makefile in `libasm_bonus/`, which builds `libasm_bonus/libasm.a` from every `ft_*_bonus.s` there (mandatory + bonus). |
| `make clean` / `fclean` | The usual, in both folders. |
| `make re` | `fclean` + `make` (mandatory). |
| `make test` | Compile `main.c` against the library and run it. `ARGS="..."` passes arguments. |
| `make valgrind` / `gdb` / `strace` | Run the tests under that tool. |
| `make re DEBUG=1` | Rebuild with debug info, for stepping through your asm in gdb. |
| `make syms` | `nm` the library: every `ft_*` should show as `T`. |
| `make relink-check` | Confirms `make` and `make bonus` don't rebuild anything needlessly (evaluators check this). |
| `make try S=scratch/add.s` | Assemble one file outside the library and run it with `scratch/add.c` as its `main`. `C=other.c` picks a different driver, `RUN="gdb -q"` runs it under a tool. |

`libasm_bonus/` has its own Makefile with the same rules except `bonus` and `try`, so you can `cd libasm_bonus` and use `make`, `make re`, `make test` (it builds and runs `main_bonus.c`), `make valgrind`, `make syms`, `make relink-check`, ... directly.

Because the Makefiles pick up sources with `wildcard`, you never have to edit them when you add a function. Some evaluators prefer explicit file lists. If yours does, replace the `wildcard` line in each Makefile with a list.

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
- **Bonus files end in `_bonus.s`.** Bonus is only graded if the mandatory part is perfect. Here they all go in `libasm_bonus/`.
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
