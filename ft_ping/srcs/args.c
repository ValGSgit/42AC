#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "ft_ping.h"

/*
 * Long-only options get values above the char range so they can't clash
 * with a short option letter.
 */
enum
{
	ARG_TTL = 256,
	ARG_USAGE,
	ARG_HELP
};

/*
 * '?' is deliberately not in here. getopt already returns '?' for unknown
 * options, so -? shows up as an "unknown" option with optopt == '?'.
 */
static const char	g_shortopts[] = ":vqfc:i:w:W:s:p:T:l:V";

static const struct option	g_longopts[] = {
	{"verbose", no_argument, NULL, 'v'},
	{"quiet", no_argument, NULL, 'q'},
	{"flood", no_argument, NULL, 'f'},
	{"count", required_argument, NULL, 'c'},
	{"interval", required_argument, NULL, 'i'},
	{"timeout", required_argument, NULL, 'w'},
	{"linger", required_argument, NULL, 'W'},
	{"size", required_argument, NULL, 's'},
	{"pattern", required_argument, NULL, 'p'},
	{"tos", required_argument, NULL, 'T'},
	{"preload", required_argument, NULL, 'l'},
	{"ttl", required_argument, NULL, ARG_TTL},
	{"help", no_argument, NULL, ARG_HELP},
	{"usage", no_argument, NULL, ARG_USAGE},
	{"version", no_argument, NULL, 'V'},
	{NULL, 0, NULL, 0}
};

static void	print_help(void)
{
	printf("Usage: %s [OPTION...] HOST ...\n"
		"Send ICMP ECHO_REQUEST packets to network hosts.\n"
		"\n"
		" Options valid for all request types:\n"
		"\n"
		"  -c, --count=NUMBER         stop after sending NUMBER packets\n"
		"  -i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n"
		"      --ttl=N                specify N as time-to-live\n"
		"  -T, --tos=NUM              set type of service (TOS) to NUM\n"
		"  -v, --verbose              verbose output\n"
		"  -w, --timeout=N            stop after N seconds\n"
		"  -W, --linger=N             number of seconds to wait for response\n"
		"\n"
		" Options valid for --echo requests:\n"
		"\n"
		"  -f, --flood                flood ping (root only)\n"
		"  -l, --preload=NUMBER       send NUMBER packets as fast as possible before\n"
		"                             falling into normal mode of behavior (root only)\n"
		"  -p, --pattern=PATTERN      fill ICMP packet with given pattern (hex)\n"
		"  -q, --quiet                quiet output\n"
		"  -s, --size=NUMBER          send NUMBER data octets\n"
		"\n"
		"  -?, --help                 give this help list\n"
		"      --usage                give a short usage message\n"
		"  -V, --version              print program version\n"
		"\n"
		"Mandatory or optional arguments to long options are also mandatory or optional\n"
		"for any corresponding short options.\n"
		"\n"
		"Options marked with (root only) are available only to superuser.\n",
		PROGNAME);
}

static void	print_usage(void)
{
	printf("Usage: %s [-vfq?V] [-c NUMBER] [-i NUMBER] [-T NUM] [-w N] [-W N]\n"
		"            [-l NUMBER] [-p PATTERN] [-s NUMBER] [--count=NUMBER]\n"
		"            [--interval=NUMBER] [--ttl=N] [--tos=NUM] [--verbose]\n"
		"            [--timeout=N] [--linger=N] [--flood] [--preload=NUMBER]\n"
		"            [--pattern=PATTERN] [--quiet] [--size=NUMBER] [--help]\n"
		"            [--usage] [--version] HOST ...\n", PROGNAME);
}

/*
 * Same behaviour as ping_cvt_number() in inetutils: strtoul with base 0
 * (so 0x10 and 010 work), the whole string must be a number, then range
 * checks. Any error is fatal. Like the original, "-c -1" goes through
 * strtoul and becomes a huge count, which is basically "forever".
 */
static size_t	cvt_number(const char *arg, size_t maxval, int allow_zero)
{
	char		*end;
	unsigned long	n;

	n = strtoul(arg, &end, 0);
	if (*end)
	{
		error_msg("invalid value (`%s' near `%s')", arg, end);
		exit(EXIT_FAILURE);
	}
	if (n == 0 && !allow_zero)
	{
		error_msg("option value too small: %s", arg);
		exit(EXIT_FAILURE);
	}
	if (maxval && n > maxval)
	{
		error_msg("option value too big: %s", arg);
		exit(EXIT_FAILURE);
	}
	return (n);
}

static double	cvt_interval(const char *arg)
{
	char	*end;
	double	d;

	d = strtod(arg, &end);
	if (*end || end == arg || d < 0)
		usage_error("invalid value (`%s' near `%s')", arg, end);
	if (d < 0.2 && getuid() != 0)
	{
		error_msg("option value too small: %s", arg);
		exit(EXIT_FAILURE);
	}
	return (d);
}

static int	hexval(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	return (tolower((unsigned char)c) - 'a' + 10);
}

/* "-p ff00a" -> {0xff, 0x00, 0x0a}, max 16 bytes, like inetutils */
static void	parse_pattern(const char *arg, t_opts *opts)
{
	const char	*p;
	size_t		n;

	p = arg;
	n = 0;
	while (*p && n < MAX_PATTERN)
	{
		if (!isxdigit((unsigned char)p[0]))
		{
			error_msg("error in pattern near %s", p);
			exit(EXIT_FAILURE);
		}
		if (p[1] && isxdigit((unsigned char)p[1]))
		{
			opts->pattern[n++] = (unsigned char)(hexval(p[0]) * 16
					+ hexval(p[1]));
			p += 2;
		}
		else if (p[1] == '\0')
		{
			opts->pattern[n++] = (unsigned char)hexval(p[0]);
			p += 1;
		}
		else
		{
			error_msg("error in pattern near %s", p + 1);
			exit(EXIT_FAILURE);
		}
	}
	opts->pattern_len = n;
}

static void	set_defaults(t_opts *opts)
{
	memset(opts, 0, sizeof(*opts));
	opts->interval = DEFAULT_INTERVAL;
	opts->linger = DEFAULT_LINGER;
	opts->datalen = DEFAULT_DATALEN;
	opts->tos = -1;
}

static void	print_help_and_exit(void)
{
	print_help();
	exit(EXIT_SUCCESS);
}

/* optopt is 0 for an unknown long option, the bad char for a short one */
static void	handle_bad_option(char **argv)
{
	if (optopt == '?')
		print_help_and_exit();
	if (optopt == 0)
		usage_error("unrecognized option '%s'", argv[optind - 1]);
	usage_error("invalid option -- '%c'", optopt);
}

static void	handle_option(int c, t_opts *opts, char **argv)
{
	if (c == 'v')
		opts->flags |= OPT_VERBOSE;
	else if (c == 'q')
		opts->flags |= OPT_QUIET;
	else if (c == 'f')
		opts->flags |= OPT_FLOOD;
	else if (c == 'c')
		opts->count = cvt_number(optarg, 0, 1);
	else if (c == 'i')
	{
		opts->interval = cvt_interval(optarg);
		opts->flags |= OPT_INTERVAL;
	}
	else if (c == 'w')
		opts->timeout = (long)cvt_number(optarg, INT_MAX, 0);
	else if (c == 'W')
		opts->linger = (long)cvt_number(optarg, INT_MAX, 0);
	else if (c == 's')
		opts->datalen = cvt_number(optarg, MAX_DATALEN, 1);
	else if (c == 'p')
		parse_pattern(optarg, opts);
	else if (c == 'T')
		opts->tos = (int)cvt_number(optarg, 255, 1);
	else if (c == 'l')
		opts->preload = cvt_number(optarg, INT_MAX, 1);
	else if (c == ARG_TTL)
		opts->ttl = (int)cvt_number(optarg, 255, 0);
	else if (c == 'V')
	{
		printf("%s %s (reimplementation of ping from GNU inetutils 2.0)\n",
			PROGNAME, VERSION);
		exit(EXIT_SUCCESS);
	}
	else if (c == ARG_USAGE)
	{
		print_usage();
		exit(EXIT_SUCCESS);
	}
	else if (c == ':' && strncmp(argv[optind - 1], "--", 2) == 0)
		usage_error("option '%s' requires an argument", argv[optind - 1]);
	else if (c == ':')
		usage_error("option requires an argument -- '%c'", optopt);
	else if (c == ARG_HELP)
		print_help_and_exit();
	else if (c == '?')
		handle_bad_option(argv);
}

/*
 * Returns the index of the first host argument in argv.
 */
int	parse_args(int argc, char **argv, t_opts *opts)
{
	int	c;

	set_defaults(opts);
	opterr = 0;
	while (1)
	{
		c = getopt_long(argc, argv, g_shortopts, g_longopts, NULL);
		if (c == -1)
			break ;
		handle_option(c, opts, argv);
	}
	if (optind >= argc)
		usage_error("missing host operand");
	if ((opts->flags & OPT_FLOOD) && (opts->flags & OPT_INTERVAL))
	{
		error_msg("-f and -i incompatible options");
		exit(EXIT_FAILURE);
	}
	if (opts->flags & OPT_FLOOD)
		opts->interval = FLOOD_INTERVAL;
	return (optind);
}
