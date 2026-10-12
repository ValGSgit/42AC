# 42AC — 42 Advanced Curriculum

This repo is where I keep my projects from the 42 Advanced curriculum, the part that comes after the Common Core. My Common Core projects are in [42CC](https://github.com/ValGSgit/42CC).

Each project has its own folder with its own README. The README explains how to build and test the project, and some folders also have a `docs/` folder for the longer explanations.

## Projects

| Project | Branch | Languages | What it's about | Status |
|---------|--------|-----------|-----------------|--------|
| [Dr_Quine](Dr_Quine/) | Security | C, x86-64 ASM | Self-replicating programs (quines). The intro to the virus/malware projects. | Done |

I'll add more rows here as I finish projects.

## Layout

```
42AC/
├── README.md          <- you are here
├── .gitattributes     <- forces LF line endings (see below)
└── Dr_Quine/
    ├── README.md
    ├── docs/
    ├── C/
    └── ASM/
```

Every project folder stands on its own. You can `cd` into one and follow its README without reading anything else here.

## Conventions

- **One folder per project**, named after the subject (`Dr_Quine`, ...).
- **Every project builds with its own Makefile(s)** and the usual rules (`all`, `clean`, `fclean`, `re`).
- **Targets Linux x86-64** unless the subject says otherwise. On Windows I use WSL.
- **LF line endings everywhere.** `.gitattributes` enforces this. Some projects (the quines especially) compare files byte-for-byte, and a Windows checkout with CRLF breaks them silently. If you cloned the repo before `.gitattributes` existed, run `git add --renormalize .` or just re-clone.

## A note if you're a 42 student

Feel free to read the code and the docs to understand how things work, that's why I wrote the docs. Don't copy-paste it for your own evaluation, though: you'll get asked to explain every line, and the projects are way more fun when you figure them out yourself.
