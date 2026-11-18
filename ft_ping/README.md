# ft_ping

My version of `ping`, written in C from scratch. The reference is `ping` from GNU inetutils 2.0 (the one you get with `ping -V` on Debian when `inetutils-ping` is installed), and the goal is to print exactly what it prints, apart from the RTT values and the reverse DNS lookup.

It builds an ICMP echo request by hand, sends it on a raw socket, and reads everything that comes back on that socket: echo replies to us, and ICMP errors (TTL exceeded, unreachable, ...) about the packets we sent.

## Layout

```
ft_ping/
├── Makefile
├── includes/
│   └── ft_ping.h
├── srcs/
│   ├── main.c         <- setup, the send/receive loop, Ctrl-C
│   ├── args.c         <- option parsing (getopt_long), help/usage
│   ├── socket.c       <- raw socket, TTL/TOS, DNS lookup
│   ├── packet.c       <- building and sending echo requests, checksum
│   ├── receive.c      <- reading replies, sorting out what's ours
│   ├── icmp_print.c   <- ICMP error messages and the -v header dump
│   ├── stats.c        <- min/avg/max/stddev and the summary
│   └── utils.c        <- time helpers, error messages
└── tests/
    ├── compare.sh     <- diff ft_ping against the system ping
    └── netns_lab.sh   <- fake router with network namespaces, for TTL tests
```

## What you need

- Linux (the subject wants a Debian VM, kernel > 3.14). Anything recent works.
- `gcc` and `make`.
- root, or `CAP_NET_RAW`. Raw sockets are privileged. Without it you get `ft_ping: Lacking privilege for icmp socket.`
- For testing only: `inetutils-ping` (the reference) and `iproute2` (for the namespace lab).

On Debian: `sudo apt install gcc make inetutils-ping iproute2`

## Building

```sh
make          # builds ./ft_ping
make clean    # removes obj/
make fclean   # removes obj/ and the binary
make re       # fclean + all
```

Running `make` twice does nothing the second time (`make relink-check` verifies it). Objects carry dependency files, so touching the header rebuilds what includes it.

Extra rules: `make run ARGS="-v -c 3 8.8.8.8"`, `make valgrind ARGS=...`, `make help`.

## Usage

```
sudo ./ft_ping [OPTION...] HOST ...
```

HOST can be an IPv4 address or a hostname/FQDN. The name is resolved once, forwards. Replies are always shown with the numeric address (the subject says no reverse DNS on returned packets).

```
$ sudo ./ft_ping -c 3 localhost
PING localhost (127.0.0.1): 56 data bytes
64 bytes from 127.0.0.1: icmp_seq=0 ttl=64 time=0.048 ms
64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.052 ms
64 bytes from 127.0.0.1: icmp_seq=2 ttl=64 time=0.056 ms
--- localhost ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max/stddev = 0.048/0.052/0.056/0.000 ms
```

### Options

Mandatory part:

| Option | What it does |
|--------|--------------|
| `-v`, `--verbose` | Shows the echo id in the first line, and for every ICMP error it shows the header of the packet that caused it (see below). Also shows errors that aren't about our target. |
| `-?`, `--help` | The help list. |

Bonus (one feature each, long and short form together count as one):

| Option | What it does |
|--------|--------------|
| `-c N`, `--count=N` | Stop after N replies (or after sending N and waiting `-W` seconds). |
| `-i N`, `--interval=N` | Seconds between packets, decimals allowed. Below 0.2 is root only. |
| `-w N`, `--timeout=N` | Stop after N seconds no matter what. |
| `-W N`, `--linger=N` | How long to wait for late replies once `-c` packets are out (default 10). |
| `-s N`, `--size=N` | Data bytes per packet (default 56, max 65399). Under 16 there's no room for the timestamp, so no `time=` and no round-trip line, same as inetutils. |
| `-p HEX`, `--pattern=HEX` | Fill the data with this byte pattern (up to 16 bytes), e.g. `-p ff00`. |
| `--ttl=N` | Set the IP time-to-live. `--ttl=1` is the easy way to get an error back. |
| `-T N`, `--tos=N` | Set the IP type-of-service byte. |
| `-q`, `--quiet` | Only the first line and the summary. |
| `-f`, `--flood` | Send as fast as replies come back (10ms max between sends). Prints a `.` per request and a backspace per reply, so the dots that stay on screen are lost packets. |
| `-l N`, `--preload=N` | Send N packets right away before the normal rhythm. |

Not bonus but there anyway: `-V`/`--version`, `--usage`.

### What -v shows

`--ttl=1` makes the first router drop the packet and send back "Time to live exceeded". With `-v` you also see the header of our packet that the router quoted back. This one is from the namespace lab (see below), 10.0.1.1 is the router:

```
$ sudo ip netns exec h ./ft_ping -v -c 1 --ttl=1 10.0.2.2
PING 10.0.2.2 (10.0.2.2): 56 data bytes, id 0x0bdb = 3035
92 bytes from 10.0.1.1: Time to live exceeded
IP Hdr Dump:
 4500 0054 d130 4000 0101 9175 0a00 0102 0a00 0202 
Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src	Dst	Data
 4  5  00 0054 d130   2 0000  01  01 9175 10.0.1.2  10.0.2.2 
ICMP: type 8, code 0, size 64, id 0x0bdb, seq 0x0000
--- 10.0.2.2 ping statistics ---
1 packets transmitted, 0 packets received, 100% packet loss
```

The "92 bytes" is 8 (ICMP error header) + 20 (our IP header) + 64 (our ICMP packet), which the router sent back in full. The second line of the dump decodes the first one: version 4, header length 5 words, total length 0x54 = 84, flags 2 (don't fragment), TTL 1, protocol 1 (ICMP), source, destination. The program doesn't stop on errors, it just reports them and keeps going.

### Exit status

- 0 if at least one reply came back
- 1 if none did, or on a fatal error (unknown host, bad option value, no privileges)
- 64 on a command-line usage error (missing host, unknown option), same as inetutils

## How it works, quickly

1. Parse options, open a `SOCK_RAW`/`IPPROTO_ICMP` socket, set TTL/TOS if asked.
2. `getaddrinfo` the host (IPv4 only), print the `PING ...` line.
3. Build the packet once: 8-byte ICMP header, then a `struct timeval` (the send time, so the reply carries it back and we don't need to remember it), then filler bytes.
4. Loop on `select()` with a timeout equal to the time left before the next send. Readable socket: read one packet. Timeout: send the next request (new seq, new timestamp, new checksum).
5. On every packet read: skip the IP header (`ip_hl * 4`), check the ICMP checksum, then
   - echo reply with our id (the pid): count it, check duplicates with a 65536-bit table indexed by seq, compute the RTT, print the line;
   - ICMP error: the error contains our original IP header and the first 8 bytes of our ICMP packet, so we can check the id and tell if it's about us;
   - anything else (other programs' pings, our own requests on `lo`): ignored.
6. Stop on Ctrl-C (SIGINT sets a flag, `select` returns `EINTR`), `-c`, or `-w`, then print the summary.

The stddev is computed like inetutils (sum and sum of squares, then a Newton square root with 0.0005 precision), which is why a fast local ping shows `0.000`: the variance is below the precision and inetutils rounds it to 0 too.

## Checking it yourself

`tests/compare.sh` runs the system ping and ft_ping with the same arguments and diffs the outputs, after masking what can't match (times, the pid-based id, and the IP id/checksum the kernel picks). The reference gets `-n` because ft_ping doesn't do reverse DNS.

```sh
sudo ./tests/compare.sh -c 3 localhost
sudo ./tests/compare.sh -v -c 3 localhost
sudo ./tests/compare.sh -c 2 -s 2000 -p ff00 localhost
sudo ./tests/compare.sh -x localhost       # bad option
sudo ./tests/compare.sh -c abc localhost   # bad value
sudo ./tests/compare.sh nosuch.invalid     # unknown host
```

For ICMP errors you need a router between you and the target. If your VM is behind NAT, `--ttl=1` to any outside address already works. Otherwise `tests/netns_lab.sh` builds one out of network namespaces:

```sh
sudo ./tests/netns_lab.sh up
sudo NS=h ./tests/compare.sh -v -c 3 --ttl=1 10.0.2.2   # Time to live exceeded
sudo NS=h ./tests/compare.sh -c 2 10.0.9.9              # Destination Net Unreachable
sudo ./tests/netns_lab.sh down
```

The kernel rate-limits ICMP errors, so now and then one of the two pings misses an error and the diff shows it. Run it again; the lab turns the TTL limit off on the router, but unreachable errors can still get dropped.

Other things worth trying during the evaluation:

- Ctrl-C in the middle: the summary is printed and the exit code depends on whether anything came back.
- `valgrind --leak-check=full --track-fds=yes ./ft_ping -v -c 3 localhost`: no leaks, no open fds besides the standard ones.
- `./ft_ping -c 3 localhost` as a normal user: the privilege error, no crash.

## Where it differs from inetutils, on purpose

- **No reverse DNS** on replies and errors (subject). inetutils prints `from _gateway (10.0.2.2)`, ft_ping prints `from 10.0.2.2`.
- **No unprivileged fallback.** When it can't open a raw socket, inetutils falls back to a `SOCK_DGRAM` ICMP socket, where the kernel picks the id and hides the IP header. ft_ping stays on raw sockets and asks for root.
- **`-?`, `--usage` and `-V`** only list what's implemented.
- **Negative `-i`** is refused instead of being taken as a strange value.

Some odd inetutils behaviours are copied on purpose, because the subject asks for the same output: `-l` ignores `-c` (`-c 3 -l 3` sends 4 packets), `-c -1` means "basically forever" because the value goes through `strtoul`, and an unknown host stops the program even if more hosts follow.
