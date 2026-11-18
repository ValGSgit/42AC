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

static int	timed_out(t_ping *ping)
{
	struct timeval	now;

	if (!ping->opts.timeout)
		return (0);
	gettimeofday(&now, NULL);
	return (tv_diff_ms(&now, &ping->start) >= (double)ping->opts.timeout * 1000.0);
}

/*
 * In flood mode every request sent from the loop prints a '.', and every
 * reply erases one with '\b'. Like inetutils, the first packet and the
 * preloaded ones (sent before the loop) don't get a dot.
 */
static void	send_flood_aware(t_ping *ping)
{
	send_echo(ping);
	if ((ping->opts.flags & OPT_FLOOD) && !(ping->opts.flags & OPT_QUIET))
	{
		putchar('.');
		fflush(stdout);
	}
}

/*
 * The main loop, same idea as ping_run() in inetutils: select() on the
 * socket with a timeout equal to the time left until the next send.
 *  - socket readable  -> read a packet
 *  - timeout          -> send the next echo request
 * Once -c packets have been sent we wait one more "linger" period (-W, 10s
 * by default) for late replies, unless everything already came back.
 */
static void	ping_loop(t_ping *ping)
{
	struct timeval	last;
	struct timeval	now;
	struct timeval	tv;
	fd_set		fds;
	double		wait;
	size_t		nresp;
	int		finishing;
	int		n;

	nresp = 0;
	finishing = 0;
	wait = ping->opts.interval;
	gettimeofday(&last, NULL);
	while (!g_stop && !timed_out(ping))
	{
		gettimeofday(&now, NULL);
		tv = seconds_to_tv(wait - tv_diff_ms(&now, &last) / 1000.0);
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
			if (receive_packet(ping) == 0)
				nresp++;
			if (ping->opts.count && nresp >= ping->opts.count)
				break ;
			continue ;
		}
		if (!ping->opts.count || ping->stats.sent < ping->opts.count)
			send_flood_aware(ping);
		else if (finishing)
			break ;
		else
		{
			finishing = 1;
			wait = (double)ping->opts.linger;
		}
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
	size_t	i;

	memset(&ping->stats, 0, sizeof(ping->stats));
	memset(ping->dup_table, 0, sizeof(ping->dup_table));
	if (resolve_host(ping, host) < 0)
	{
		/* fatal, even with more hosts left, like inetutils */
		cleanup(ping);
		exit(EXIT_FAILURE);
	}
	printf("PING %s (%s): %zu data bytes", ping->hostname, ping->ipstr,
		ping->opts.datalen);
	if (ping->opts.flags & OPT_VERBOSE)
		printf(", id 0x%04x = %u", ping->id, ping->id);
	printf("\n");
	gettimeofday(&ping->start, NULL);
	/*
	 * Like inetutils, -l doesn't care about -c and the first packet always
	 * goes out: "-c 3 -l 3" sends 4. Weird, but it's the reference.
	 */
	i = 0;
	while (i++ < ping->opts.preload)
		send_echo(ping);
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
