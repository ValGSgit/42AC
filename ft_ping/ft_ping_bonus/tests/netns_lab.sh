#!/bin/bash
# Tiny 2-hop network made of network namespaces, to get real ICMP errors
# (TTL exceeded, net unreachable) without needing a real router.
#
#   [ns h] 10.0.1.2 ---- 10.0.1.1 [ns r1, router] 10.0.2.1 ---- 10.0.2.2 [ns t]
#
#   sudo ./tests/netns_lab.sh up
#   sudo ip netns exec h ./ft_ping -v --ttl=1 10.0.2.2   -> Time to live exceeded
#   sudo ip netns exec h ./ft_ping 10.0.9.9              -> Destination Net Unreachable
#   sudo ./tests/netns_lab.sh down
#
# Needs iproute2 and root.

set -e

up() {
	ip netns add h
	ip netns add r1
	ip netns add t
	ip link add h0 type veth peer name r0
	ip link add r1b type veth peer name t0
	ip link set h0 netns h
	ip link set r0 netns r1
	ip link set r1b netns r1
	ip link set t0 netns t

	ip -n h addr add 10.0.1.2/24 dev h0
	ip -n r1 addr add 10.0.1.1/24 dev r0
	ip -n r1 addr add 10.0.2.1/24 dev r1b
	ip -n t addr add 10.0.2.2/24 dev t0
	for ns in h r1 t; do ip -n $ns link set lo up; done
	ip -n h link set h0 up
	ip -n r1 link set r0 up
	ip -n r1 link set r1b up
	ip -n t link set t0 up

	ip -n h route add default via 10.0.1.1
	ip -n t route add default via 10.0.2.1
	ip netns exec r1 sysctl -qw net.ipv4.ip_forward=1
	# Linux rate-limits ICMP errors to ~1/s by default, which makes the
	# output random when comparing two pings. Turn that off on the router.
	ip netns exec r1 sysctl -qw net.ipv4.icmp_ratelimit=0
	echo "lab up: try  sudo ip netns exec h ./ft_ping -v --ttl=1 10.0.2.2"
}

down() {
	ip netns del h 2>/dev/null || true
	ip netns del r1 2>/dev/null || true
	ip netns del t 2>/dev/null || true
	echo "lab down"
}

case "$1" in
	up) up ;;
	down) down ;;
	*) echo "usage: $0 up|down" >&2; exit 1 ;;
esac
