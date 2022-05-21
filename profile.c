/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   profile.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/05/21 12:28:06 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/21 23:58:29 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

int	release_profile(struct profile *profile) {
	if (profile == NULL)
		return (0);
	if (profile->sock > 0)
		close(profile->sock);
	if (profile->packet != NULL)
		free(profile->packet);
	if (profile->message != NULL)
		free(profile->message);
	if (profile->ai != NULL)
		freeaddrinfo(profile->ai);
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
		msghdr_len = sizeof(struct icmp6_hdr) + preset->payload_len + reserve;
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

	profile->ai = preset->ai;
	profile->domain = preset->domain;
	profile->interval = preset->interval;
	profile->attempts_count = preset->attempts_count;
	ft_memcpy(&profile->flag, &preset->flag, sizeof(preset->flag));
	return (profile);
}
