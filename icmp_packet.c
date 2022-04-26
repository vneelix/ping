/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   icmp_packet.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/09 23:46:20 by vneelix           #+#    #+#             */
/*   Updated: 2022/04/26 20:37:24 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

int		add_timestamp(void *icmphdr, size_t offset) {
	struct timeval timestamp;
	if (gettimeofday(&timestamp, NULL) == 0)
		*((struct timeval*)(icmphdr + offset)) = timestamp;
	return (0);
}

// ipv4
void	*create_icmp4_hdr(uint16_t id, uint16_t payload_len) {
	void	*buffer = malloc(sizeof(struct icmphdr) + payload_len);
	if (buffer == NULL)
		return (NULL);
	ft_memset(buffer, 0, sizeof(struct icmphdr) + payload_len);

	struct icmphdr *icmphdr = (struct icmphdr*)buffer;
	icmphdr->type = ICMP_ECHO;
	icmphdr->un.echo.id = id;
	icmphdr->un.echo.sequence = 1;
	if (payload_len >= sizeof(struct timeval))
		add_timestamp((void*)icmphdr, sizeof(struct icmphdr));
	icmphdr->checksum = checksum_rfc1071(buffer, sizeof(struct icmphdr) + payload_len);
	return ((void*)icmphdr);
}

void	*increment_icmp4_hdr(void *hdr, uint16_t payload_len) {
	struct icmphdr *icmphdr = (struct icmphdr*)hdr;
	icmphdr->un.echo.sequence++;
	if (payload_len >= sizeof(struct timeval))
		add_timestamp((void*)icmphdr, sizeof(struct icmphdr));
	icmphdr->checksum = checksum_rfc1071(hdr, sizeof(struct icmphdr) + payload_len);
	return (hdr);
}

int		icmp4_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes) {
	struct icmp4 *packet = (struct icmp4*)msghdr->msg_iov->iov_base;
	if (packet->iphdr.ip_p != IPPROTO_ICMP)
		return (UNEXPECTED_PACKET);
	switch (packet->icmphdr.type) {
		case ICMP_ECHOREPLY:
			if (packet->icmphdr.un.echo.id != getpid())
				return (UNEXPECTED_PACKET);
			return (ECHO_REPLY);
			break;
		case ICMP_TIME_EXCEEDED:
			packet = (void*)packet + sizeof(struct icmp4);
			if (packet->icmphdr.un.echo.id != getpid())
				return (UNEXPECTED_PACKET);
			return (TIME_EXCEEDED);
			break;
		default:
			break;
	}
	return (UNEXPECTED_PACKET);
}

int		extract_control_data(struct msghdr *msghdr, int level, int type, void **data) {
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

int		icmp4_reply_printer(struct msghdr *msghdr, ssize_t recieved_bytes) {
	struct icmp4 *packet = msghdr->msg_iov->iov_base;

	char addr[INET_ADDRSTRLEN];
	struct sockaddr_in *sa = msghdr->msg_name;
	inet_ntop(AF_INET, &sa->sin_addr, addr, INET6_ADDRSTRLEN);

	int icmp_sequence = packet->icmphdr.un.echo.sequence;
	
	int *ttl = NULL;
	extract_control_data(msghdr, IPPROTO_IP, IP_TTL, (void**)&ttl);

	return (0);
}

// ipv6
void	*create_icmp6_hdr(uint16_t id, uint16_t payload_len) {
	void	*buffer = malloc(sizeof(struct icmp6_hdr) + payload_len);
	if (buffer == NULL)
		return (NULL);
	ft_memset(buffer, 0, sizeof(struct icmp6_hdr) + payload_len);

	struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr*)buffer;
	icmp6_hdr->icmp6_type = ICMP6_ECHO_REQUEST;
	icmp6_hdr->icmp6_id = id;
	icmp6_hdr->icmp6_seq = htons(1);
	if (payload_len >= sizeof(struct timeval))
		add_timestamp((void*)icmp6_hdr, sizeof(struct icmp6_hdr));
	icmp6_hdr->icmp6_cksum = checksum_rfc1071(buffer, sizeof(struct icmp6_hdr) + payload_len);
	return ((void*)icmp6_hdr);
}

void	*increment_icmp6_hdr(void *hdr, uint16_t payload_len) {
	struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr*)hdr;
	icmp6_hdr->icmp6_seq = htons(ntohs(icmp6_hdr->icmp6_seq) + 1);
	if (payload_len >= sizeof(struct timeval))
		add_timestamp((void*)icmp6_hdr, sizeof(struct icmp6_hdr));
	icmp6_hdr->icmp6_cksum = checksum_rfc1071(hdr, sizeof(struct icmp6_hdr) + payload_len);
	return (hdr);
}

int		icmp6_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes) {
	struct icmp6_hdr *packet = (struct icmp6_hdr*)msghdr->msg_iov->iov_base;
	switch (packet->icmp6_type) {
		case ICMP6_ECHO_REPLY:
			if (packet->icmp6_id != getpid())
				return (UNEXPECTED_PACKET);
			return (ECHO_REPLY);
			break;
		case ICMP6_TIME_EXCEEDED:
			packet = (void*)packet + sizeof(struct icmp6_hdr);
			if (packet->icmp6_id != getpid())
				return (UNEXPECTED_PACKET);
			return (TIME_EXCEEDED);
			break;
		default:
			break;
	}
	return (UNEXPECTED_PACKET);
}
