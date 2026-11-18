#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ft_ping_bonus.h"

double	tv_diff_ms(const struct timeval *a, const struct timeval *b)
{
	return ((double)(a->tv_sec - b->tv_sec) * 1000.0
		+ (double)(a->tv_usec - b->tv_usec) / 1000.0);
}

/* We can only time packets that are big enough to carry a timestamp. */
int	timing_enabled(const t_ping *ping)
{
	return (ping->opts.datalen >= sizeof(struct timeval));
}

/* "ft_ping: <message>" on stderr */
void	error_msg(const char *fmt, ...)
{
	va_list	ap;

	fflush(stdout);
	fprintf(stderr, "%s: ", PROGNAME);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

/* Bad command line: message + hint, exit 64 like inetutils/argp */
void	usage_error(const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%s: ", PROGNAME);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\nTry '%s --help' or '%s --usage' for more information.\n",
		PROGNAME, PROGNAME);
	exit(EXIT_USAGE);
}
