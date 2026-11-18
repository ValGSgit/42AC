#include <stdio.h>
#include "ft_ping_bonus.h"

void	record_rtt(t_stats *st, double rtt)
{
	if (st->received + st->dups == 1 || rtt < st->tmin)
		st->tmin = rtt;
	if (rtt > st->tmax)
		st->tmax = rtt;
	st->tsum += rtt;
	st->tsumsq += rtt * rtt;
}

/*
 * Newton's method, so we don't need -lm. inetutils does the same thing
 * (nsqrt() in ping_common.c) with the same 0.0005 precision.
 */
#define SQRT_PREC	0.0005

static double	ft_sqrt(double a)
{
	double	x0;
	double	x1;

	if (a < SQRT_PREC)
		return (0);
	x1 = a / 2;
	x0 = 0;
	while (x1 - x0 > SQRT_PREC || x0 - x1 > SQRT_PREC)
	{
		x0 = x1;
		x1 = (x0 + a / x0) / 2;
	}
	return (x1);
}

void	print_stats(t_ping *ping)
{
	t_stats	*st;
	double	total;
	double	avg;
	double	var;

	st = &ping->stats;
	fflush(stdout);
	printf("--- %s ping statistics ---\n", ping->hostname);
	printf("%zu packets transmitted, ", st->sent);
	printf("%zu packets received, ", st->received);
	if (st->dups)
		printf("+%zu duplicates, ", st->dups);
	if (st->sent)
	{
		if (st->received > st->sent)
			printf("-- somebody's printing up packets!");
		else
			printf("%d%% packet loss",
				(int)(((st->sent - st->received) * 100) / st->sent));
	}
	printf("\n");
	if (st->received && timing_enabled(ping))
	{
		total = (double)(st->received + st->dups);
		avg = st->tsum / total;
		var = st->tsumsq / total - avg * avg;
		printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			st->tmin, avg, st->tmax, ft_sqrt(var));
	}
}
