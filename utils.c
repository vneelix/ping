/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   icmp_packet.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/09 23:46:20 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/19 20:52:28 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

int	add_timestamp(void *icmphdr, size_t offset) {
	struct timeval timestamp;
	if (gettimeofday(&timestamp, NULL) == 0)
		*((struct timeval*)(icmphdr + offset)) = timestamp;
	return (0);
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
