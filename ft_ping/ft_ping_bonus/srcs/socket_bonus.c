#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "ft_ping_bonus.h"

/*
 * Raw ICMP socket. Needs root (or CAP_NET_RAW). Unlike inetutils we don't
 * fall back to an unprivileged SOCK_DGRAM ICMP socket: with those the
 * kernel rewrites the echo id and strips the IP header, and the subject is
 * about building and reading the packets ourselves anyway.
 */
int	open_socket(t_ping *ping)
{
	int	val;

	ping->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping->sock < 0)
	{
		if (errno == EPERM || errno == EACCES)
			fprintf(stderr, "%s: Lacking privilege for icmp socket.\n",
				PROGNAME);
		else
			error_msg("socket: %s", strerror(errno));
		return (-1);
	}
	if (ping->opts.ttl > 0)
	{
		val = ping->opts.ttl;
		if (setsockopt(ping->sock, IPPROTO_IP, IP_TTL, &val, sizeof(val)))
			error_msg("setsockopt(IP_TTL): %s", strerror(errno));
	}
	if (ping->opts.tos >= 0)
	{
		val = ping->opts.tos;
		if (setsockopt(ping->sock, IPPROTO_IP, IP_TOS, &val, sizeof(val)))
			error_msg("setsockopt(IP_TOS): %s", strerror(errno));
	}
	return (0);
}

/*
 * Forward lookup only. Replies are always printed with the numeric address,
 * the subject says not to do reverse DNS on returned packets.
 */
int	resolve_host(t_ping *ping, const char *host)
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	const char	*name;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	hints.ai_flags = AI_CANONNAME;
	if (getaddrinfo(host, NULL, &hints, &res) != 0 || res == NULL)
	{
		error_msg("unknown host");
		return (-1);
	}
	memcpy(&ping->dest, res->ai_addr, sizeof(ping->dest));
	name = res->ai_canonname ? res->ai_canonname : host;
	snprintf(ping->hostname, sizeof(ping->hostname), "%s", name);
	inet_ntop(AF_INET, &ping->dest.sin_addr, ping->ipstr, sizeof(ping->ipstr));
	freeaddrinfo(res);
	return (0);
}
