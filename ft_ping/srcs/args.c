#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ft_ping.h"

/*
 * Long-only options get values above the char range so they can't clash
 * with a short option letter.
 */
enum
{
	ARG_USAGE = 256,
	ARG_HELP
};

/*
 * '?' is deliberately not in here. getopt already returns '?' for unknown
 * options, so -? shows up as an "unknown" option with optopt == '?'.
 * None of these options takes an argument.
 */
static const char	g_shortopts[] = "vV";

static const struct option	g_longopts[] = {
	{"verbose", no_argument, NULL, 'v'},
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
		"  -v, --verbose              verbose output\n"
		"\n"
		"  -?, --help                 give this help list\n"
		"      --usage                give a short usage message\n"
		"  -V, --version              print program version\n",
		PROGNAME);
}

static void	print_usage(void)
{
	printf("Usage: %s [-v?V] [--verbose] [--help] [--usage] [--version]\n"
		"            HOST ...\n", PROGNAME);
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

	memset(opts, 0, sizeof(*opts));
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
	return (optind);
}
