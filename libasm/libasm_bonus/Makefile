# **************************************************************************** #
#  libasm (bonus)                                                              #
#                                                                              #
#  Required rules : all  $(NAME)  clean  fclean  re                            #
#  Dev helpers    : test  valgrind  gdb  strace  syms  relink-check  help      #
#                                                                              #
#  The full library: the mandatory functions AND the bonus ones, all in this   #
#  folder, all with a _bonus suffix (ft_strlen_bonus.s, ft_list_size_bonus.s,  #
#  ...). It builds on its own from here (nothing comes from ../), or from the  #
#  parent folder with `make bonus`. The library is called libasm.a here too.   #
#                                                                              #
#  Sources are picked up automatically: every ft_*_bonus.s goes in.            #
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

SRC         = $(wildcard ft_*_bonus.s)
OBJ         = $(SRC:%.s=$(OBJ_DIR)/%.o)

# The test program: main_bonus.c, linked against this folder's library.
TEST_SRC    = main_bonus.c
HEADER      = libasm_bonus.h
TEST_BIN    = run_tests_bonus

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

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_BIN)

re: fclean all

# ---------------------------------------------------------------------------- #
#  Testing                                                                     #
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

# Fails if `make` would do anything right after a build.
relink-check:
	@$(MAKE) --no-print-directory -q all && echo "OK, nothing to relink" \
		|| { echo "KO, would rebuild"; exit 1; }

help:
	@echo "make                   build $(NAME) (mandatory + bonus functions)"
	@echo "make clean|fclean|re   the usual"
	@echo "make test [ARGS=...]   build $(TEST_SRC) against the lib and run it"
	@echo "make valgrind|gdb|strace       run the tests under a tool"
	@echo "make syms              list the library's symbols"
	@echo "make relink-check      verify nothing rebuilds needlessly"
	@echo "DEBUG=1                add debug info (use with re)"

.PHONY: all clean fclean re test valgrind gdb strace syms relink-check help
