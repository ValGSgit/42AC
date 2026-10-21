# **************************************************************************** #
#  libasm                                                                      #
#                                                                              #
#  Required rules : all  $(NAME)  bonus  clean  fclean  re                     #
#  Dev helpers    : test  test_bonus  valgrind  gdb  strace  syms  try         #
#                   relink-check  help                                         #
#                                                                              #
#  Sources are picked up automatically: every ft_*.s in this folder is part    #
#  of the library, and every ft_*_bonus.s is part of the bonus. Add a file,    #
#  run make, done.                                                             #
# **************************************************************************** #

NAME        = libasm.a

AS          = nasm
ASFLAGS     = -f elf64
CC          = gcc
CFLAGS      = -Wall -Wextra -Werror
AR          = ar rcs

# make DEBUG=1 ... adds debug info for gdb. Switching it doesn't rebuild what's
# already built, so use: make re DEBUG=1
ifeq ($(DEBUG), 1)
ASFLAGS     += -g -F dwarf
CFLAGS      += -g
endif

OBJ_DIR     = obj

BONUS_SRC   = $(wildcard ft_*_bonus.s)
SRC         = $(filter-out $(BONUS_SRC), $(wildcard ft_*.s))
OBJ         = $(SRC:%.s=$(OBJ_DIR)/%.o)
BONUS_OBJ   = $(BONUS_SRC:%.s=$(OBJ_DIR)/%.o)

# `bonus` isn't a file, so a plain `bonus:` rule would re-run ar every time
# (= relink). This empty file records when the bonus objects were last
# archived, so make can tell whether anything changed since.
BONUS_STAMP = .bonus

# The test program: main.c, linked against the library.
# test_bonus is the same main.c compiled with -DBONUS, so you can put the bonus
# tests inside #ifdef BONUS ... #endif and keep a single main.
TEST_SRC    = main.c
HEADER      = libasm.h
TEST_BIN    = run_tests
TEST_BONUS  = run_tests_bonus

# Arguments passed to the test program, e.g. make test ARGS="hello world"
ARGS        =

# ---------------------------------------------------------------------------- #
#  Required rules                                                              #
# ---------------------------------------------------------------------------- #

all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $(NAME) $(OBJ)

bonus: $(BONUS_STAMP)

$(BONUS_STAMP): $(OBJ) $(BONUS_OBJ)
	$(AR) $(NAME) $(OBJ) $(BONUS_OBJ)
	@touch $(BONUS_STAMP)

# One .o per .s, in obj/. The `| $(OBJ_DIR)` is an order-only prerequisite:
# the folder must exist, but its timestamp changing doesn't trigger rebuilds.
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) try_bin

fclean: clean
	rm -f $(NAME) $(BONUS_STAMP) $(TEST_BIN) $(TEST_BONUS)

re: fclean all

# ---------------------------------------------------------------------------- #
#  Testing                                                                     #
# ---------------------------------------------------------------------------- #

# -lasm must come AFTER the C file: the linker only takes from a static library
# the symbols that are already needed at that point.
$(TEST_BIN): $(TEST_SRC) $(HEADER) $(NAME)
	$(CC) $(CFLAGS) $(TEST_SRC) -L. -lasm -o $@

$(TEST_BONUS): $(TEST_SRC) $(HEADER) $(BONUS_STAMP)
	$(CC) $(CFLAGS) -DBONUS $(TEST_SRC) -L. -lasm -o $@

test: $(TEST_BIN)
	./$(TEST_BIN) $(ARGS)

test_bonus: $(TEST_BONUS)
	./$(TEST_BONUS) $(ARGS)

# B=1 runs the bonus build instead, e.g. make valgrind B=1
RUN_BIN = $(if $(B),$(TEST_BONUS),$(TEST_BIN))

valgrind: $(RUN_BIN)
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes \
		--error-exitcode=1 ./$(RUN_BIN) $(ARGS)

gdb: $(RUN_BIN)
	gdb -q -ex "set disassembly-flavor intel" --args ./$(RUN_BIN) $(ARGS)

strace: $(RUN_BIN)
	strace -e trace=read,write ./$(RUN_BIN) $(ARGS)

# Which symbols the library defines (T) and needs from elsewhere (U).
syms: $(NAME)
	nm $(NAME)

# Fails if `make` or `make bonus` would do anything right after a build.
relink-check:
	@$(MAKE) --no-print-directory -q all && echo "all:   OK, nothing to relink" \
		|| { echo "all:   KO, would rebuild"; exit 1; }
	@if [ -f $(BONUS_STAMP) ]; then \
		$(MAKE) --no-print-directory -q bonus && echo "bonus: OK, nothing to relink" \
		|| { echo "bonus: KO, would rebuild"; exit 1; }; fi

# ---------------------------------------------------------------------------- #
#  Scratch: assemble and run any one .s with a C driver, outside the library.  #
#  For the exercises in docs/01 and for trying ideas.                          #
#                                                                              #
#    make try S=scratch/add.s                 driver defaults to scratch/add.c #
#    make try S=scratch/add.s C=scratch/t.c   explicit driver                  #
#    make try S=scratch/add.s RUN="gdb -q"    run it under a tool              #
# ---------------------------------------------------------------------------- #

C   ?= $(S:.s=.c)
RUN ?=

try:
	@test -n "$(S)" || { echo "usage: make try S=file.s [C=driver.c] [RUN=tool]"; exit 1; }
	@mkdir -p try_bin
	$(AS) $(ASFLAGS) -g -F dwarf $(S) -o try_bin/try.o
	$(CC) $(CFLAGS) -g $(C) try_bin/try.o -o try_bin/try
	$(RUN) ./try_bin/try $(ARGS)

help:
	@echo "make / make bonus      build $(NAME) (bonus adds *_bonus.s)"
	@echo "make clean|fclean|re   the usual"
	@echo "make test [ARGS=...]   build $(TEST_SRC) against the lib and run it"
	@echo "make test_bonus        same, compiled with -DBONUS against the bonus lib"
	@echo "make valgrind|gdb|strace [B=1]   run the tests under a tool"
	@echo "make syms              list the library's symbols"
	@echo "make relink-check      verify nothing rebuilds needlessly"
	@echo "make try S=x.s [C=x.c] [RUN=tool]   build/run a single scratch file"
	@echo "DEBUG=1                add debug info (use with re)"

.PHONY: all bonus clean fclean re test test_bonus valgrind gdb strace syms \
	relink-check try help
