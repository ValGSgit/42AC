# 42AC — 42 Advanced Curriculum

This repo is where I keep my projects from the 42 Advanced curriculum, the part that comes after the Common Core. My Common Core projects are in [42CC](https://github.com/ValGSgit/42CC).

Each project has its own folder with its own README. The README explains how to build and test the project, and some folders also have a `docs/` folder for the longer explanations.

## Projects

| Project | Branch | Languages | What it's about | Status |
|---------|--------|-----------|-----------------|--------|
| [Dr_Quine](Dr_Quine/) | Security | C, x86-64 ASM | Self-replicating programs (quines). The intro to the virus/malware projects. | Done |
| [libasm](libasm/) | Kernel | x86-64 ASM (NASM) | Rewriting libc functions in assembly: calling convention, syscalls, errno, SIMD. | In progress (study guide written, code to do) |
| [ft_ping](ft_ping/) | Network | C | Recoding `ping` (inetutils-2.0 output) with raw sockets and hand-built ICMP packets. | Done (mandatory + bonus flags) |

I'll add more rows here as I finish projects.

## Layout

```
42AC/
├── README.md          <- you are here
├── .gitattributes     <- forces LF line endings (see below)
├── Dr_Quine/
│   ├── README.md
│   ├── docs/
│   ├── C/
│   └── ASM/
├── libasm/
│   ├── README.md
│   └── docs/
└── ft_ping/
    ├── README.md
    ├── Makefile           <- mandatory part; `make bonus` builds ft_ping_bonus/
    ├── includes/
    ├── srcs/
    ├── tests/
    └── ft_ping_bonus/     <- the full program with the bonus, builds on its own
        ├── Makefile
        ├── includes/
        ├── srcs/
        └── tests/
```

Every project folder stands on its own. You can `cd` into one and follow its README without reading anything else here.

When a project has a bonus, the bonus lives in its own `<project>_bonus/` folder inside the project folder. The mandatory folder only has mandatory code (no `#ifdef BONUS`, no bonus options), and the bonus folder is a complete copy with the bonus added, so each one builds and can be evaluated alone. The project's Makefile builds the mandatory part with `make` and the bonus folder with `make bonus`.

## Conventions

- **One folder per project**, named after the subject (`Dr_Quine`, `libasm`, ...).
- **Every project builds with its own Makefile(s)** and the usual rules (`all`, `clean`, `fclean`, `re`, plus `bonus` when there is one).
- **Bonus in `<project>_bonus/`**, with `_bonus` in the file names (`main_bonus.c`, `ft_ping_bonus.h`, ...).
- **Targets Linux x86-64** unless the subject says otherwise. On Windows I use WSL.
- **LF line endings everywhere.** `.gitattributes` enforces this. Some projects (the quines especially) compare files byte-for-byte, and a Windows checkout with CRLF breaks them silently. If you cloned the repo before `.gitattributes` existed, run `git add --renormalize .` or just re-clone.

## A note if you're a 42 student

Feel free to read the code and the docs to understand how things work, that's why I wrote the docs. Don't copy-paste it for your own evaluation, though: you'll get asked to explain every line, and the projects are way more fun when you figure them out yourself.
