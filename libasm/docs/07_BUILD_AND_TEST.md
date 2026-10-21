# 07 — Building and testing

## The Makefile

The [Makefile](../Makefile) in this folder is ready to use (run `make help`), and it's commented. This section explains the ideas behind it, so you can defend it or write your own.

Required rules: `$(NAME)`, `all`, `clean`, `fclean`, `re`, and `bonus`. `NAME = libasm.a`. It must not relink when nothing changed: running `make` twice should print "Nothing to be done" (or nothing) the second time.

The pieces you need:

```make
NAME    = libasm.a
AS      = nasm
ASFLAGS = -f elf64          # 64-bit ELF object files (Linux)

%.o: %.s                    # pattern rule: how to build any .o from its .s
	$(AS) $(ASFLAGS) $< -o $@
```

- `$<` is the first prerequisite (the `.s`), `$@` is the target (the `.o`).
- The library itself is built with `ar rcs $(NAME) <objects>`:
  - `r` insert/replace members, `c` create the archive without complaining, `s` write a symbol index (what `ranlib` does, needed so the linker can find symbols).
- A static library is just an archive of `.o` files. `ar t libasm.a` lists them, `nm libasm.a` lists the symbols (`T ft_strlen` = defined in text, `U malloc` = used but undefined, resolved at final link).
- Recipe lines start with a **tab**, not spaces.

### The bonus relink problem (not obvious)

The usual way to do `bonus` is to add the bonus objects to the same `libasm.a`. The naive version:

```make
bonus: $(OBJ) $(BONUS_OBJ)
	ar rcs $(NAME) $^
```

`bonus` isn't a file, so make thinks it's never up to date and re-runs `ar` every time you type `make bonus`. Evaluators check this. Common fixes, pick one you can explain:

- Make `bonus` depend on a real file, e.g. a stamp file `.bonus` that's created by the rule and whose prerequisites are the objects, so it's only rebuilt when an object changed. `fclean` removes the stamp.
- Or make `$(NAME)` itself depend on the bonus objects when building in bonus mode (a variable switch).

Also think about the sequence `make bonus` then `make`: should `make` drop the bonus functions again? Either answer is defensible if you can explain it, but nothing should relink needlessly.

### Building the test program

The subject wants a `main` you submit, compiled with your library:

```sh
gcc -Wall -Wextra -Werror main.c -L. -lasm -o test
```

- `-L.` adds the current directory to the library search path; `-lasm` means "find `libasm.a`" (the `lib` prefix and `.a` suffix are implied).
- **Order matters (not obvious):** with static libraries the linker scans left to right and only pulls in symbols that are *already needed*. Put `-lasm` *after* `main.c`. `gcc -L. -lasm main.c` fails with "undefined reference to ft_strlen".
- Don't pass `-no-pie`. gcc's default is PIE, which is what the subject wants. `file ./test` should say "pie executable".
- A `make test` rule (and removing `test` in `fclean`) is convenient but not required.

## Writing the test main

Compare against the real function, every time, return value **and** `errno`:

```c
errno = 0;
ssize_t r1 = write(-1, "x", 1);
int     e1 = errno;
errno = 0;
ssize_t r2 = ft_write(-1, "x", 1);
int     e2 = errno;
printf("%s write bad fd: %zd/%d vs %zd/%d\n",
       (r1 == r2 && e1 == e2) ? "OK" : "KO", r1, e1, r2, e2);
```

Gotchas found while preparing this guide:

- **Read `errno` in its own statement.** `printf("%zd %d", ft_write(-1, "x", 1), errno)` is wrong: C doesn't specify the order in which arguments are evaluated, and gcc evaluated `errno` *before* the call, printing 0.
- **gcc refuses obviously huge allocations at compile time.** `calloc(1, SIZE_MAX)` with `-Werror` fails with `-Werror=alloc-size-larger-than=`. Put the sizes in variables the compiler can't fold (e.g. `volatile size_t big = SIZE_MAX;`). Your own `ft_calloc` isn't affected since it has no `alloc_size` attribute.
- **gcc may replace `strlen("literal")` with a constant** and `strcmp("a", "b")` with a fixed result. Fine for comparison, but if you want to see what libc returns at runtime, use non-constant strings (e.g. from `argv` or a buffer).
- **Compare signs for `strcmp`**, not exact values (see [doc 4](04_MANDATORY_GUIDE.md#ft_strcmp)).
- Pre-fill destination buffers with garbage before `ft_strcpy`, so a missing `'\0'` shows up.

A small helper macro or function that prints OK/KO per test keeps the output readable during the defense.

## Debugging with gdb

Put this in `~/.gdbinit` so gdb uses Intel syntax like your code:

```
set disassembly-flavor intel
```

Build with debug info: add `-g -F dwarf` to `ASFLAGS` while developing (and `-g` to the `gcc` line). Then:

```sh
gdb ./test
```

| Command | What it does |
|---------|--------------|
| `break ft_strdup` | stop when `ft_strdup` is entered |
| `run` | start the program |
| `layout asm` / `layout regs` | split-screen view of the code / registers (`tui disable` to exit) |
| `si` / `ni` | step one instruction (into / over calls) |
| `info registers rdi rsi rax` | show registers (`i r` for all) |
| `p/x $rax`, `p (long)$rax` | print a register in hex / as a signed number |
| `x/s $rdi` | memory at `rdi` as a string |
| `x/16xb $rdi` | 16 bytes at `rdi` in hex |
| `x/4gx $rsp` | top 4 qwords of the stack |
| `p $rsp % 16` | stack alignment check: must be `0` right before a `call` |
| `finish` | run until the current function returns, show the return value |
| `bt` | backtrace: where did the crash happen |
| `p $xmm0` | vector register contents (for the SIMD bonus) |

When a crash happens *inside* libc (`malloc`, `printf`), check alignment and callee-saved registers in *your* function first. `bt` shows which of your functions called it.

## Valgrind

```sh
valgrind --leak-check=full --show-leak-kinds=all ./test
```

Expect "All heap blocks were freed" and "ERROR SUMMARY: 0 errors". "Invalid read/write" in your function means you're touching memory you shouldn't (e.g. writing past the `malloc`'d size in `ft_strdup`). "Conditional jump depends on uninitialised value" often means a `calloc` that didn't zero everything.

## Other tools

- `strace -e trace=read,write ./test` — see your raw syscalls and their return values.
- `objdump -d -M intel ft_strlen.o` — disassemble your object to check what nasm actually produced.
- `nm libasm.a` — check every `ft_*` symbol is there and exported (`T`).
- [Compiler Explorer](https://godbolt.org) with `-O1 -masm=intel` — write the C version and see how gcc compiles it. Great for learning idioms; just don't copy its output as your submission, you need to understand and own every line.
