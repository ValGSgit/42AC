# **************************************************************************** #
#  ft_ping                                                                     #
#                                                                              #
#  Required rules : all  $(NAME)  bonus  clean  fclean  re                     #
#  Dev helpers    : run  valgrind  relink-check  help                          #
#                                                                              #
#  all / re build the mandatory part from this folder. `make bonus` builds     #
#  the full program in ft_ping_bonus/ with that folder's own Makefile, so      #
#  the bonus binary is ft_ping_bonus/ft_ping and never overwrites this one.    #
#  clean / fclean clean both parts.                                            #
#                                                                              #
#  Sources live in srcs/, the header in includes/. Objects go to obj/ and      #
#  carry .d files, so touching the header rebuilds what includes it.           #
# **************************************************************************** #

NAME        = ft_ping

CC          = gcc
CFLAGS      = -Wall -Wextra -Werror
CPPFLAGS    = -Iincludes -MMD -MP

# make re DEBUG=1 -> debug info + no optimisation, for gdb
ifeq ($(DEBUG), 1)
CFLAGS      += -g3 -O0
endif

SRC_DIR     = srcs
OBJ_DIR     = obj
BONUS_DIR   = ft_ping_bonus

SRC         = main.c \
              args.c \
              socket.c \
              packet.c \
              receive.c \
              icmp_print.c \
              stats.c \
              utils.c

OBJ         = $(SRC:%.c=$(OBJ_DIR)/%.o)
DEP         = $(OBJ:.o=.d)

# Arguments for `make run` / `make valgrind`, e.g. make run ARGS="-v 8.8.8.8"
# The mandatory ft_ping runs until Ctrl-C.
ARGS        = localhost

# ---------------------------------------------------------------------------- #
#  Required rules                                                              #
# ---------------------------------------------------------------------------- #

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

# `| $(OBJ_DIR)` is order-only: obj/ must exist, but its mtime changing
# (every time a .o is written) must not trigger rebuilds.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Always hands over to the bonus Makefile, which decides on its own whether
# anything needs rebuilding. A second `make bonus` does nothing.
bonus:
	@$(MAKE) --no-print-directory -C $(BONUS_DIR)

clean:
	rm -rf $(OBJ_DIR)
	@$(MAKE) --no-print-directory -C $(BONUS_DIR) clean

fclean: clean
	rm -f $(NAME)
	@$(MAKE) --no-print-directory -C $(BONUS_DIR) fclean

re: fclean all

-include $(DEP)

# ---------------------------------------------------------------------------- #
#  Dev helpers (raw sockets need root, hence sudo)                             #
# ---------------------------------------------------------------------------- #

run: $(NAME)
	sudo ./$(NAME) $(ARGS)

valgrind: $(NAME)
	sudo valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes \
		./$(NAME) $(ARGS)

# `bonus` always runs a sub-make, so `make -q bonus` can't answer. Ask the
# bonus Makefile directly instead (only if the bonus was built).
relink-check:
	@$(MAKE) --no-print-directory -q all && echo "all:   OK, nothing to relink" \
		|| { echo "all:   KO, would rebuild"; exit 1; }
	@if [ -f $(BONUS_DIR)/$(NAME) ]; then \
		$(MAKE) --no-print-directory -C $(BONUS_DIR) -q all \
		&& echo "bonus: OK, nothing to relink" \
		|| { echo "bonus: KO, would rebuild"; exit 1; }; fi

help:
	@echo "make / clean / fclean / re   the usual (mandatory part)"
	@echo "make bonus                   build $(BONUS_DIR)/$(NAME), all options"
	@echo "make run [ARGS=...]          sudo ./$(NAME) ARGS (default: $(ARGS))"
	@echo "make valgrind [ARGS=...]     same, under valgrind"
	@echo "make relink-check            verify nothing rebuilds needlessly"
	@echo "DEBUG=1                      debug build (use with re)"

.PHONY: all bonus clean fclean re run valgrind relink-check help
