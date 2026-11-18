#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "ft_ping.h"

/* Returns 1 if we already got a reply for this seq, and marks it seen. */
static int	check_dup(t_ping *ping, uint16_t seq)
{
	unsigned char	bit;

	bit = (unsigned char)(1u << (seq & 7));
	if (ping->dup_table[seq >> 3] & bit)
		return (1);
	ping->dup_table[seq >> 3] |= bit;
	return (0);
}

static void	handle_echo_reply(t_ping *ping, struct ip *ip, struct icmp *icmp,
			size_t icmp_len, struct in_addr from)
{
	struct timeval	now;
	struct timeval	sent_at;
	double		rtt;
	int		dup;
	uint16_t	seq;

	seq = ntohs(icmp->icmp_seq);
	dup = check_dup(ping, seq);
	if (dup)
		ping->stats.dups++;
	else
		ping->stats.received++;
	rtt = 0;
	if (icmp_len >= ICMP_MINLEN + sizeof(sent_at))
	{
		gettimeofday(&now, NULL);
		memcpy(&sent_at, (unsigned char *)icmp + ICMP_MINLEN, sizeof(sent_at));
		rtt = tv_diff_ms(&now, &sent_at);
		record_rtt(&ping->stats, rtt);
	}
	printf("%zu bytes from %s: icmp_seq=%u ttl=%d", icmp_len,
		inet_ntoa(from), seq, ip->ip_ttl);
	printf(" time=%.3f ms", rtt);
	if (dup)
		printf(" (DUP!)");
	printf("\n");
}

static int	is_error_type(int type)
{
	return (type == ICMP_DEST_UNREACH || type == ICMP_SOURCE_QUENCH
		|| type == ICMP_REDIRECT || type == ICMP_TIME_EXCEEDED
		|| type == ICMP_PARAMETERPROB);
}

/*
 * Reads one datagram from the socket. A raw ICMP socket gets *every* ICMP
 * packet that reaches the host (other pings, our own requests on loopback,
 * ...), so most of the work here is throwing away what isn't ours.
 */
void	receive_packet(t_ping *ping)
{
	static unsigned char	buf[RECV_BUFSIZE];
	struct sockaddr_in	from;
	socklen_t		fromlen;
	ssize_t			n;
	struct ip		*ip;
	struct icmp		*icmp;
	size_t			hlen;

	fromlen = sizeof(from);
	n = recvfrom(ping->sock, buf, sizeof(buf), 0,
			(struct sockaddr *)&from, &fromlen);
	if (n < 0)
	{
		if (errno != EINTR && errno != EAGAIN)
			error_msg("receiving packet: %s", strerror(errno));
		return ;
	}
	ip = (struct ip *)buf;
	hlen = (size_t)ip->ip_hl << 2;
	if ((size_t)n < hlen + ICMP_MINLEN)
	{
		if (ping->opts.flags & OPT_VERBOSE)
			fprintf(stderr, "packet too short (%zd bytes) from %s\n",
				n, inet_ntoa(from.sin_addr));
		return ;
	}
	icmp = (struct icmp *)(buf + hlen);
	if (checksum(icmp, (size_t)n - hlen) != 0)
		printf("checksum mismatch from %s\n", inet_ntoa(from.sin_addr));
	if (icmp->icmp_type == ICMP_ECHOREPLY)
	{
		if (ntohs(icmp->icmp_id) == ping->id)
			handle_echo_reply(ping, ip, icmp, (size_t)n - hlen,
				from.sin_addr);
	}
	else if (is_error_type(icmp->icmp_type))
		print_icmp_error(ping, ip, icmp, (size_t)n, from.sin_addr);
}
