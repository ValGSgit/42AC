#include <stdio.h>
/*
	This program writes its own source code into Grace_kid.c.
*/
#define KID "Grace_kid.c"
#define SRC "#include <stdio.h>%1$c/*%1$c%4$cThis program writes its own source code into Grace_kid.c.%1$c*/%1$c#define KID %2$cGrace_kid.c%2$c%1$c#define SRC %2$c%3$s%2$c%1$c#define FT(x) int main(void) { FILE *f = fopen(KID, %2$cw%2$c); if (!f) return (1); fprintf(f, x, 10, 34, x, 9); return (fclose(f) != 0); }%1$cFT(SRC)%1$c"
#define FT(x) int main(void) { FILE *f = fopen(KID, "w"); if (!f) return (1); fprintf(f, x, 10, 34, x, 9); return (fclose(f) != 0); }
FT(SRC)
