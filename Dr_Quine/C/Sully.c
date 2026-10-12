int i = 5;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	const char *s = "int i = %5$d;%1$c#include <stdio.h>%1$c#include <stdlib.h>%1$c#include <string.h>%1$c%1$cint main(void)%1$c{%1$c%4$cconst char *s = %2$c%3$s%2$c;%1$c%4$cchar file[64];%1$c%4$cchar cmd[256];%1$c%4$cFILE *f;%1$c%4$cint n;%1$c%1$c%4$cn = i;%1$c%4$cif (strncmp(__FILE__, %2$cSully_%2$c, 6) == 0)%1$c%4$c%4$cn--;%1$c%4$cif (n < 0)%1$c%4$c%4$creturn (0);%1$c%4$csnprintf(file, sizeof(file), %2$cSully_%%d.c%2$c, n);%1$c%4$cf = fopen(file, %2$cw%2$c);%1$c%4$cif (!f)%1$c%4$c%4$creturn (1);%1$c%4$cfprintf(f, s, 10, 34, s, 9, n);%1$c%4$cif (fclose(f) != 0)%1$c%4$c%4$creturn (1);%1$c%4$csnprintf(cmd, sizeof(cmd), %2$cclang -Wall -Wextra -Werror %%1$s -o Sully_%%2$d && ./Sully_%%2$d%2$c, file, n);%1$c%4$cif (system(cmd) != 0)%1$c%4$c%4$creturn (1);%1$c%4$creturn (0);%1$c}%1$c";
	char file[64];
	char cmd[256];
	FILE *f;
	int n;

	n = i;
	if (strncmp(__FILE__, "Sully_", 6) == 0)
		n--;
	if (n < 0)
		return (0);
	snprintf(file, sizeof(file), "Sully_%d.c", n);
	f = fopen(file, "w");
	if (!f)
		return (1);
	fprintf(f, s, 10, 34, s, 9, n);
	if (fclose(f) != 0)
		return (1);
	snprintf(cmd, sizeof(cmd), "clang -Wall -Wextra -Werror %1$s -o Sully_%2$d && ./Sully_%2$d", file, n);
	if (system(cmd) != 0)
		return (1);
	return (0);
}
