#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <libdlpi.h>
#include <stdbool.h>

const uint8_t SEGMENT_ROUTING_HEADER = 4;
#define FRAG_SIZE 896

#pragma pack(push, 1)

// As extension header option as described in RFC 8200. Simplified to be a padN where N=4
struct ip6_opt_pad4 {
	uint8_t  ip6opt_type;
	uint8_t  ip6opt_len;
	uint32_t ip6opt_pad;
};
typedef struct ip6_opt_pad4 ip6_opt_pad4_t;


// Segment routing header as described in RFC 8754. Simplified to have two
// static segments.
struct ip6_srh {
	uint8_t		ip6srh_next;
	uint8_t		ip6srh_len;
	uint8_t		ip6srh_type;
	uint8_t		ip6srh_segs_left;
	uint8_t		ip6srh_last_entry;
	uint8_t		ip6srh_flags;
	uint16_t	ip6srh_tag;
	struct in6_addr	ip6srh_seg0;
	struct in6_addr	ip6srh_seg1;
};
typedef struct ip6_srh ip6_srh_t;

struct payload {
	char message[128];
};
typedef struct payload payload_t;

struct payload_frag {
	char message[FRAG_SIZE];
};
typedef struct payload_frag payload_frag_t;

typedef struct udphdr udp_t;

struct pkt_hbh {
	ip6_t 		ip6;
	ip6_hbh_t 	ext_hbh;
	ip6_opt_pad4_t  ext_opt_pad4;
	udp_t		udp;
	payload_t	payload;
};
typedef struct pkt_hbh pkt_hbh_t;

struct pkt_rtr {
	ip6_t 		ip6;
	ip6_srh_t 	ext_srh;
	udp_t		udp;
	payload_t	payload;
};
typedef struct pkt_rtr pkt_rtr_t;

struct pkt_hbh_rtr {
	ip6_t 		ip6;
	ip6_hbh_t 	ext_hbh;
	ip6_opt_pad4_t  ext_opt_pad4;
	ip6_srh_t 	ext_srh;
	udp_t		udp;
	payload_t	payload;
};
typedef struct pkt_hbh_rtr pkt_hbh_rtr_t;

// The first part of a fragmented packet
struct pkt_frag0 {
	ip6_t 		ip6;
	ip6_frag_t 	ext_frag;
	udp_t		udp;
	payload_frag_t	payload;
};
typedef struct pkt_frag0 pkt_frag0_t;

// The second part of a fragmented packet
struct pkt_frag1 {
	ip6_t 		ip6;
	ip6_frag_t 	ext_frag;
	payload_frag_t	payload;
};
typedef struct pkt_frag1 pkt_frag1_t;

struct pkt_dst {
	ip6_t 		ip6;
	ip6_dest_t 	ext_dst;
	ip6_opt_pad4_t  ext_opt_pad4;
	udp_t		udp;
	payload_t	payload;
};
typedef struct pkt_dst pkt_dst_t;

pkt_hbh_t create_hbh_pkt();
pkt_rtr_t create_rtr_pkt();
pkt_frag0_t create_frag0_pkt(char*, size_t);
pkt_frag1_t create_frag1_pkt(char*, size_t);
pkt_hbh_rtr_t create_hbh_rtr_pkt();
pkt_dst_t create_dst_pkt();
ip6_t base_ipv6(size_t, uint8_t);
payload_t base_payload(char*);
void send_pkt(void *const, size_t);
uint16_t udp6_checksum(ip6_t *ip6, udp_t *, char *, size_t);
udp_t base_udp(uint16_t);
void set_hbh(ip6_hbh_t *, ip6_opt_pad4_t *, uint8_t);
void set_srh(ip6_srh_t *, uint8_t);
void set_frag(ip6_frag_t *, uint16_t, bool, uint8_t, uint32_t);
void set_dst(ip6_dest_t *, ip6_opt_pad4_t *, uint8_t);
payload_frag_t base_frag_payload(char *, size_t);

int main() {
	pkt_hbh_t hbh = create_hbh_pkt();
	send_pkt(&hbh, sizeof(pkt_hbh_t));

	pkt_rtr_t rtr = create_rtr_pkt();
	send_pkt(&rtr, sizeof(pkt_rtr_t));

	pkt_hbh_rtr_t hbh_rtr = create_hbh_rtr_pkt();
	send_pkt(&hbh_rtr, sizeof(pkt_hbh_rtr_t));

	char payload[FRAG_SIZE*2];
	memset(payload, 'A', FRAG_SIZE);
	memset(payload+FRAG_SIZE, 'B', FRAG_SIZE);

	pkt_frag0_t frag0 = create_frag0_pkt(payload, FRAG_SIZE);
	send_pkt(&frag0, sizeof(pkt_frag0_t));

	pkt_frag1_t frag1 = create_frag1_pkt(payload+FRAG_SIZE, FRAG_SIZE);
	send_pkt(&frag1, sizeof(pkt_frag1_t));

	pkt_dst_t dst = create_dst_pkt();
	send_pkt(&dst, sizeof(pkt_dst_t));
}

void send_pkt(void *const pkt, size_t size) {
	dlpi_handle_t h;
	uint8_t dst[6] = {0x02, 0x08, 0x20, 0x2, 0x96, 0x56};

	dlpi_open("vnic1", &h, 0);
	dlpi_bind(h, 0x86dd, NULL);
	dlpi_send(h, dst, 6, pkt, size, NULL);
}

pkt_hbh_t create_hbh_pkt() {
	pkt_hbh_t pkt;
	memset(&pkt, 0, sizeof(pkt_hbh_t));

	pkt.ip6 = base_ipv6(sizeof(pkt_hbh_t) - sizeof(ip6_t), IPPROTO_HOPOPTS);
	set_hbh(&pkt.ext_hbh, &pkt.ext_opt_pad4, IPPROTO_UDP);
	pkt.udp = base_udp(sizeof(payload_t));
	pkt.payload = base_payload("hop-by-hop");
	pkt.udp.uh_sum = udp6_checksum(&pkt.ip6, &pkt.udp, pkt.payload.message, 128);
	
	return pkt;
}

pkt_dst_t create_dst_pkt() {
	pkt_dst_t pkt;
	memset(&pkt, 0, sizeof(pkt_dst_t));

	pkt.ip6 = base_ipv6(sizeof(pkt_dst_t) - sizeof(ip6_t), IPPROTO_DSTOPTS);
	set_dst(&pkt.ext_dst, &pkt.ext_opt_pad4, IPPROTO_UDP);
	pkt.udp = base_udp(sizeof(payload_t));
	pkt.payload = base_payload("destination");
	pkt.udp.uh_sum = udp6_checksum(&pkt.ip6, &pkt.udp, pkt.payload.message, 128);

	return pkt;
}

pkt_rtr_t create_rtr_pkt() {
	pkt_rtr_t pkt;
	memset(&pkt, 0, sizeof(pkt_rtr_t));

	pkt.ip6 = base_ipv6(sizeof(pkt_rtr_t) - sizeof(ip6_t), IPPROTO_ROUTING);
	set_srh(&pkt.ext_srh, IPPROTO_UDP);
	pkt.udp = base_udp(sizeof(payload_t));
	pkt.payload = base_payload("router");
	pkt.udp.uh_sum = udp6_checksum(&pkt.ip6, &pkt.udp, pkt.payload.message, 128);
	
	return pkt;
}

pkt_hbh_rtr_t create_hbh_rtr_pkt() {
	pkt_hbh_rtr_t pkt;
	memset(&pkt, 0, sizeof(pkt_hbh_rtr_t));

	pkt.ip6 = base_ipv6(sizeof(pkt_hbh_rtr_t) - sizeof(ip6_t), IPPROTO_HOPOPTS);
	set_hbh(&pkt.ext_hbh, &pkt.ext_opt_pad4, IPPROTO_ROUTING);
	set_srh(&pkt.ext_srh, IPPROTO_UDP);
	pkt.udp = base_udp(sizeof(payload_t));
	pkt.payload = base_payload("hop-by-hop+router");
	pkt.udp.uh_sum = udp6_checksum(&pkt.ip6, &pkt.udp, pkt.payload.message, 128);

	return pkt;
}

pkt_frag0_t create_frag0_pkt(char* payload, size_t len) {
	pkt_frag0_t pkt;
	memset(&pkt, 0, sizeof(pkt_frag0_t));

	pkt.ip6 = base_ipv6(sizeof(pkt_frag0_t) - sizeof(ip6_t), IPPROTO_FRAGMENT);
	set_frag(&pkt.ext_frag, 0, true, IPPROTO_UDP, 0x1701d);
	pkt.udp = base_udp(sizeof(payload_frag_t)*2);
	pkt.payload = base_frag_payload(payload, len);

	pkt.udp.uh_sum = udp6_checksum(&pkt.ip6, &pkt.udp, payload, len*2);

	return pkt;
}

pkt_frag1_t create_frag1_pkt(char* payload, size_t len) {
	pkt_frag1_t pkt;
	memset(&pkt, 0, sizeof(pkt_frag1_t));

	pkt.ip6 = base_ipv6(sizeof(pkt_frag1_t) - sizeof(ip6_t), IPPROTO_FRAGMENT);
	set_frag(&pkt.ext_frag, 1+(sizeof(payload_frag_t)>>3), false, IPPROTO_UDP, 0x1701d);
	pkt.payload = base_frag_payload(payload, len);

	return pkt;
}

ip6_t base_ipv6(size_t plen, uint8_t proto) {
	ip6_t ip6;
	memset(&ip6, 0, sizeof(ip6_t));
	ip6.ip6_vfc = 0b01100000;
	ip6.ip6_plen = htons(plen);
	ip6.ip6_nxt = proto;
	ip6.ip6_hlim = 64;
	inet_pton(AF_INET6, "fd00::1", &ip6.ip6_src);
	inet_pton(AF_INET6, "fd00::2", &ip6.ip6_dst);
	return ip6;
}

udp_t base_udp(uint16_t payload_size) {
	udp_t udp;
	memset(&udp, 0, sizeof(udp_t));
	udp.uh_sport = htons(0x1701);
	udp.uh_dport = htons(0x1701);
	udp.uh_ulen = htons(sizeof(udp_t) + payload_size);
	return udp;
}

payload_t base_payload(char *msg) {
	payload_t payload;
	memset(&payload, 0, sizeof(payload_t));
	strcpy(payload.message, msg);
	return payload;
}

payload_frag_t base_frag_payload(char *msg, size_t len) {
	payload_frag_t payload;
	memcpy(&payload, msg, len);
	return payload;
}

void set_hbh(ip6_hbh_t *ext_hbh, ip6_opt_pad4_t *ext_opt_pad4, uint8_t proto) {
	ext_hbh->ip6h_nxt = proto;
	ext_hbh->ip6h_len = 0;
	ext_opt_pad4->ip6opt_type = IP6OPT_PADN;
	ext_opt_pad4->ip6opt_len = 4;
}

void set_dst(ip6_dest_t *ext_dst, ip6_opt_pad4_t *ext_opt_pad4, uint8_t proto) {
	ext_dst->ip6d_nxt = proto;
	ext_dst->ip6d_len = 0;
	ext_opt_pad4->ip6opt_type = IP6OPT_PADN;
	ext_opt_pad4->ip6opt_len = 4;
}

void set_srh(ip6_srh_t *ext_srh, uint8_t proto) {
	ext_srh->ip6srh_next = proto;
	ext_srh->ip6srh_len = 4;
	ext_srh->ip6srh_type = SEGMENT_ROUTING_HEADER;
	ext_srh->ip6srh_segs_left = 0;
	ext_srh->ip6srh_last_entry = 1;
	ext_srh->ip6srh_flags = 0;
	ext_srh->ip6srh_tag = 0;
	inet_pton(AF_INET6, "fd00::2", &ext_srh->ip6srh_seg0);
	inet_pton(AF_INET6, "fd00::2", &ext_srh->ip6srh_seg1);
}

void set_frag(
    ip6_frag_t *ext_frag,
    uint16_t offset,
    bool more,
    uint8_t proto,
    uint32_t id) {
	ext_frag->ip6f_nxt = proto;
	ext_frag->ip6f_reserved = 0;
	ext_frag->ip6f_offlg = (offset & 0xe0) >> 5 | (offset & 0x1f) << 11;
	ext_frag->ip6f_offlg |= (uint16_t)more << 8;
	printf("%x\n", ext_frag->ip6f_offlg);
	ext_frag->ip6f_ident = htonl(id);
}

uint16_t udp6_checksum(ip6_t *ip6, udp_t *udp, char *payload, size_t payload_len) {
	uint32_t csum = 0;
	for (size_t i=0; i<8; i++) csum += ip6->ip6_src._S6_un._S6_u16[i];
	for (size_t i=0; i<8; i++) csum += ip6->ip6_dst._S6_un._S6_u16[i];
	csum += (uint16_t)udp->uh_ulen;
	csum += ((uint16_t)IPPROTO_UDP)<<8;
	csum += udp->uh_sport;
	csum += udp->uh_dport;
	csum += (uint16_t)udp->uh_ulen;
	uint16_t *data = (uint16_t*)payload;
	for (size_t i=0; i<(payload_len>>1); i++) csum += data[i];
	while (csum>>16) csum = (csum & 0xffff) + (csum >> 16);
	return ~csum;
}

#pragma pack(pop)
