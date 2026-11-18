#include <stdio.h>
#include <arpa/inet.h>
#include "ft_ping.h"

typedef struct s_icmp_desc
{
	int		type;
	int		code;
	const char	*text;
}	t_icmp_desc;

/* Same wording as inetutils (ping/ping_echo.c, icmp_code_descr) */
static const t_icmp_desc	g_descr[] = {
	{ICMP_DEST_UNREACH, ICMP_NET_UNREACH, "Destination Net Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, "Destination Host Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, "Destination Protocol Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, "Destination Port Unreachable"},
	{ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, "Fragmentation needed and DF set"},
	{ICMP_DEST_UNREACH, ICMP_SR_FAILED, "Source Route Failed"},
	{ICMP_DEST_UNREACH, ICMP_NET_UNKNOWN, "Network Unknown"},
	{ICMP_DEST_UNREACH, ICMP_HOST_UNKNOWN, "Host Unknown"},
	{ICMP_DEST_UNREACH, ICMP_HOST_ISOLATED, "Host Isolated"},
	{ICMP_DEST_UNREACH, ICMP_NET_UNR_TOS,
		"Destination Network Unreachable At This TOS"},
	{ICMP_DEST_UNREACH, ICMP_HOST_UNR_TOS,
		"Destination Host Unreachable At This TOS"},
	{ICMP_DEST_UNREACH, ICMP_PKT_FILTERED, "Packet Filtered"},
	{ICMP_DEST_UNREACH, ICMP_PREC_VIOLATION, "Precedence Violation"},
	{ICMP_DEST_UNREACH, ICMP_PREC_CUTOFF, "Precedence Cutoff"},
	{ICMP_REDIRECT, ICMP_REDIR_NET, "Redirect Network"},
	{ICMP_REDIRECT, ICMP_REDIR_HOST, "Redirect Host"},
	{ICMP_REDIRECT, ICMP_REDIR_NETTOS, "Redirect Type of Service and Network"},
	{ICMP_REDIRECT, ICMP_REDIR_HOSTTOS, "Redirect Type of Service and Host"},
	{ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, "Time to live exceeded"},
	{ICMP_TIME_EXCEEDED, ICMP_EXC_FRAGTIME, "Frag reassembly time exceeded"},
	{ICMP_SOURCE_QUENCH, -1, "Source Quench"},
	{ICMP_PARAMETERPROB, -1, "Parameter problem"},
	{-1, -1, NULL}
};

static const char	*describe(int type, int code)
{
	int	i;

	i = 0;
	while (g_descr[i].text)
	{
		if (g_descr[i].type == type
			&& (g_descr[i].code == -1 || g_descr[i].code == code))
			return (g_descr[i].text);
		i++;
	}
	return (NULL);
}

/*
 * Prints the header of the packet that caused the error (it's quoted back
 * to us inside the ICMP error). Format copied from inetutils' -v output:
 *
 * IP Hdr Dump:
 *  4500 0054 ... (raw bytes, grouped by 2)
 * Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src	Dst	Data
 *  4  5  00 0054 1234   2 0000  01  01 abcd 10.0.2.15  8.8.8.8
 * ICMP: type 8, code 0, size 64, id 0x1234, seq 0x0000
 */
static void	dump_orig_ip(struct ip *ip)
{
	unsigned char	*cp;
	size_t		hlen;
	size_t		i;
	uint16_t	off;

	cp = (unsigned char *)ip;
	printf("IP Hdr Dump:\n ");
	i = 0;
	while (i < sizeof(*ip))
	{
		printf("%02x%s", cp[i], (i % 2) ? " " : "");
		i++;
	}
	printf("\n");
	off = ntohs(ip->ip_off);
	printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
	printf(" %1x  %1x  %02x %04x %04x", ip->ip_v, ip->ip_hl, ip->ip_tos,
		ntohs(ip->ip_len), ntohs(ip->ip_id));
	printf("   %1x %04x", (off & 0xe000) >> 13, off & 0x1fff);
	printf("  %02x  %02x %04x", ip->ip_ttl, ip->ip_p, ntohs(ip->ip_sum));
	printf(" %s ", inet_ntoa(ip->ip_src));
	printf(" %s ", inet_ntoa(ip->ip_dst));
	hlen = (size_t)ip->ip_hl << 2;
	i = sizeof(*ip);
	while (i < hlen)
		printf("%02x", cp[i++]);
	printf("\n");
}

static void	dump_orig_icmp(struct ip *ip)
{
	unsigned char	*cp;
	size_t		hlen;

	hlen = (size_t)ip->ip_hl << 2;
	cp = (unsigned char *)ip + hlen;
	if (ip->ip_p != IPPROTO_ICMP)
		return ;
	printf("ICMP: type %u, code %u, size %u", cp[0], cp[1],
		(unsigned)(ntohs(ip->ip_len) - hlen));
	if (cp[0] == ICMP_ECHOREPLY || cp[0] == ICMP_ECHO)
		printf(", id 0x%04x, seq 0x%04x", cp[4] * 256 + cp[5],
			cp[6] * 256 + cp[7]);
	printf("\n");
}

/* Is the packet quoted inside the error one of our echo requests? */
static int	is_ours(t_ping *ping, struct ip *orig)
{
	struct icmp	*orig_icmp;

	if (orig->ip_p != IPPROTO_ICMP)
		return (0);
	orig_icmp = (struct icmp *)((unsigned char *)orig + (orig->ip_hl << 2));
	return (orig_icmp->icmp_type == ICMP_ECHO
		&& ntohs(orig_icmp->icmp_id) == ping->id);
}

/*
 * len is the full datagram length (outer IP header included).
 * Without -v we only report errors about packets sent to our target, like
 * inetutils does. With -v everything is shown, even errors caused by other
 * programs' traffic.
 *
 * Returns 1 if the error was about one of our packets (the caller counts it
 * as a response), 0 otherwise.
 */
int	print_icmp_error(t_ping *ping, struct ip *ip, struct icmp *icmp,
		size_t len, struct in_addr from)
{
	size_t		hlen;
	struct ip	*orig;
	const char	*text;
	int		ours;

	hlen = (size_t)ip->ip_hl << 2;
	if (len < hlen + ICMP_MINLEN + sizeof(struct ip))
		return (0);
	orig = &icmp->icmp_ip;
	if (len < hlen + ICMP_MINLEN + ((size_t)orig->ip_hl << 2) + 8)
		return (0);
	ours = is_ours(ping, orig);
	if (!(ping->opts.flags & OPT_VERBOSE)
		&& orig->ip_dst.s_addr != ping->dest.sin_addr.s_addr)
		return (ours);
	if (ping->opts.flags & OPT_FLOOD)
		putchar('\n');
	printf("%zu bytes from %s: ", len - hlen, inet_ntoa(from));
	text = describe(icmp->icmp_type, icmp->icmp_code);
	if (text)
		printf("%s\n", text);
	else
		printf("Unknown Code: %d, type %d\n", icmp->icmp_code,
			icmp->icmp_type);
	if (ping->opts.flags & OPT_VERBOSE)
	{
		dump_orig_ip(orig);
		dump_orig_icmp(orig);
	}
	return (ours);
}
