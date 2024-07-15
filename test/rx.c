#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/errno.h>

extern int errno;

int main() {
	int sk;
	struct sockaddr_in6 sa, sender;
	socklen_t sender_len;
	char msg[4096], sender_s[256];

	memset(&sa, 0, sizeof(struct sockaddr_in6));
	memset(&sender, 0, sizeof(struct sockaddr_in6));
	memset(&msg, 0, 1024);
	memset(&sender_s, 0, 256);
	sender_len = sizeof(struct sockaddr_in6);
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(0x1701);
	inet_pton(AF_INET6, "fd00::2", &sa.sin6_addr);

	sk = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (bind(sk, (struct sockaddr *)&sa, sizeof(struct sockaddr_in6))) {
		printf("bind: %s\n", strerror(errno));
		return -1;
	}

	while (true) {
		recvfrom(sk, msg, 4096, 0, (struct sockaddr *)&sender, &sender_len);
		inet_ntop(AF_INET6, &(sender.sin6_addr), sender_s, 256);
		printf("[%s]: %s\n", sender_s, msg);
	}
}
