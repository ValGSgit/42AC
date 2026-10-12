# Dr_Quine

This is my take on the Dr_Quine project: three self-replicating programs (Colleen, Grace and Sully), each written twice, once in C and once in x86-64 assembly (NASM, Linux).

None of these programs open their own source file. Each one carries its source inside itself as a string and rebuilds the file from that string. If you want to know how that works, read [docs/HOW_IT_WORKS.md](docs/HOW_IT_WORKS.md). This file only covers building, running and testing.

`Colleen.s` and `Sully.s` are commented line by line. `Grace.s` is allowed only one comment, so its commented version is in [docs/GRACE_ASM_ANNOTATED.md](docs/GRACE_ASM_ANNOTATED.md).

## Layout

```
Dr_Quine/
├── C/
│   ├── Colleen.c
│   ├── Grace.c
│   ├── Sully.c
│   └── Makefile
├── ASM/
│   ├── Colleen.s
│   ├── Grace.s
│   ├── Sully.s
│   └── Makefile
└── docs/
    ├── HOW_IT_WORKS.md
    └── GRACE_ASM_ANNOTATED.md
```

## What you need

- Linux on x86-64. WSL works fine.
- `clang` for the C part. The C Makefile uses it, and so does the C Sully when it compiles its children.
- `nasm` and `gcc` for the assembly part. gcc is only used as the linker, so we get libc (`printf`, `fopen`, `system`, ...) for free.

On Debian/Ubuntu: `sudo apt install clang nasm gcc make`

## Building

Each folder has its own Makefile with the usual rules:

```sh
cd C        # or ASM
make        # builds Colleen, Grace and Sully
make clean  # removes the .o files
make fclean # removes the binaries plus everything the programs generated (Grace_kid.*, Sully_*)
make re     # fclean + all
```

Running `make` twice in a row does nothing the second time. It only rebuilds what changed.

## What each program does

| Program | What it does |
|---------|--------------|
| **Colleen** | Prints its own source code to stdout. |
| **Grace** | Writes its own source code into `Grace_kid.c` / `Grace_kid.s`. No functions in the C version, just three `#define`s and a macro call. |
| **Sully** | Writes a copy of itself with the counter decremented into `Sully_X.c` / `Sully_X.s`, compiles it, runs it, and so on until the counter would go below 0. |

## Checking it yourself

These are the same commands the subject uses. Every `diff` should print nothing except where noted.

### C

```sh
cd C

clang -Wall -Wextra -Werror -o Colleen Colleen.c; ./Colleen > tmp_Colleen; diff tmp_Colleen Colleen.c

clang -Wall -Wextra -Werror -o Grace Grace.c; ./Grace; diff Grace.c Grace_kid.c

mkdir test && cd test
clang -Wall -Wextra -Werror ../Sully.c -o Sully; ./Sully
ls -al | grep Sully | wc -l      # 13
diff ../Sully.c Sully_0.c        # only line 1 differs: int i = 5; vs int i = 0;
diff Sully_3.c Sully_2.c         # only line 1 differs: int i = 3; vs int i = 2;
```

### Assembly

```sh
cd ASM

nasm -f elf64 Colleen.s -o Colleen.o && gcc Colleen.o -o Colleen
./Colleen > tmp_Colleen; diff tmp_Colleen Colleen.s

nasm -f elf64 Grace.s -o Grace.o && gcc Grace.o -o Grace
rm -f Grace_kid.s; ./Grace; diff Grace_kid.s Grace.s

mkdir test && cd test
nasm -f elf64 ../Sully.s -o Sully.o && gcc Sully.o -o Sully
./Sully
ls -al | grep Sully | wc -l      # 14, see the note below
diff ../Sully.s Sully_0.s        # only line 1 differs: i equ 5 vs i equ 0
diff Sully_3.s Sully_2.s         # only line 1 differs: i equ 3 vs i equ 2
```

## Things worth knowing before an evaluation

- **The ASM Sully leaves 14 files, not 13.** The extra one is `Sully.o`, which comes from the subject's own `nasm ... -o Sully.o` line. The children clean up their own `.o` files, so what's left is `Sully`, `Sully.o`, `Sully_0.s` to `Sully_5.s` and `Sully_0` to `Sully_5`. The 13 in the subject's ASM example looks like it was copied over from the C example.
- **Line 1 of `Sully.s` is `i equ 5`.** The subject's example shows a comment (`; i = 5`) there. I made it a real assemble-time constant instead, because the program actually uses that number. The diffs still only change line 1.
- **How Sully knows whether it's the original or a child:** it checks whether `__FILE__` starts with `Sully_`. `__FILE__` is filled in by the compiler or assembler at build time, so this isn't reading the source and it doesn't touch argv. The full explanation is in the docs.
- **Line endings matter.** All the sources use LF only. If your editor or git converts them to CRLF, every diff will fail. The repo's `.gitattributes` forces LF, but watch out if you copy the files somewhere else.
- **Don't edit the source by hand without reading the docs first.** Each file contains an encoded copy of itself, and changing one without the other breaks the quine. The docs explain how to keep them in sync.
