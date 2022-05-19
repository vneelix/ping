/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   icmp6.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/05/19 20:51:26 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/19 21:00:04 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

void	*create_icmp6_hdr(uint16_t id, uint16_t payload_len) {
	void	*buffer = malloc(sizeof(struct icmp6_hdr) + payload_len);
	if (buffer == NULL)
		return (NULL);
	ft_memset(buffer, 0, sizeof(struct icmp6_hdr) + payload_len);

	struct icmp6_hdr *icmp6_hdr = (struct icmp6_hdr*)buffer;
	icmp6_hdr->icmp6_type = ICMP6_ECHO_REQUEST;
	icmp6_hdr->icmp6_id = htons(id);
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

static int	icmp6_reply_printer(enum reply_type type,
				struct msghdr *msghdr, ssize_t recieved_bytes, double rtt) {
	struct icmp6_hdr *icmp6_hdr = msghdr->msg_iov->iov_base;
	const char *format_string = NULL;
	switch (type) {
		case ECHO_REPLY:
			if (recieved_bytes - sizeof(struct icmp6_hdr) >= sizeof(struct timeval))
				format_string = "%lu bytes from (%s): icmp_seq=%d ttl=%d time=%.2f ms\n";
			else
				format_string = "%lu bytes from (%s): icmp_seq=%d ttl=%d\n";
			break;

		case TIME_EXCEEDED:
			format_string = "%lu bytes from (%s): icmp_seq=%d Time to live exceeded\n";
			icmp6_hdr += 1;
			break;

		default:
			format_string = "%lu bytes from (%s): unprocessed ICMP type\n";
			break;
	}

	char addr[INET6_ADDRSTRLEN];
	struct sockaddr_in *sa = msghdr->msg_name;
	inet_ntop(AF_INET6, &sa->sin_addr, addr, INET6_ADDRSTRLEN);

	int *ttl = NULL;
	int icmp_sequence = ntohs(icmp6_hdr->icmp6_seq);
	extract_control_data(msghdr, IPPROTO_IPV6, IPV6_HOPLIMIT, (void**)&ttl);

	fprintf(stdout, format_string, recieved_bytes, addr, icmp_sequence, *ttl, rtt);
	return (0);
}

int		icmp6_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes, double *rtt) {	
	enum reply_type type;
	struct icmp6_hdr *icmp6_hdr = msghdr->msg_iov->iov_base;
	switch (icmp6_hdr->icmp6_type) {
		case ICMP6_ECHO_REPLY:
			if (ntohs(icmp6_hdr->icmp6_id) != getpid())
				type = UNEXPECTED_PACKET;
			type = ECHO_REPLY;
			break;
		case ICMP6_TIME_EXCEEDED:
			icmp6_hdr = (void*)icmp6_hdr + sizeof(struct icmp6_hdr);
			if (ntohs(icmp6_hdr->icmp6_id) != getpid())
				type = UNEXPECTED_PACKET;
			type = TIME_EXCEEDED;
			break;
		default:
			type = UNEXPECTED_PACKET;
			break;
	}

	double round_trip_time = 0;
	if (type == ECHO_REPLY &&
		recieved_bytes - sizeof(struct icmp6_hdr) >= sizeof(struct timeval))
	{
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		struct timeval *time_from_packet = (struct timeval*)((void*)icmp6_hdr + sizeof(struct icmp6_hdr));
		round_trip_time = ((current_time.tv_sec - time_from_packet->tv_sec) * 1000000.
								+ (current_time.tv_usec - time_from_packet->tv_usec)) / 1000.;
	}
	if (rtt != NULL)
		*rtt = round_trip_time;

	if (1)
		icmp6_reply_printer(type, msghdr, recieved_bytes, round_trip_time);

	return (type);
}
