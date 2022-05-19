#include "ping.h"

struct ping global_ping;

int		dns_lookup(const char *address, int ai_family, struct addrinfo *ai) {
	struct addrinfo hints;
	ft_memset(&hints, 0, sizeof(hints));
	hints.ai_family = ai_family;
	struct addrinfo *node = NULL;
	if (getaddrinfo(address, NULL, &hints, &node) != 0)
		return (0);
	ft_memcpy((void *)ai, (const void*)node, sizeof(struct addrinfo));
	ai->ai_next = NULL;
	freeaddrinfo(node);
	return (1);
}

int		socket_create(int domain, uint8_t ttl_override, uint8_t ttl) {
	int	sock = socket(domain, SOCK_RAW,
						(domain == AF_INET ? IPPROTO_ICMP : IPPROTO_ICMPV6));
	if (sock == -1)
		return (-1);
	int ret = 0;
	// set ttl
	if (ttl_override == 1) {
		int ttl6 = ttl;
		if (domain == AF_INET)
			ret = setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
		else
			ret = setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &ttl6, sizeof(ttl6));
		if (ret != 0)
			fprintf(stderr, "ft_ping: warning: failed to set socket parameter: ttl %d\n", ttl6);
	}
	// set cmsghdr ttl recieve
	int flag = 1;
	if (domain == AF_INET)
		ret = setsockopt(sock, IPPROTO_IP, IP_RECVTTL, &flag, sizeof(flag));
	else
		ret = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &flag, sizeof(flag));
	if (ret != 0)
		fprintf(stderr, "ft_ping: warning: failed to set socket parameter: recvttl\n");
	return (sock);
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

int				release_profile(struct profile *profile) {
	if (profile == NULL)
		return (0);
	if (profile->sock > 0)
		close(profile->sock);
	if (profile->packet != NULL)
		free(profile->packet);
	if (profile->message != NULL)
		free(profile->message);
	free(profile);
	return (0);
}

struct profile	*create_profile(struct preset *preset) {
	struct profile *profile = malloc(sizeof(struct profile));
	if (profile == NULL)
		return (NULL);
	ft_memset(profile, 0, sizeof(struct profile));

	size_t msghdr_len = 0, reserve = 128;
	if (preset->domain == AF_INET) {
		profile->create_icmphdr = create_icmp4_hdr;
		profile->increment_icmphdr = increment_icmp4_hdr;
		profile->imcp_reply_handler = icmp4_reply_handler;
		profile->total_len = sizeof(struct icmphdr) + preset->payload_len;
		msghdr_len = sizeof(struct icmp4) + preset->payload_len + reserve;
	} else {
		profile->create_icmphdr = create_icmp6_hdr;
		profile->increment_icmphdr = increment_icmp6_hdr;
		profile->imcp_reply_handler = icmp6_reply_handler;
		msghdr_len = sizeof(struct icmp6) + preset->payload_len + reserve;
		profile->total_len = sizeof(struct icmp6_hdr) + preset->payload_len;
	}
	msghdr_len += reserve - msghdr_len % reserve;

	profile->sock = socket_create(preset->domain, preset->flag.ttl, preset->ttl);
	if (profile->sock == -1)
		return ((void*)(uint64_t)release_profile(profile));

	profile->packet = profile->create_icmphdr(getpid(), preset->payload_len);
	if (profile->packet == NULL)
		return ((void*)(uint64_t)release_profile(profile));

	profile->message = create_msghdr(1024, 1024);
	if (profile->message == NULL)
		return ((void*)(uint64_t)release_profile(profile));

	profile->domain = preset->domain;
	profile->attempts_count = preset->attempts_count;
	ft_memcpy(&profile->ai, &preset->ai, sizeof(struct addrinfo));
	ft_memcpy(&profile->flag, &preset->flag, sizeof(preset->flag));
	return (profile);
}

int	print_statistics(struct sockaddr_in *sa, struct statistics *statistics) {
	char addr[INET6_ADDRSTRLEN];
	inet_ntop(sa->sin_family, &sa->sin_addr, addr, INET6_ADDRSTRLEN);
	const char *format_string =	"\n--- (%s) statistics---\n"
								"%d packets transmitted, %d received, %.0f%% packet loss, time %.2fms\n"
								"rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n";
	fprintf(stdout, format_string, addr,
		statistics->packets_transmitted, statistics->packets_received, statistics->packet_loss, statistics->time,
			statistics->min, statistics->avg, statistics->max, statistics->mdev);
	return (0);
}

ssize_t recieve_packet(struct profile *profile) {
	ssize_t recieved_bytes = recvmsg(profile->sock, profile->message, MSG_WAITALL);
	if (recieved_bytes == -1)
		return (-1);
	struct msghdr *message = profile->message;

	if (message->msg_namelen == INET_ADDRSTRLEN) {
		struct icmp4 *icmp4 = message->msg_iov->iov_base;
		while (!(icmp4->icmphdr.un.echo.id == getpid() && icmp4->icmphdr.type != ICMP_ECHO))
			recieved_bytes = recvmsg(profile->sock, profile->message, MSG_WAITALL);
		return (recieved_bytes);
	} else {
		struct icmp6_hdr *icmp6_hdr = message->msg_iov->iov_base;
		while (!(ntohs(icmp6_hdr->icmp6_id) == getpid() && icmp6_hdr->icmp6_type != ICMP6_ECHO_REQUEST))
			recieved_bytes = recvmsg(profile->sock, profile->message, MSG_WAITALL);
		return (recieved_bytes);
	}
}

int	ping(struct profile *profile, struct statistics *statistics) {
	char addr[INET6_ADDRSTRLEN];
	struct sockaddr_in *sa = profile->ai.ai_addr;
	inet_ntop(profile->domain, &sa->sin_addr, addr, INET6_ADDRSTRLEN);
	uint16_t payload_size = profile->total_len - 8;
	uint16_t summary_size = payload_size + (profile->domain == AF_INET ? sizeof(struct icmp4) : sizeof(struct icmp6_hdr));
	fprintf(stdout, "PING (%s) %hu(%hu) bytes of data.\n", addr, payload_size, summary_size);

	struct timeval time;
	gettimeofday(&time, NULL);
	statistics->time = time.tv_sec * 1000. + time.tv_usec / 1000.;

	uint64_t attempt = 0;
	double sum = 0, sum2 = 0;
	while (1)
	{
		ssize_t sended_bytes = sendto(
			profile->sock, profile->packet, profile->total_len, 0, profile->ai.ai_addr, profile->ai.ai_addrlen);
		if (sended_bytes == -1) {
			attempt++;
			continue;
		}
		statistics->packets_transmitted++;

		ssize_t recieved_bytes = recieve_packet(profile);

		if (recieved_bytes == -1) {
			attempt++;
			continue;
		}
		statistics->packets_received++;

		double rtt = 0;
		enum reply_type type = profile->imcp_reply_handler(profile->message, recieved_bytes, &rtt);
		if (rtt <= statistics->min)
			statistics->min = rtt;
		if (rtt >= statistics->max)
			statistics->max = rtt;
		sum += rtt;
        sum2 += rtt * rtt;
		
		attempt++;
		if (!profile->flag.infinite_attempts
				&& attempt >= profile->attempts_count)
			break;
		sleep(1);
		profile->increment_icmphdr(profile->packet, profile->total_len);
	}

	gettimeofday(&time, NULL);
	statistics->time = (time.tv_sec * 1000. + time.tv_usec / 1000.) - statistics->time;
	statistics->avg = sum / statistics->packets_received;
	sum /= statistics->packets_received;
    sum2 /= statistics->packets_received;
    statistics->mdev = sqrt(sum2 - sum * sum);
	statistics->packet_loss = 1. -
		(double)statistics->packets_transmitted / (double)statistics->packets_received;
	return (0);
}

int	main(int argc, char *argv[]) {
	struct preset preset;
	ft_memset(&preset, 0, sizeof(preset));
	preset.domain = AF_INET;
	preset.payload_len = 56;
	preset.flag.infinite_attempts = 1;

	if (argv_handler(argc, argv, &preset) == -1)
		exit(EXIT_FAILURE);
	struct profile *profile = create_profile(&preset);
	if (profile == NULL) {
		fprintf(stderr, "FAILURE\n");
		return (0);
	}
	global_ping = (struct ping) {
		.profile = profile,
		.statistics = (struct statistics){
			.packets_transmitted = 0,
			.packets_received = 0,
			.packet_loss = 0,
			.time = 0,
			.min = __DBL_MAX__, .avg = 0, .max = 0., .mdev = 0
		}
	};

	ping(profile, &global_ping.statistics);
	print_statistics(profile->ai.ai_addr, &global_ping.statistics);
	release_profile(profile);
	return (0);
}
