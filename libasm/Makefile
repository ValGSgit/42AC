# **************************************************************************** #
#  libasm                                                                      #
#                                                                              #
#  Required rules : all  $(NAME)  bonus  clean  fclean  re                     #
#  Dev helpers    : test  valgrind  gdb  strace  syms  try  relink-check  help #
#                                                                              #
#  This folder is the mandatory part only. Sources are picked up               #
#  automatically: every ft_*.s here goes into libasm.a. Add a file, run make,  #
#  done.                                                                       #
#                                                                              #
#  The bonus lives in libasm_bonus/, with its own Makefile. `make bonus` runs  #
#  it, and it builds libasm_bonus/libasm.a with the mandatory AND the bonus    #
#  functions. It never touches the libasm.a in this folder.                    #
#  clean / fclean clean both parts.                                            #
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
BONUS_DIR   = libasm_bonus

SRC         = $(wildcard ft_*.s)
OBJ         = $(SRC:%.s=$(OBJ_DIR)/%.o)

# The test program: main.c, linked against the library. The bonus tests are
# in libasm_bonus/main_bonus.c.
TEST_SRC    = main.c
HEADER      = libasm.h
TEST_BIN    = run_tests

# Arguments passed to the test program, e.g. make test ARGS="hello world"
ARGS        =

# ---------------------------------------------------------------------------- #
#  Required rules                                                              #
# ---------------------------------------------------------------------------- #

all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $(NAME) $(OBJ)

# One .o per .s, in obj/. The `| $(OBJ_DIR)` is an order-only prerequisite:
# the folder must exist, but its timestamp changing doesn't trigger rebuilds.
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Always hands over to the bonus Makefile, which decides on its own whether
# anything needs rebuilding. A second `make bonus` does nothing.
bonus:
	@$(MAKE) --no-print-directory -C $(BONUS_DIR)

clean:
	rm -rf $(OBJ_DIR) try_bin
	@$(MAKE) --no-print-directory -C $(BONUS_DIR) clean

fclean: clean
	rm -f $(NAME) $(TEST_BIN)
	@$(MAKE) --no-print-directory -C $(BONUS_DIR) fclean

re: fclean all

# ---------------------------------------------------------------------------- #
#  Testing (for the bonus: same rules, run from libasm_bonus/)                 #
# ---------------------------------------------------------------------------- #

# -lasm must come AFTER the C file: the linker only takes from a static library
# the symbols that are already needed at that point.
$(TEST_BIN): $(TEST_SRC) $(HEADER) $(NAME)
	$(CC) $(CFLAGS) $(TEST_SRC) -L. -lasm -o $@

test: $(TEST_BIN)
	./$(TEST_BIN) $(ARGS)

valgrind: $(TEST_BIN)
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes \
		--error-exitcode=1 ./$(TEST_BIN) $(ARGS)

gdb: $(TEST_BIN)
	gdb -q -ex "set disassembly-flavor intel" --args ./$(TEST_BIN) $(ARGS)

strace: $(TEST_BIN)
	strace -e trace=read,write ./$(TEST_BIN) $(ARGS)

# Which symbols the library defines (T) and needs from elsewhere (U).
syms: $(NAME)
	nm $(NAME)

# Fails if `make` or `make bonus` would do anything right after a build.
# `bonus` always runs a sub-make, so `make -q bonus` can't answer. Ask the
# bonus Makefile directly instead (only if the bonus was built).
relink-check:
	@$(MAKE) --no-print-directory -q all && echo "all:   OK, nothing to relink" \
		|| { echo "all:   KO, would rebuild"; exit 1; }
	@if [ -f $(BONUS_DIR)/$(NAME) ]; then \
		$(MAKE) --no-print-directory -C $(BONUS_DIR) -q all \
		&& echo "bonus: OK, nothing to relink" \
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
	@echo "make                   build $(NAME) (mandatory functions)"
	@echo "make bonus             build $(BONUS_DIR)/$(NAME) (mandatory + bonus)"
	@echo "make clean|fclean|re   the usual (clean/fclean: both parts)"
	@echo "make test [ARGS=...]   build $(TEST_SRC) against the lib and run it"
	@echo "make valgrind|gdb|strace       run the tests under a tool"
	@echo "make syms              list the library's symbols"
	@echo "make relink-check      verify nothing rebuilds needlessly"
	@echo "make try S=x.s [C=x.c] [RUN=tool]   build/run a single scratch file"
	@echo "DEBUG=1                add debug info (use with re)"
	@echo "Bonus tests: cd $(BONUS_DIR) && make test (same rules there)"

.PHONY: all bonus clean fclean re test valgrind gdb strace syms \
	relink-check try help
