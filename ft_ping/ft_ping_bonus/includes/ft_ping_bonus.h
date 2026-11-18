#ifndef FT_PING_BONUS_H
# define FT_PING_BONUS_H

# include <stddef.h>
# include <stdint.h>
# include <signal.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# define PROGNAME		"ft_ping"
# define VERSION		"1.0"

/* Same defaults and limits as inetutils-2.0 (ping.h / ping_common.h) */
# define DEFAULT_DATALEN	56
# define MAX_IPLEN		60
# define MAX_ICMPLEN		76
# define MAX_DATALEN		(65535 - MAX_IPLEN - MAX_ICMPLEN)
# define DEFAULT_INTERVAL	1.0
# define FLOOD_INTERVAL		0.01
# define DEFAULT_LINGER		10
# define MAX_PATTERN		16
# define RECV_BUFSIZE		(65535 + 1)

/* Exit codes, as in inetutils (EX_USAGE from sysexits.h) */
# define EXIT_USAGE		64

/* Option flags */
# define OPT_VERBOSE	0x01
# define OPT_QUIET	0x02
# define OPT_FLOOD	0x04
# define OPT_INTERVAL	0x08

typedef struct s_opts
{
	int		flags;
	size_t		count;		/* -c, 0 = forever */
	double		interval;	/* -i, seconds */
	long		timeout;	/* -w, seconds, 0 = none */
	long		linger;		/* -W, seconds */
	size_t		datalen;	/* -s */
	int		ttl;		/* --ttl, 0 = system default */
	int		tos;		/* -T, -1 = untouched */
	size_t		preload;	/* -l */
	unsigned char	pattern[MAX_PATTERN];
	size_t		pattern_len;
}	t_opts;

typedef struct s_stats
{
	size_t	sent;
	size_t	received;
	size_t	dups;
	double	tmin;
	double	tmax;
	double	tsum;
	double	tsumsq;
}	t_stats;

typedef struct s_ping
{
	t_opts			opts;
	int			sock;
	uint16_t		id;
	char			hostname[1025];	/* what we print in "PING x" */
	char			ipstr[16];
	struct sockaddr_in	dest;
	unsigned char		*packet;	/* outgoing ICMP packet */
	size_t			packet_len;
	unsigned char		dup_table[65536 / 8];
	struct timeval		start;
	t_stats			stats;
}	t_ping;

extern volatile sig_atomic_t	g_stop;

/* args_bonus.c */
int		parse_args(int argc, char **argv, t_opts *opts);

/* socket_bonus.c */
int		open_socket(t_ping *ping);
int		resolve_host(t_ping *ping, const char *host);

/* packet_bonus.c */
uint16_t	checksum(const void *data, size_t len);
int		build_packet(t_ping *ping);
int		send_echo(t_ping *ping);

/* receive_bonus.c */
int		receive_packet(t_ping *ping);

/* icmp_print_bonus.c */
int		print_icmp_error(t_ping *ping, struct ip *ip, struct icmp *icmp,
			size_t len, struct in_addr from);

/* stats_bonus.c */
void		record_rtt(t_stats *st, double rtt);
void		print_stats(t_ping *ping);

/* utils_bonus.c */
double		tv_diff_ms(const struct timeval *a, const struct timeval *b);
int		timing_enabled(const t_ping *ping);
void		error_msg(const char *fmt, ...);
void		usage_error(const char *fmt, ...);

#endif
