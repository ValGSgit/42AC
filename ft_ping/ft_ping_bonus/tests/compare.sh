#!/bin/bash
# Runs the system ping (inetutils) and ./ft_ping with the same arguments and
# diffs the output. Only a test helper: ft_ping itself never calls ping.
#
#   sudo ./tests/compare.sh -c 3 localhost
#   sudo NS=h ./tests/compare.sh -v -c 2 --ttl=1 10.0.2.2   (inside netns "h")
#
# Things that can't match are masked before diffing: times, the echo id
# (it's the pid), and the IP id/checksum the kernel picks for each packet.
# The system ping gets -n because the subject says no reverse DNS.

cd "$(dirname "$0")/.." || exit 1

run() {
	if [ -n "$NS" ]; then
		timeout 20 ip netns exec "$NS" "$@" 2>&1
	else
		timeout 20 "$@" 2>&1
	fi
	echo "exit=$?"
}

norm() {
	sed -E \
		-e 's/ft_ping/ping/g' \
		-e 's/time=[0-9.]+/time=X/' \
		-e 's/= [0-9.\/]+ ms/= X ms/' \
		-e 's/id 0x[0-9a-f]{4} = [0-9]+/id ID/' \
		-e 's/(id|seq) 0x[0-9a-f]{4}/\1 H/g' \
		-e 's/^ 4500 (....) .... (....) (....) ..../ 4500 \1 IPID \2 \3 CKS/' \
		-e 's/^( 4  5  [0-9a-f]{2} [0-9a-f]{4}) [0-9a-f]{4}/\1 IPID/' \
		-e 's/^( 4  5  .* [0-9a-f]{2}  [0-9a-f]{2}) [0-9a-f]{4} /\1 CKS /'
}

if ! command -v ping >/dev/null || ! ping -V 2>&1 | grep -q inetutils; then
	echo "need inetutils ping as reference (apt install inetutils-ping)" >&2
	exit 2
fi

ref=$(run ping -n "$@" | norm)
mine=$(run ./ft_ping "$@" | norm)
if diff <(echo "$ref") <(echo "$mine"); then
	echo "SAME: $*"
else
	echo "DIFF: $*  (< ping, > ft_ping)"
	exit 1
fi
