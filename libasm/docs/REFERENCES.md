# References

Grouped by what you'll need them for. The starred ones are the ones worth actually reading.

## Learning assembly

- ★ Ray Toal, *NASM Tutorial* (LMU) — short, practical, 64-bit Linux, NASM. https://cs.lmu.edu/~ray/notes/nasmtutorial/
- ★ Ed Jorgensen, *x86-64 Assembly Language Programming with Ubuntu* (free PDF, NASM, very beginner-friendly). http://www.egr.unlv.edu/~ed/assembly64.pdf
- *NASM manual* — syntax, directives (`section`, `global`, `extern`, `default rel`, `%define`, `struc`), `wrt ..plt`. https://www.nasm.us/doc/
  - Section "Writing 64-bit Code (Unix, Win64)" covers `default rel` and PIC.
  - Section "elf Extensions to the GLOBAL Directive" / "Position-Independent Code" covers `wrt ..plt` and `wrt ..got`.
- Compiler Explorer — see what C compiles to. Use `-O1 -masm=intel`. https://godbolt.org

## Instruction reference

- ★ Félix Cloutier's x86 reference (the Intel manual, one instruction per page, searchable). https://www.felixcloutier.com/x86/
  - Read the pages for `mul`, `movzx`, `rep stos`, `pcmpeqb`, `pmovmskb`, `bsf`, `tzcnt`.
- Intel 64 and IA-32 Architectures Software Developer's Manuals (the source of truth, thousands of pages). https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- Intel Intrinsics Guide — the C names for SIMD instructions; useful to understand what an SSE instruction does lane by lane. https://www.intel.com/content/www/us/en/docs/intrinsics-guide/
- Agner Fog's optimization manuals and instruction tables, if you want to understand *why* one approach is faster. https://www.agner.org/optimize/

## ABI

- ★ System V AMD64 ABI (psABI), the official spec. Chapter 3.2 "Function Calling Sequence" has the register table and the stack alignment rule. https://gitlab.com/x86-psABIs/x86-64-ABI
- Wikipedia, *x86 calling conventions* → "System V AMD64 ABI" — the one-paragraph summary. https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI
- Eli Bendersky, *Position Independent Code (PIC) in shared libraries* — what the PLT and GOT are and why they exist. https://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/

## Syscalls and errno

- `man 2 syscall` — the "Architecture calling conventions" table lists the registers for every architecture. Online: https://man7.org/linux/man-pages/man2/syscall.2.html
- `man 2 write`, `man 2 read` — return values and the list of errors. https://man7.org/linux/man-pages/man2/write.2.html
- `man 3 errno` — why it's thread-local. https://man7.org/linux/man-pages/man3/errno.3.html
- Linux x86-64 syscall table. https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
  - Locally: `/usr/include/x86_64-linux-gnu/asm/unistd_64.h`
- Error numbers locally: `/usr/include/asm-generic/errno-base.h`

## The functions

- `man 3 strlen`, `strcpy`, `strcmp`, `strdup`, `calloc`, `memchr` (man7.org has them all: https://man7.org/linux/man-pages/dir_section_3.html).

## SIMD / SWAR

- ★ Sean Eron Anderson, *Bit Twiddling Hacks*, "Determine if a word has a zero byte". https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
- Wikipedia, *SWAR*. https://en.wikipedia.org/wiki/SWAR
- glibc's own SSE2 `memchr` (read it *after* writing yours, to compare approaches): `sysdeps/x86_64/multiarch/memchr-sse2.S` in https://sourceware.org/git/?p=glibc.git

## Tools

- GDB quick reference: https://sourceware.org/gdb/current/onlinedocs/gdb/ (and `help` inside gdb).
- Valgrind Memcheck manual: https://valgrind.org/docs/manual/mc-manual.html
- GNU make manual, "Pattern Rules" and "Automatic Variables": https://www.gnu.org/software/make/manual/make.html

## History

- Kathleen Booth, from the foreword. https://en.wikipedia.org/wiki/Kathleen_Booth
