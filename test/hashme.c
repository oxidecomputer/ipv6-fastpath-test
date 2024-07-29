#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>

#define HASH_ADDR6(src, dst, ports)                                     \
        ((src.s6_addr32[0] ^ src.s6_addr32[1] ^                         \
        src.s6_addr32[2] ^ src.s6_addr32[3]) ^                          \
        (dst.s6_addr32[0] ^ dst.s6_addr32[1] ^                          \
        dst.s6_addr32[2] ^ dst.s6_addr32[3]) ^                          \
        ((ports) >> 24) ^ ((ports) >> 16) ^                             \
        ((ports) >> 8) ^ (ports))

#define s6_addr8        _S6_un._S6_u8
#define s6_addr16       _S6_un._S6_u16
#define s6_addr32       _S6_un._S6_u32

static const uint64_t N = 1ull<<32;

uint32_t *data = NULL;
void stats(int);

int main() {
	struct in6_addr src;
	struct in6_addr dst;
	uint32_t ports;
	char buf[256];
	uint32_t hash;

	uint64_t sz = N * sizeof(uint32_t);
	data = malloc(sz);
	memset(data, 0, sz);

	signal(SIGUSR1, stats);
	signal(SIGINFO, stats);

	printf("start\n");
	for (uint64_t a=0; a<N; a++) {
		dst.s6_addr32[3] = a;
		for (uint64_t b=0; b<N; b++) {
			dst.s6_addr32[2] = b;
			for (uint64_t c=0; c<N; c++) {
				dst.s6_addr32[1] = c;
				for (uint64_t d=0; d<N; d++) {
					dst.s6_addr32[0] = d;
					printf("%s\n", inet_ntop(AF_INET6, &dst, buf, 256));
					for (uint64_t e=0; e<N; e++) {
						ports = e;
						hash = HASH_ADDR6(src, dst, ports);
						data[hash]++;
					}
					stats(0);
				}
			}
		}
	}

	stats(0);

}

void stats(int sig) {
	uint32_t high = 0, low = 0xffffffff; 
	for (uint64_t i=0; i<N; i++) {
		if (data[i] > high) {
			high = data[i];
		}
		if (data[i] < low) {
			low = data[i];
		}
	}
	
	printf("high %d\n", high);
	printf("low %d\n", low);

	if (sig == SIGUSR1)
		signal(SIGUSR1, stats);
	if (sig == SIGINFO)
		signal(SIGINFO, stats);
}
