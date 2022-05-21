/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/09 23:46:20 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/21 12:47:59 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

uint16_t	checksum_rfc1071(void *data, int length) {
    uint32_t	sum = 0;
	uint16_t	*buf = (uint16_t*)data;

	for (sum = 0; length > 1; length -= 2)
		sum += *buf++;
    if (length == 1)
		sum += *(uint8_t*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += sum >> 16;
	sum = ~sum;
    return (sum);
}

int	add_timestamp(void *icmphdr, size_t offset) {
	struct timeval timestamp;
	if (gettimeofday(&timestamp, NULL) == 0)
		*((struct timeval*)(icmphdr + offset)) = timestamp;
	return (0);
}

int	dns_lookup(const char *address, int ai_family, struct addrinfo **ai) {
	struct addrinfo hints;
	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = ai_family;
	struct addrinfo *node = NULL;
	if (getaddrinfo(address, NULL, &hints, &node) != 0)
		return (0);
	*ai = node;
	return (1);
}

int	extract_control_data(struct msghdr *msghdr, int level, int type, void **data) {
	struct cmsghdr *cmsghdr = CMSG_FIRSTHDR(msghdr);
	while (cmsghdr != NULL) {
		if (cmsghdr->cmsg_level == level && cmsghdr->cmsg_type == type) {
			*data = CMSG_DATA(cmsghdr);
			return (0);
		}
		cmsghdr = CMSG_NXTHDR(msghdr, cmsghdr);
	}
	return (-1);
}

struct msghdr	*create_msghdr(size_t buffer_len, size_t msg_controllen) {
	size_t total_len = sizeof(struct msghdr) + INET6_ADDRSTRLEN
						+ sizeof(struct iovec) + buffer_len + msg_controllen;
	void *buffer = malloc(total_len);
	struct msghdr *msghdr = buffer;
	if (msghdr == NULL)
		return (NULL);
	memset((void*)msghdr, 0, total_len);

	msghdr->msg_name = buffer + sizeof(struct msghdr);
	msghdr->msg_namelen = INET6_ADDRSTRLEN;

	msghdr->msg_iovlen = 1;
	msghdr->msg_iov = msghdr->msg_name + INET6_ADDRSTRLEN;
	msghdr->msg_iov->iov_len = buffer_len;
	msghdr->msg_iov->iov_base = (void*)msghdr->msg_iov + sizeof(struct iovec);

	if (msg_controllen != 0) {
		msghdr->msg_controllen = msg_controllen;
		msghdr->msg_control = (void*)msghdr->msg_iov + sizeof(struct iovec) + msghdr->msg_iov->iov_len;
	}

	return (msghdr);
}

int		socket_create(int domain, uint8_t ttl_override, uint8_t ttl) {
	int	sock = socket(domain, SOCK_RAW,
						(domain == AF_INET ? IPPROTO_ICMP : IPPROTO_ICMPV6));
	if (sock == -1)
		return (-1);
	int ret = 0;
	if (ttl_override == 1) {
		int ttl6 = ttl;
		if (domain == AF_INET)
			ret = setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
		else
			ret = setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &ttl6, sizeof(ttl6));
		if (ret != 0)
			fprintf(stderr, "ft_ping: warning: failed to set socket parameter: ttl %d\n", ttl6);
	}
	int flag = 1;
	if (domain == AF_INET)
		ret = setsockopt(sock, IPPROTO_IP, IP_RECVTTL, &flag, sizeof(flag));
	else
		ret = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &flag, sizeof(flag));
	if (ret != 0)
		fprintf(stderr, "ft_ping: warning: failed to set socket parameter: recvttl\n");
	return (sock);
}
