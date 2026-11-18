#ifndef FT_PING_H
# define FT_PING_H

# include <stddef.h>
# include <stdint.h>
# include <signal.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# define PROGNAME		"ft_ping"
# define VERSION		"1.0"

/*
 * Same defaults as inetutils-2.0 (ping.h / ping_common.h). The mandatory
 * part has no option to change them: 56 data bytes, one packet a second.
 * 56 is enough room for the timestamp, so every reply gets a "time=".
 */
# define DATALEN		56
# define INTERVAL		1.0
# define RECV_BUFSIZE		(65535 + 1)

/* Exit codes, as in inetutils (EX_USAGE from sysexits.h) */
# define EXIT_USAGE		64

/* Option flags */
# define OPT_VERBOSE	0x01

typedef struct s_opts
{
	int		flags;
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
	t_stats			stats;
}	t_ping;

extern volatile sig_atomic_t	g_stop;

/* args.c */
int		parse_args(int argc, char **argv, t_opts *opts);

/* socket.c */
int		open_socket(t_ping *ping);
int		resolve_host(t_ping *ping, const char *host);

/* packet.c */
uint16_t	checksum(const void *data, size_t len);
int		build_packet(t_ping *ping);
int		send_echo(t_ping *ping);

/* receive.c */
void		receive_packet(t_ping *ping);

/* icmp_print.c */
void		print_icmp_error(t_ping *ping, struct ip *ip, struct icmp *icmp,
			size_t len, struct in_addr from);

/* stats.c */
void		record_rtt(t_stats *st, double rtt);
void		print_stats(t_ping *ping);

/* utils.c */
double		tv_diff_ms(const struct timeval *a, const struct timeval *b);
void		error_msg(const char *fmt, ...);
void		usage_error(const char *fmt, ...);

#endif
