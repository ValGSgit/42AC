#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include "ft_ping.h"

/*
 * RFC 1071 internet checksum: one's complement of the one's complement sum
 * of all 16-bit words. An odd trailing byte is padded with a zero byte.
 */
uint16_t	checksum(const void *data, size_t len)
{
	const uint16_t	*w;
	uint32_t	sum;

	w = data;
	sum = 0;
	while (len > 1)
	{
		sum += *w++;
		len -= 2;
	}
	if (len == 1)
		sum += *(const uint8_t *)w;
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return ((uint16_t)~sum);
}

/*
 * Packet layout:
 *   [ icmp header (8) ][ struct timeval (16) ][ filler ... ]
 * The filler is 0x00, 0x01, 0x02, ... like inetutils.
 */
int	build_packet(t_ping *ping)
{
	unsigned char	*data;
	size_t		i;

	ping->packet_len = ICMP_MINLEN + DATALEN;
	ping->packet = calloc(1, ping->packet_len);
	if (!ping->packet)
	{
		error_msg("%s", strerror(errno));
		return (-1);
	}
	data = ping->packet + ICMP_MINLEN;
	i = 0;
	while (i < DATALEN)
	{
		data[i] = (unsigned char)i;
		i++;
	}
	return (0);
}

static void	mark_unseen(t_ping *ping, uint16_t seq)
{
	ping->dup_table[seq >> 3] &= (unsigned char)~(1u << (seq & 7));
}

int	send_echo(t_ping *ping)
{
	struct icmp	*icmp;
	struct timeval	now;
	uint16_t	seq;
	ssize_t		n;

	seq = (uint16_t)ping->stats.sent;
	icmp = (struct icmp *)ping->packet;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = htons(ping->id);
	icmp->icmp_seq = htons(seq);
	icmp->icmp_cksum = 0;
	gettimeofday(&now, NULL);
	memcpy(ping->packet + ICMP_MINLEN, &now, sizeof(now));
	icmp->icmp_cksum = checksum(ping->packet, ping->packet_len);
	mark_unseen(ping, seq);
	n = sendto(ping->sock, ping->packet, ping->packet_len, 0,
			(struct sockaddr *)&ping->dest, sizeof(ping->dest));
	/* Counted even if it failed, so a dead link shows as packet loss */
	ping->stats.sent++;
	if (n < 0)
	{
		error_msg("sending packet: %s", strerror(errno));
		return (-1);
	}
	return (0);
}
