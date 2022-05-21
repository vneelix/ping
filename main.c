#include "ping.h"
struct ping global_ping;


int	conclude_statistics(struct statistics *statistics) {
	struct timeval time;
	gettimeofday(&time, NULL);
	statistics->time = (time.tv_sec * 1000. + time.tv_usec / 1000.) - statistics->time;
	if (statistics->packets_received == 0)
		return (0);
	statistics->avg = statistics->sum / statistics->packets_received;
	statistics->sum /= statistics->packets_received;
    statistics->sum2 /= statistics->packets_received;
    statistics->mdev = sqrt(statistics->sum2 - statistics->sum * statistics->sum);
	statistics->packet_loss = (1.
		- (double)statistics->packets_received / (double)statistics->packets_transmitted) * 100.;
	return (0);
}

int	print_statistics(struct sockaddr_in *sa, struct statistics *statistics) {
	char addr[INET6_ADDRSTRLEN];
	inet_ntop(sa->sin_family, &sa->sin_addr, addr, INET6_ADDRSTRLEN);
	const char *format_string =	"\n--- (%s) statistics---\n"
								"%d packets transmitted, %d received, %.2f%% packet loss, time %.2fms\n\n";;
	if (statistics->errors != 0)
		format_string =	"\n--- (%s) statistics---\n"
						"%d packets transmitted, %d received, +%d errors, %.2f%% packet loss, time %.2fms\n\n";
	if (statistics->min != 0. && statistics->avg != 0. && statistics->max != 0. && statistics->mdev != 0.)
		format_string =	"\n--- (%s) statistics---\n"
						"%d packets transmitted, %d received, %.2f%% packet loss, time %.2fms\n"
						"rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n";
	if (statistics->errors != 0)
		fprintf(stdout, format_string, addr,
			statistics->packets_transmitted, statistics->packets_received,
				statistics->errors, statistics->packet_loss, statistics->time,
					statistics->min, statistics->avg, statistics->max, statistics->mdev);
	else
		fprintf(stdout, format_string, addr,
			statistics->packets_transmitted, statistics->packets_received, statistics->packet_loss,
				statistics->time, statistics->min, statistics->avg, statistics->max, statistics->mdev);
	return (0);
}


ssize_t recieve_packet(struct profile *profile) {
	ssize_t recieved_bytes = recvmsg(profile->sock, profile->message, MSG_WAITALL);
	if (recieved_bytes == -1)
		return (-1);
	struct msghdr *message = profile->message;

	if (message->msg_namelen == INET_ADDRSTRLEN) {
		struct icmp4 *icmp4 = message->msg_iov->iov_base;
		while (icmp4->icmphdr.type == ICMP_ECHO)
			recieved_bytes = recvmsg(profile->sock, profile->message, MSG_WAITALL);
		return (recieved_bytes);
	} else {
		struct icmp6_hdr *icmp6_hdr = message->msg_iov->iov_base;
		while (icmp6_hdr->icmp6_type == ICMP6_ECHO_REQUEST)
			recieved_bytes = recvmsg(profile->sock, profile->message, MSG_WAITALL);
		return (recieved_bytes);
	}
}

int	ping(struct profile *profile, struct statistics *statistics) {
	char addr[INET6_ADDRSTRLEN];
	struct sockaddr_in *sa = (struct sockaddr_in*)profile->ai->ai_addr;
	inet_ntop(profile->domain, &sa->sin_addr, addr, INET6_ADDRSTRLEN);
	uint16_t payload_size = profile->total_len - 8;
	uint16_t summary_size = payload_size + (profile->domain == AF_INET ? sizeof(struct icmp4) : sizeof(struct icmp6_hdr));
	fprintf(stdout, "PING (%s) %hu(%hu) bytes of data.\n", addr, payload_size, summary_size);

	struct timeval time;
	gettimeofday(&time, NULL);
	statistics->time = time.tv_sec * 1000. + time.tv_usec / 1000.;

	uint64_t attempt = 0;
	while (1)
	{
		ssize_t sended_bytes = sendto(
			profile->sock, profile->packet, profile->total_len, 0, profile->ai->ai_addr, profile->ai->ai_addrlen);
		if (sended_bytes == -1)
			fprintf(stderr, "ft_ping: %zu: failed to send the packet\n", attempt);
		else
			statistics->packets_transmitted++;

		ssize_t recieved_bytes = -1;
		if (sended_bytes != -1) {
			recieved_bytes = recieve_packet(profile);
			if (recieved_bytes != -1)
				statistics->packets_received++;
		}

		double rtt = 0;
		enum reply_type type = UNEXPECTED_PACKET;
		if (recieved_bytes != -1)
			type = profile->imcp_reply_handler(profile->message, recieved_bytes, &rtt);
		if (rtt <= statistics->min)
			statistics->min = rtt;
		if (rtt >= statistics->max)
			statistics->max = rtt;
		statistics->sum += rtt;
        statistics->sum2 += rtt * rtt;
		if (type != ECHO_REPLY)
			statistics->errors++;

		attempt++;
		if (!profile->flag.infinite_attempts
				&& attempt >= profile->attempts_count)
			break;
		usleep(profile->interval);
		profile->increment_icmphdr(profile->packet, profile->total_len);
	}
	conclude_statistics(statistics);
	return (0);
}


void	ping_sigint(int signal) {
	(void)signal;
	conclude_statistics(&global_ping.statistics);
	print_statistics(
		(struct sockaddr_in*)global_ping.profile->ai->ai_addr, &global_ping.statistics);
	release_profile(global_ping.profile);
	exit(EXIT_SUCCESS);
}

int	main(int argc, char *argv[]) {
	struct preset preset;
	ft_memset(&preset, 0, sizeof(preset));
	preset.domain = AF_INET;
	preset.interval = 1. * 1000000.;
	preset.payload_len = 56;
	preset.flag.infinite_attempts = 1;

	if (argv_handler(argc, argv, &preset) == -1)
		exit(EXIT_FAILURE);
	struct profile *profile = create_profile(&preset);
	if (profile == NULL) {
		fprintf(stderr, "ft_ping: unable to create profile\n");
		exit(EXIT_FAILURE);
	}
	global_ping = (struct ping) {
		.profile = profile,
		.statistics = (struct statistics){
			.packets_transmitted = 0,
			.packets_received = 0,
			.packet_loss = 0,
			.time = 0,
			.min = __DBL_MAX__, .avg = 0, .max = 0., .mdev = 0,
			.sum = 0, .sum2 = 0,
			.errors = 0
		}
	};
	signal(SIGINT, ping_sigint);

	ping(profile, &global_ping.statistics);
	print_statistics((struct sockaddr_in*)profile->ai->ai_addr, &global_ping.statistics);
	release_profile(profile);
	return (0);
}
