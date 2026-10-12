NAMES	= Colleen Grace Sully

AS		= nasm
ASFLAGS	= -f elf64
LD		= gcc

OBJS	= $(NAMES:=.o)

all: $(NAMES)

$(NAMES): %: %.o
	$(LD) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAMES) Grace_kid.s Sully_*

re: fclean all

.PHONY: all clean fclean re
