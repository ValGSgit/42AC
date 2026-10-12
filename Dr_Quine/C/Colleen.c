#include <stdio.h>

/*
	This program prints its own source code when run.
*/

void print_source(const char *s)
{
	printf(s, 10, 34, s, 9);
}

int main(void)
{
	/*
		Call the function that prints the source code.
	*/
	print_source("#include <stdio.h>%1$c%1$c/*%1$c%4$cThis program prints its own source code when run.%1$c*/%1$c%1$cvoid print_source(const char *s)%1$c{%1$c%4$cprintf(s, 10, 34, s, 9);%1$c}%1$c%1$cint main(void)%1$c{%1$c%4$c/*%1$c%4$c%4$cCall the function that prints the source code.%1$c%4$c*/%1$c%4$cprint_source(%2$c%3$s%2$c);%1$c%4$creturn (0);%1$c}%1$c");
	return (0);
}
