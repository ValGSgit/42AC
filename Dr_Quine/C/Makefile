NAMES	= Colleen Grace Sully

CC		= clang
CFLAGS	= -Wall -Wextra -Werror

OBJS	= $(NAMES:=.o)

all: $(NAMES)

$(NAMES): %: %.o
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAMES) Grace_kid.c Sully_*

re: fclean all

.PHONY: all clean fclean re
