#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include "ft_ping.h"

volatile sig_atomic_t	g_stop = 0;

static void	on_sigint(int sig)
{
	(void)sig;
	g_stop = 1;
}

static void	setup_signals(void)
{
	struct sigaction	sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_sigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}

static struct timeval	seconds_to_tv(double s)
{
	struct timeval	tv;

	if (s < 0)
		s = 0;
	tv.tv_sec = (time_t)s;
	tv.tv_usec = (suseconds_t)((s - (double)tv.tv_sec) * 1000000.0);
	return (tv);
}

/*
 * The main loop, same idea as ping_run() in inetutils: select() on the
 * socket with a timeout equal to the time left until the next send.
 *  - socket readable  -> read a packet
 *  - timeout          -> send the next echo request
 * It runs until Ctrl-C.
 */
static void	ping_loop(t_ping *ping)
{
	struct timeval	last;
	struct timeval	now;
	struct timeval	tv;
	fd_set		fds;
	int		n;

	gettimeofday(&last, NULL);
	while (!g_stop)
	{
		gettimeofday(&now, NULL);
		tv = seconds_to_tv(INTERVAL - tv_diff_ms(&now, &last) / 1000.0);
		FD_ZERO(&fds);
		FD_SET(ping->sock, &fds);
		n = select(ping->sock + 1, &fds, NULL, NULL, &tv);
		if (n < 0)
		{
			if (errno == EINTR)
				continue ;
			error_msg("select: %s", strerror(errno));
			break ;
		}
		if (n > 0)
		{
			receive_packet(ping);
			continue ;
		}
		send_echo(ping);
		gettimeofday(&last, NULL);
	}
}

static void	cleanup(t_ping *ping)
{
	free(ping->packet);
	close(ping->sock);
}

static int	ping_host(t_ping *ping, const char *host)
{
	memset(&ping->stats, 0, sizeof(ping->stats));
	memset(ping->dup_table, 0, sizeof(ping->dup_table));
	if (resolve_host(ping, host) < 0)
	{
		/* fatal, even with more hosts left, like inetutils */
		cleanup(ping);
		exit(EXIT_FAILURE);
	}
	printf("PING %s (%s): %d data bytes", ping->hostname, ping->ipstr,
		DATALEN);
	if (ping->opts.flags & OPT_VERBOSE)
		printf(", id 0x%04x = %u", ping->id, ping->id);
	printf("\n");
	send_echo(ping);
	ping_loop(ping);
	print_stats(ping);
	return (ping->stats.received == 0);
}

int	main(int argc, char **argv)
{
	static t_ping	ping;
	int		first;
	int		status;

	/* line by line even into a pipe, so `ft_ping x | tee log` works */
	setvbuf(stdout, NULL, _IOLBF, 0);
	first = parse_args(argc, argv, &ping.opts);
	ping.id = (uint16_t)(getpid() & 0xffff);
	if (open_socket(&ping) < 0)
		return (EXIT_FAILURE);
	if (build_packet(&ping) < 0)
	{
		close(ping.sock);
		return (EXIT_FAILURE);
	}
	setup_signals();
	status = 0;
	while (first < argc && !g_stop)
		status |= ping_host(&ping, argv[first++]);
	cleanup(&ping);
	return (status);
}
