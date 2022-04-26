#ifndef	PING_H
#define	PING_H

#include <sys/time.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "minilib/minilib.h"

typedef enum	e_icmp_request {
	UNEXPECTED_PACKET,
	TIME_EXCEEDED,
	ECHO_REPLY
}				t_icmp_request;

struct preset {
	struct	addrinfo	ai;
	int					domain;
	uint16_t			payload_len;

	struct {
		uint32_t		ttl: 1;
	} flag;

	uint8_t				ttl;
	uint64_t			attempts_count;
};

struct icmp4 {
	struct ip			iphdr;
	struct icmphdr		icmphdr;
};

struct icmp6 {
	struct ip6_hdr		iphdr;
	struct icmp6_hdr	icmphdr;
};

struct profile {
	int					sock;
	void				*packet;
	struct msghdr		*message;

	struct addrinfo		ai;
	uint16_t			packet_len;
	uint64_t			packet_count;

	// addresses ipv4 or ipv6 functions
	void*	(*create_icmphdr)(uint16_t, uint16_t);
	void*	(*increment_icmphdr)(void*, uint16_t);
	int		(*imcp_reply_handler)(struct msghdr*, ssize_t);
};

struct printable {
	uint16_t		packet_len;
	char			address[INET6_ADDRSTRLEN];
	int				icmp_sequence;
	int				ttl;
	struct timeval	time;
};

int	dns_lookup(const char *address, int ai_family, struct addrinfo *ai);

uint16_t	checksum_rfc1071(void *data, int length);
void		*create_icmphdr_with_payload(uint8_t type, uint16_t id, uint16_t plen);

int	argv_handler(int argc, char *argv[], struct preset *preset);

void	*create_icmp4_hdr(uint16_t id, uint16_t payload_len);
void	*increment_icmp4_hdr(void *hdr, uint16_t payload_len);
int		icmp4_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes);
int		icmp4_reply_printer(struct msghdr *msghdr, ssize_t recieved_bytes);

void	*create_icmp6_hdr(uint16_t id, uint16_t payload_len);
void	*increment_icmp6_hdr(void *hdr, uint16_t payload_len);
int		icmp6_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes);
#endif
