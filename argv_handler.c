/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argv_handler.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/15 00:19:44 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/17 21:04:00 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

int	argv_handler(int argc, char *argv[], struct preset *preset) {
	const char	*address = NULL;

	for (int i = 1; i != argc; i++) {
		if (argv[i][0] != '-') {
			address = argv[i];
			continue;
		}
		if (ft_strlen(argv[i]) != 2) {
			fprintf(stderr, "ft_ping: invalid option: %s\n", argv[i]);
			return (-1);
		}

		switch (argv[i][1]) {
			case 'h':
				break;
			case 'v':
				break;
			case '4':
				preset->domain = AF_INET;
				break;
			case '6':
				preset->domain = AF_INET6;	
				break;
			case 't':
				if (i + 1 == argc || !is_number(argv[i + 1])) {
					fprintf(stderr, "ft_ping: -t invalid argument: %s\n", i + 1 == argc ? "(null)" : argv[i + 1]);
					return (-1);
				}
				i++;
				preset->flag.ttl = 1;
				preset->ttl = ft_atoi(argv[i]);
				break;
			case 'c':
				if (i + 1 == argc || !is_number(argv[i + 1])) {
					fprintf(stderr, "ft_ping: -c invalid argument: %s\n", i + 1 == argc ? "(null)" : argv[i + 1]);
					return (-1);
				}
				i++;
				preset->flag.infinite_attempts = 0;
				preset->attempts_count = ft_atoi(argv[i]);
				break;
			case 's':
				if (i + 1 == argc || !is_number(argv[i + 1])) {
					fprintf(stderr, "ft_ping: -s invalid argument: %s\n", i + 1 == argc ? "(null)" : argv[i + 1]);
					return (-1);
				}
				i++;
				preset->payload_len = ft_atoi(argv[i]);
				break;
			default:
				fprintf(stderr, "ft_ping: invalid option: %s\n", argv[i]);
				return (-1);
				break;
		}
	}
	if (address == NULL) {
		fprintf(stderr, "ft_ping: usage error: destination address required\n");
		return (-1);
	}
	if (!dns_lookup(address, preset->domain, &preset->ai)) {
		fprintf(stderr, "ft_ping: %s: name or service not known\n", address);
		return (-1);
	}
	return (0);
}
