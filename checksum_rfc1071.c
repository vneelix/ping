/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   checksum_rfc1071.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/10 00:07:36 by vneelix           #+#    #+#             */
/*   Updated: 2022/03/20 17:46:24 by vneelix          ###   ########.fr       */
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
