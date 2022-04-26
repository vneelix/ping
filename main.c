#include "ping.h"

#include <stdio.h>

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
	int	sock = socket(domain, SOCK_RAW, (domain == AF_INET ? IPPROTO_ICMP : IPPROTO_ICMPV6));
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

/* int	ping(t_config *config) {
	int sock;
	void *icmphdr;
	struct msghdr *msghdr;
	if (init_ping(config, &sock, &icmphdr, &msghdr) == -1)
		return (-1);

	// get char representation of address
	char ip_src[INET6_ADDRSTRLEN];
	ft_memset((void*)ip_src, 0, sizeof(ip_src));
	{
		void *addr = NULL;
		if (config->domain == AF_INET)
			addr = &((struct sockaddr_in*)config->ai->ai_addr)->sin_addr;
		else
			addr = &((struct sockaddr_in6*)config->ai->ai_addr)->sin6_addr;
		const char *ret = inet_ntop(config->domain, addr, ip_src, (socklen_t)sizeof(ip_src));
	}

	fprintf(stdout, "ft_ping (%s) %hu(%hu) bytes of data.\n",
			ip_src, config->payload_size, (uint16_t)(config->payload_size + 28));

	size_t msglen = sizeof(struct icmphdr) + config->payload_size;
	const struct sockaddr *addr = config->ai->ai_addr;
	socklen_t addrlen = config->ai->ai_addrlen;

	struct timeval send_time, recieve_time;
	config->attempts_count = 8;

	for (uint64_t attempt = 0; attempt != config->attempts_count; attempt++) {
		struct timeval time;
		gettimeofday(&time, NULL);
		long *timestamp = icmphdr + sizeof(struct icmphdr);
		*timestamp = time.tv_sec;
		struct icmphdr *p = icmphdr;
		p->checksum = 0;
		p->checksum = checksum_rfc1071(p, msglen);

		gettimeofday(&send_time, NULL);
		ssize_t	ret = sendto(sock, icmphdr, msglen, 0, addr, addrlen);
		if (ret == -1) {
				fprintf(stdout, "ft_ping: error: unable to send packet\n");
				break;
		}
		ret = recvmsg(sock, msghdr, 0);
		if (ret == -1)
			continue;
		gettimeofday(&recieve_time, NULL);
		double diff = (recieve_time.tv_usec - send_time.tv_usec) / 1000.;
		ret = icmp_reply_handler(msghdr, ret);
	}
} */

/* int	init_ping(t_config *config, int *sock, void **icmphdr, struct msghdr **msghdr) {
	int s = socket_create(config->domain, config->ttl_override, config->ttl);
	if (s == -1) {
		fprintf(stderr, "ft_ping: error: failed to create socket\n");
		return (-1);
	}
	void *hdr = create_icmphdr_with_payload(ICMP_ECHO, (uint16_t)getpid(), config->payload_size);
	if (hdr == NULL) {
		fprintf(stderr, "ft_ping: error: failed to create ICMP header: size %hu\n", config->payload_size);
		close(s);
		return (-1);
	}
	struct msghdr *message_hdr = create_msghdr(512, 0);
	if (msghdr == NULL) {
		fprintf(stderr, "ft_ping: error: failed to prepare the environment: struct msghdr alloc\n");
		free(hdr);
		close(s);
		return (-1);
	}
	*sock = s;
	*icmphdr = hdr;
	*msghdr = message_hdr;
	return (0);
} */

/* int	profile_init(struct profile *profile, int *sock, void **icmphdr, struct msghdr **msghdr) {
	// create socket for transmitting/receiving
	int s = socket_create(profile->domain, profile->ttl_override, profile->ttl);
	if (s == -1) {
		fprintf(stderr, "ft_ping: error: failed to create socket\n");
		return (-1);
	}

	// create data to sent
	// void *hdr = create_icmphdr_with_payload(ICMP_ECHO, (uint16_t)getpid(), profile->payload_len);
	void *hdr = NULL;
	if (hdr == NULL) {
		fprintf(stderr, "ft_ping: error: failed to create ICMP header\n");
		close(s);
		return (-1);
	}

	// create buffer for receiving
	size_t size = 0;
	if (profile->domain == AF_INET)
		size = sizeof(struct ip) + sizeof(struct icmphdr) + profile->payload_len;
	else
		size = sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr) + profile->payload_len;
	struct msghdr *message_hdr = create_msghdr(size, 0);
	if (msghdr == NULL) {
		fprintf(stderr, "ft_ping: error: failed to create msghdr\n");
		free(hdr);
		close(s);
		return (-1);
	}

	*sock = s;
	*icmphdr = hdr;
	*msghdr = message_hdr;
	return (0);
} */

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

	if (preset->domain == AF_INET) {
		profile->create_icmphdr = create_icmp4_hdr;
		profile->increment_icmphdr = increment_icmp4_hdr;
		profile->imcp_reply_handler = icmp4_reply_handler;
		profile->packet_len = sizeof(struct icmphdr) + preset->payload_len;
	} else {
		profile->create_icmphdr = create_icmp6_hdr;
		profile->increment_icmphdr = increment_icmp6_hdr;
		profile->imcp_reply_handler = icmp6_reply_handler;
		profile->packet_len = sizeof(struct icmp6_hdr) + preset->payload_len;
	}

	profile->sock = socket_create(preset->domain, preset->flag.ttl, preset->ttl);
	if (profile->sock == -1)
		return ((void*)(uint64_t)release_profile(profile));

	profile->packet = profile->create_icmphdr(getpid(), preset->payload_len);
	if (profile->packet == NULL)
		return ((void*)(uint64_t)release_profile(profile));

	profile->message = create_msghdr(256 + preset->payload_len, 256);
	if (profile->message == NULL)
		return ((void*)(uint64_t)release_profile(profile));

	profile->packet_count = preset->attempts_count;
	ft_memcpy(&profile->ai, &preset->ai, sizeof(struct addrinfo));
	return (profile);
}

int	main(int argc, char *argv[]) {
	struct preset preset;
	ft_memset(&preset, 0, sizeof(preset));
	preset.payload_len = 56;
	preset.attempts_count = ~((uint64_t)0);

	if (argv_handler(argc, argv, &preset) == -1)
		exit(EXIT_FAILURE);
	struct profile *profile = create_profile(&preset);

	while (1) {
		ssize_t ret = sendto(profile->sock, profile->packet, profile->packet_len, 0, profile->ai.ai_addr, profile->ai.ai_addrlen);

		ssize_t msglen = recvmsg(profile->sock, profile->message, MSG_WAITALL);
		if (ret != -1)
			ret = profile->imcp_reply_handler(profile->message, ret);

		icmp4_reply_printer(profile->message, msglen);
		exit(0);
		// int ttl = 0;
		// for (struct cmsghdr *cmsghdr = CMSG_FIRSTHDR(profile->message);
		// 		cmsghdr != NULL; cmsghdr = CMSG_NXTHDR(profile->message, cmsghdr)) {
		// 	if (cmsghdr->cmsg_level == IPPROTO_IPV6 && cmsghdr->cmsg_type == IPV6_HOPLIMIT)
		// 		ttl = *(int*)CMSG_DATA(cmsghdr);
		// }
		sleep(1);
	}

	exit(EXIT_SUCCESS);
	return (0);
}
