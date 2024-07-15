# Ipv6 Fastpath Test

This repo contains some code for testing an IPv6 fast path patch for illumos.

- `setup.sh`: a script for setting up two zones on a machine to send packets
  between.
- `test`: Two C programs for sending and recieving IPv6/UDP packets with various
  types of extension headers.
