# **************************************************************************** #
#  ft_ping                                                                     #
#                                                                              #
#  Required rules : all  $(NAME)  clean  fclean  re                            #
#  Dev helpers    : run  valgrind  relink-check  help                          #
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

# Arguments for `make run` / `make valgrind`, e.g. make run ARGS="-v -c 3 8.8.8.8"
ARGS        = -c 4 localhost

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

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

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

relink-check:
	@$(MAKE) --no-print-directory -q all && echo "OK, nothing to relink" \
		|| { echo "KO, would rebuild"; exit 1; }

help:
	@echo "make / clean / fclean / re   the usual"
	@echo "make run [ARGS=...]          sudo ./$(NAME) ARGS (default: $(ARGS))"
	@echo "make valgrind [ARGS=...]     same, under valgrind"
	@echo "make relink-check            verify nothing rebuilds needlessly"
	@echo "DEBUG=1                      debug build (use with re)"

.PHONY: all clean fclean re run valgrind relink-check help
