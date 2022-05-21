#ifndef	PING_H
#define	PING_H

#include <math.h>

#include <sys/time.h>
#include <signal.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <poll.h>

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

enum reply_type {
	UNEXPECTED_PACKET,
	TIME_EXCEEDED,
	ECHO_REPLY
};

struct preset {
	struct	addrinfo	*ai;
	int					domain;
	uint16_t			payload_len;

	struct {
		uint32_t		ttl: 1;
		uint32_t		infinite_attempts: 1;
	} flag;

	uint8_t				ttl;
	uint64_t			attempts_count;
	double				interval;
};

struct icmp4 {
	struct ip			iphdr;
	struct icmphdr		icmphdr;
};

struct profile {
	int					sock;
	void				*packet;
	struct msghdr		*message;

	struct addrinfo		*ai;
	int					domain;
	uint16_t			total_len;

	struct {
		uint32_t		ttl: 1;
		uint32_t		infinite_attempts: 1;
	} flag;
	uint64_t			attempts_count;
	double				interval;

	void*				(*create_icmphdr)(uint16_t, uint16_t);
	void*				(*increment_icmphdr)(void*, uint16_t);
	int					(*imcp_reply_handler)(struct msghdr*, ssize_t, double*);
};

struct statistics {
	int		packets_transmitted;
	int		packets_received;
	double	packet_loss;
	double	time;

	double	min, avg, max, mdev;

	double	sum, sum2;

	int		errors;
};

struct ping {
	struct profile *profile;
	struct statistics statistics;
};

/* icmp4 */
void			*create_icmp4_hdr(uint16_t id, uint16_t payload_len);
void			*increment_icmp4_hdr(void *packet, uint16_t total_len);
int				icmp4_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes, double *rtt);

/* icmp6 */
void			*create_icmp6_hdr(uint16_t id, uint16_t payload_len);
void			*increment_icmp6_hdr(void *hdr, uint16_t total_len);
int				icmp6_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes, double *rtt);

/* profile */
struct profile	*create_profile(struct preset *preset);
int				release_profile(struct profile *profile);

/* utils */
uint16_t		checksum_rfc1071(void *data, int length);
int				add_timestamp(void *icmphdr, size_t offset);
int				dns_lookup(const char *address, int ai_family,
												struct addrinfo **ai);
int				extract_control_data(struct msghdr *msghdr, int level,
															int type, void **data);
struct msghdr	*create_msghdr(size_t buffer_len, size_t msg_controllen);
int				socket_create(int domain, uint8_t ttl_override, uint8_t ttl);

int				argv_handler(int argc, char *argv[], struct preset *preset);
#endif
