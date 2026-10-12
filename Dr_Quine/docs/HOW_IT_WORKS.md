# How these quines work

These are my notes on how the programs work, written for whoever picks this repo up next, or for me in six months. If you only want to build and run things, the README covers that.

## The problem

A quine has to output its own source. The obvious approach is to put the source in a string and print it, and that runs straight into a wall: the source contains the string, so the string has to contain itself, which has to contain itself, forever.

The way out is to stop trying to store the string inside itself. You store a **template** of the program with a hole where the string goes, and at runtime you fill the hole with the template itself. `printf` does exactly this when you give it the same string twice: once as the format, and once as the `%s` argument.

Here's the idea stripped to the bone. This isn't valid as-is, it's just the shape:

```c
char *s = "char *s = %s; printf(s, s);";
printf(s, s);
```

Printing `s` with `s` plugged into the `%s` gives you back `char *s = "...` with the template in the middle. There are two snags:

1. The string literal needs quote characters around it, and you can't write `"` inside a C string without a backslash. A backslash would then show up in the output and not match.
2. Newlines (and tabs in C) have the same problem: inside the string they would have to be `\n`, but the real file has actual line breaks.

So I don't write those characters in the template at all. I pass them in as numbers and print them with `%c`: 10 is a newline, 34 is `"`, 9 is a tab.

## Positional arguments

Every program uses glibc's positional format specifiers (`%1$c`, `%2$c`, ...) so that each argument can be reused as often as needed without having to pass `10` fifty times. This is the same in all six programs:

| Specifier | Argument | Meaning |
|-----------|----------|---------|
| `%1$c` | `10` | newline |
| `%2$c` | `34` | double quote `"` |
| `%3$s` | the string itself | the hole where the template goes |
| `%4$c` | `9` | tab (C only) |
| `%5$d` (C) / `%4$d` (ASM) | the counter | Sully only, the value written on line 1 |

The ASM files have no tabs (they're indented with spaces), so their positions stop at 3, or at 4 for Sully.

Any `%` that appears in the real code, such as `"Sully_%d.c"` in Sully, becomes `%%` in the template, so it prints back as a single `%`.

One gotcha that cost me some time: **glibc doesn't allow gaps in positional arguments.** If the format uses `%5$d`, then 1, 2, 3 and 4 must all appear somewhere in it too, otherwise the behavior is undefined. Every template here uses every position, so it's fine, but keep this in mind if you change one. For example, if you remove the only tab from a C file, `%4$c` disappears and Sully's `%5$d` is suddenly left with a gap.

## Encoding rules (read this before editing anything)

To get the string literal from the real code, start from the program with a placeholder where the literal goes and apply these steps **in this order**:

1. `%` becomes `%%`
2. each newline becomes `%1$c`
3. each `"` becomes `%2$c`
4. each tab becomes `%4$c` (C only)
5. the placeholder becomes `%3$s`
6. in Sully, the counter's value becomes `%5$d` (C) or `%4$d` (ASM)

Then paste the result between the quotes. The whole thing goes on one line.

Things that must **not** appear anywhere in the code: backslashes, and in ASM, tab characters. There's no escape for them in this scheme. The file must also end with exactly one newline.

If you change even one character in the code part of a file, you need to make the same change in the string, encoded as above. Otherwise the output stops matching. In practice it's easier to write a tiny script that does the encoding than to do it by hand. I did it with a ~20 line Python script.

## Colleen

Colleen is the plain quine. The C version:

```c
void print_source(const char *s)
{
	printf(s, 10, 34, s, 9);
}

int main(void)
{
	/* ... comment ... */
	print_source("...the template...");
	return (0);
}
```

The subject's checklist asks for `main`, a second function that `main` calls (`print_source`), a comment inside `main`, and a comment outside it (the block at the top of the file). All of these are there.

The ASM version has the same structure. `main` calls a `print_source` routine, which loads the arguments into registers (System V order: `rdi`, `rsi`, `rdx`, `rcx`) and calls `printf`. There's a `;` comment at the top of the file and another one inside `main`.

`printf` buffers stdout, and the buffer gets flushed when `main` returns normally, so the output is complete even when redirected into a file.

## Grace

Grace writes to `Grace_kid.c` / `Grace_kid.s` instead of stdout, and has some odd constraints: no declared functions, exactly three `#define`s, one comment, and the program has to run by calling a macro.

The trick in C is that the macro **is** `main`:

```c
#define KID "Grace_kid.c"
#define SRC "...the template..."
#define FT(x) int main(void) { FILE *f = fopen(KID, "w"); ...; fprintf(f, x, 10, 34, x, 9); ... }
FT(SRC)
```

Nothing in the source says `int main` outside a macro. It only exists once the preprocessor expands `FT(SRC)` at the bottom. That's three defines, one comment and one macro call, which is what the subject asks for.

The ASM version follows the same idea with NASM's preprocessor:

- `%define KID` holds the output file name
- `%define SRC` holds the template
- `%macro FT 0 ... %endmacro` contains *the entire program*: sections, data and the `main` routine

The last line of the file is just `FT`, which expands into the whole program. No extra routines exist besides `main`.

Grace returns 1 if `fopen` or `fclose` fails, so it won't crash on a read-only directory.

## Sully

Sully is the fun one. Line 1 holds a counter (`int i = 5;` / `i equ 5`). When you run it:

1. It writes a copy of itself to `Sully_X.c` (or `.s`), with the counter set to X.
2. It compiles that file into `Sully_X`.
3. It runs `Sully_X`, which does the same with X − 1.
4. This keeps going until the counter would drop below 0, and then it stops.

### Original or child?

This took the most thought. The subject's example expects `Sully_5.c` all the way down to `Sully_0.c` (13 files in total for C), and `diff ../Sully.c Sully_0.c` to show `5` versus `0`. That means:

- the **original** `Sully` (i = 5) must write `Sully_5.c` with i = 5, so it **doesn't** decrement
- each **child** `Sully_X` (i = X) must write `Sully_(X-1)`, so it **does** decrement

So the program needs to know whether it's the original or a copy. Reading `argv[0]` is explicitly forbidden, and so is reading the source file. What I used is `__FILE__`, the name of the source file as the compiler saw it, baked in at compile time:

```c
n = i;
if (strncmp(__FILE__, "Sully_", 6) == 0)
	n--;
if (n < 0)
	return (0);
```

The children are always compiled as `clang ... Sully_4.c`, so their `__FILE__` is exactly `"Sully_4.c"`. The original is compiled as `Sully.c`, `../Sully.c` or some absolute path, and none of those *start* with `Sully_`. I check the start of the string on purpose rather than searching it for `Sully_` or `_`, because this repo's own folder is called `dr_quine`, and a full path would contain an underscore.

NASM supports `__FILE__` too, so the ASM version does the same thing: it stores `self db __FILE__, 0` in `.data` and runs `strncmp` on it at startup.

Here's the run in full, starting from i = 5:

```
Sully      (original, i=5)  -> writes Sully_5 (i=5), compiles, runs it
Sully_5    (child,    i=5)  -> n=4 -> writes Sully_4, compiles, runs it
Sully_4                     -> n=3 -> Sully_3
...
Sully_1                     -> n=0 -> Sully_0
Sully_0    (child,    i=0)  -> n=-1 -> stops, writes nothing
```

You end up with `Sully_0` to `Sully_5`, as both source files and executables.

### The commands Sully runs

The C version builds this command with `snprintf` and runs it with `system()`:

```
clang -Wall -Wextra -Werror Sully_N.c -o Sully_N && ./Sully_N
```

The ASM version runs this one:

```
nasm -f elf64 Sully_N.s -o Sully_N.o && gcc Sully_N.o -o Sully_N && rm -f Sully_N.o && ./Sully_N
```

The `rm -f` is there so the directory doesn't fill up with six extra `.o` files. Even so, the ASM run ends with **14** files rather than the subject's 13, because the `Sully.o` you created by hand when building the original is still there. I believe the 13 in the subject's ASM example was copied from the C example.

If any step fails (the file can't be opened, `fclose` fails, or the compile command returns non-zero), Sully returns 1 instead of carrying on.

## Notes specific to the ASM versions

A few things in the `.s` files look like noise but are needed:

- **`default rel` and `call foo wrt ..plt`.** Modern gcc links as PIE (position-independent executable) by default. Without RIP-relative addressing and calls through the PLT, you get linker errors like `relocation R_X86_64_32 against '.data' can not be used when making a PIE object`. With them, plain `gcc Foo.o -o Foo` works and you don't need `-no-pie`.
- **`section .note.GNU-stack noalloc noexec nowrite progbits`.** This tells the linker the stack doesn't need to be executable. Without it, recent binutils prints a "missing .note.GNU-stack section implies executable stack" warning on every link.
- **Stack alignment.** The System V ABI requires `rsp` to be 16-byte aligned at every `call`. When `main` starts, it's off by 8 because of the return address. The `push rbp` (plus `push rbx`/`sub rsp, 8` or `push r12` where needed) brings it back to a multiple of 16 before calling into libc. If you add or remove a push, check the alignment again. Misalignment tends to crash inside `printf` in confusing ways.
- **`xor eax, eax` before `printf`/`fprintf`/`sprintf`.** For variadic functions, `al` has to hold the number of vector registers used for arguments. We use none, so it's 0.
- **Callee-saved registers.** `rbx` holds the `FILE *` and `r12` holds the counter in Sully, because libc calls are allowed to clobber `rdi`, `rsi`, `rax` and so on. Both are pushed at the start of `main` and popped at the end.

## Quick checklist for an evaluation

- `grep -c '#define' C/Grace.c` gives 3
- `grep -c '^%define\|^%macro' ASM/Grace.s` gives 3
- `grep -n main C/Grace.c` only matches the `FT` macro line and the `SRC` template (which holds a copy of that line). There's no `main` outside a macro
- Colleen C: a comment inside `main`, a comment outside it, and `print_source` called from `main`
- Colleen ASM: a comment inside `main`, a comment at the top of the file, and `print_source` called from `main`
- None of the programs use `argc`/`argv` or open their own source
- All files use LF line endings and end with exactly one newline
