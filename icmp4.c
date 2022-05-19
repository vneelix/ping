/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   icmp4.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/05/15 15:34:35 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/19 20:50:15 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

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

void	*increment_icmp4_hdr(void *packet, uint16_t total_len) {
	struct icmphdr *icmphdr = (struct icmphdr*)packet;
	icmphdr->un.echo.sequence++;
	if (total_len - sizeof(icmphdr) >= sizeof(struct timeval))
		add_timestamp(packet, sizeof(struct icmphdr));
	icmphdr->checksum = 0;
	icmphdr->checksum = checksum_rfc1071(packet, total_len);
	return (packet);
}

static int	icmp4_reply_printer(enum reply_type type,
				struct msghdr *msghdr, ssize_t recieved_bytes, double rtt) {
	struct icmp4 *packet = msghdr->msg_iov->iov_base;
	const char *format_string = NULL;
	switch (type) {
		case ECHO_REPLY:
			if (recieved_bytes - sizeof(struct icmp4) >= sizeof(struct timeval))
				format_string = "%lu bytes from (%s): icmp_seq=%d ttl=%d time=%.2f ms\n";
			else
				format_string = "%lu bytes from (%s): icmp_seq=%d ttl=%d\n";
			break;

		case TIME_EXCEEDED:
			format_string = "%lu bytes from (%s): icmp_seq=%d Time to live exceeded\n";
			packet += 1;
			break;

		default:
			format_string = "%lu bytes from (%s): unprocessed ICMP type\n";
			break;
	}

	char addr[INET_ADDRSTRLEN];
	struct sockaddr_in *sa = msghdr->msg_name;
	inet_ntop(AF_INET, &sa->sin_addr, addr, INET_ADDRSTRLEN);

	int *ttl = NULL;
	int icmp_sequence = packet->icmphdr.un.echo.sequence;
	extract_control_data(msghdr, IPPROTO_IP, IP_TTL, (void**)&ttl);

	recieved_bytes -= sizeof(struct ip);
	fprintf(stdout, format_string, recieved_bytes, addr, icmp_sequence, *ttl, rtt);
	return (0);
}

int		icmp4_reply_handler(struct msghdr *msghdr, ssize_t recieved_bytes, double *rtt) {
	struct icmp4 *packet = (struct icmp4*)msghdr->msg_iov->iov_base;
	if (packet->iphdr.ip_p != IPPROTO_ICMP)
		return (UNEXPECTED_PACKET);
	
	enum reply_type type;
	switch (packet->icmphdr.type) {
		case ICMP_ECHOREPLY:
			if (packet->icmphdr.un.echo.id != getpid())
				type = UNEXPECTED_PACKET;
			type = ECHO_REPLY;
			break;
		case ICMP_TIME_EXCEEDED:
			packet = (void*)packet + sizeof(struct icmp4);
			if (packet->icmphdr.un.echo.id != getpid())
				type = UNEXPECTED_PACKET;
			type = TIME_EXCEEDED;
			break;
		default:
			type = UNEXPECTED_PACKET;
			break;
	}

	double round_trip_time = 0;
	if (type == ECHO_REPLY &&
		recieved_bytes - sizeof(struct icmp4) >= sizeof(struct timeval))
	{
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		struct timeval *time_from_packet = (struct timeval*)((void*)packet + sizeof(struct icmp4));
		round_trip_time = ((current_time.tv_sec - time_from_packet->tv_sec) * 1000000.
								+ (current_time.tv_usec - time_from_packet->tv_usec)) / 1000.;
	}
	if (rtt != NULL)
		*rtt = round_trip_time;

	if (1)
		icmp4_reply_printer(type, msghdr, recieved_bytes, round_trip_time);

	return (type);
}
